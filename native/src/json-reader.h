#pragma once

#include "utf.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct JsonObject;
struct JsonArray;

/**
 * @brief Variant representing any primitive or composite JSON value.
 *
 * Types supported:
 * - `void*`: Represents `null` (`nullptr`).
 * - `std::string`: JSON string.
 * - `bool`: JSON boolean (`true` / `false`).
 * - `int64_t`: JSON integer numeral.
 * - `long double`: JSON floating point numeral.
 * - `JsonObject`: JSON object (key-value list).
 * - `JsonArray`: JSON array (ordered list of values).
 */
using JsonValue = std::variant<void*, std::string, bool, int64_t, long double, JsonObject, JsonArray>;

/**
 * @brief Ordered list of JSON values representing a JSON array.
 */
struct JsonArray : public std::vector<JsonValue> { };

/**
 * @brief Ordered key-value list representing a JSON object.
 *
 * Backed by `std::vector<std::pair<std::string, JsonValue>>` to preserve insertion order.
 */
struct JsonObject : public std::vector<std::pair<std::string, JsonValue>> {
  /**
   * @brief Retrieves the value for a given key cast to type `T`.
   *
   * Performs a linear scan over the object entries. If found and the underlying variant
   * holds type `T`, returns the unwrapped value; otherwise returns `std::nullopt`.
   *
   * @tparam T Expected C++ type matching an alternative in `JsonValue`.
   * @param key Property name.
   * @return Extracted value if present and matching type, otherwise `std::nullopt`.
   */
  template <typename T> std::optional<T> get(const std::string_view& key) const noexcept {
    for (const auto& entry : *this) {
      if (entry.first == key) {
        std::optional<T> res { };
        if (std::holds_alternative<T>(entry.second)) {
          res = std::get<T>(entry.second);
        }
        return res;
      }
    }
    return std::nullopt;
  }
};

/**
 * @brief Lightweight, zero-dependency recursive-descent JSON parser.
 *
 * Supports standard JSON primitives, nested objects/arrays, Unicode escape sequences
 * (`\uXXXX` decoded into UTF-8), and C/C++ style comments (both line comments and block comments).
 */
class JsonReader {
public:
  /**
   * @brief Parses a JSON string into a structured JsonValue.
   *
   * @tparam T String container type providing `data()` and `length()`.
   * @param str The raw JSON string or string view.
   * @return Parsed JsonValue on success; std::nullopt on parse error.
   */
  template <typename T> static std::optional<JsonValue> parseJson(const T& str) noexcept {
    auto start = str.data(), end = start + str.length();
    auto val = parseValue(start, end);
    // parseValue uses an empty string as its error sentinel. This also rejects
    // valid top-level strings; nested strings are still retained in containers.
    if (std::holds_alternative<std::string>(val)) {
      return std::nullopt;
    }
    return std::move(val);
  }

private:
  using stringpos = const char*;

  /**
   * @brief Parses the next JSON value (primitive, array, or object) from the input buffer.
   *
   * Recursively consumes tokens starting at `curr` up to `end`.
   *
   * @param curr In-out reference to character cursor in the input text.
   * @param end Pointer past the end of the input buffer.
   * @return Parsed JsonValue variant representing array, object, string, number, bool, or null.
   */
  static JsonValue parseValue(stringpos& curr, stringpos end) noexcept {
    if (!skipSpaces(curr, end)) {
      return "";
    }

    switch (*curr) {
      case '[': {
        curr++;
        skipSpaces(curr, end);
        JsonArray arr;
        while (curr < end && *curr != ']') {
          auto val = parseValue(curr, end);
          if (curr == end) {
            return "";
          }
          arr.emplace_back(std::move(val));
          if (!skipSpaces(curr, end)) {
            return "";
          }
          if (*curr != ',') {
            break;
          }
          curr++;
          skipSpaces(curr, end);
        }
        if (curr < end && *curr == ']') {
          curr++;
          return arr;
        }
      } break;
      case '{': {
        curr++;
        JsonObject obj;
        skipSpaces(curr, end);
        while (curr < end && *curr != '}') {
          if (*curr != '"') {
            curr = end;
            return "";
          }
          curr++;
          auto key = readUntil(curr, end, "\"");
          if (curr == end || *curr != '"') {
            curr = end;
            return "";
          }
          curr++;
          if (!skipSpaces(curr, end)) {
            return "";
          }
          if (*curr != ':') {
            curr = end;
            return "";
          }
          curr++;
          auto val = parseValue(curr, end);
          if (curr == end) {
            return "";
          }
          obj.emplace_back(std::move(key), std::move(val));
          if (!skipSpaces(curr, end)) {
            return "";
          }
          if (*curr != ',') {
            break;
          }
          curr++;
          skipSpaces(curr, end);
        }
        if (*curr == '}') {
          curr++;
          return obj;
        }
      } break;
      case '"': {
        curr++;
        auto str = readUntil(curr, end, "\"");
        if (*curr == '"') {
          curr++;
          return str;
        }
      } break;
      case 't': {
        if (!std::strncmp(curr, "true", 4)) {
          curr += 4;
          return true;
        }
      } break;
      case 'f': {
        if (!std::strncmp(curr, "false", 5)) {
          curr += 5;
          return false;
        }
      } break;
      case 'n': {
        if (!std::strncmp(curr, "null", 4)) {
          curr += 4;
          return nullptr;
        }
      } break;
      default: {
        int64_t sign = 1;
        if (*curr == '-' || *curr == '+') {
          if (*curr == '-') {
            sign = -1;
          }
          curr++;
        }
        int64_t intVal = 0;
        static constexpr std::string_view Numerals { "0123456789" };
        while (Numerals.find(*curr) != Numerals.npos) {
          intVal = intVal * 10 + (*curr++ - '0');
        }
        if (*curr == '.') {
          auto divider = 0.1;
          long double floatVal = 0;
          curr++;
          while (Numerals.find(*curr) != Numerals.npos) {
            floatVal += (*curr++ - '0') * divider;
            divider /= 10;
          }
          return (floatVal + intVal) * sign;
        } else {
          return intVal * sign;
        }
      } break;
    }

    curr = end;
    return "";
  }

  /**
   * @brief Advances cursor until any character matching `delims` is encountered (inclusive).
   *
   * @param curr In-out reference to character cursor in the input buffer.
   * @param end Pointer past the end of the input buffer.
   * @param delims String view of delimiter characters.
   * @return The matched delimiter character, or 0 if end of buffer was reached.
   */
  static char skipUntil(stringpos& curr, stringpos end, std::string_view delims) noexcept {
    while (curr < end) {
      auto ch = *curr++;
      if (delims.find(ch) != delims.npos) {
        return ch;
      }
    }

    return 0;
  }

  /**
   * @brief Skips whitespace characters and C/C++ style comments (`//` and block comments).
   *
   * @param curr In-out reference to character cursor.
   * @param end Pointer past the end of the input buffer.
   * @return true if non-whitespace character was reached before `end`; false if EOF reached.
   */
  static bool skipSpaces(stringpos& curr, stringpos end) noexcept {
    static constexpr std::string_view Spaces { " \t\n\r" };
    while (curr < end) {
      if (Spaces.find(*curr) == Spaces.npos) {
        if (end - curr < 2 || *curr != '/') {
          break;
        }
        if (*(curr + 1) == '/') {
          curr = std::find(curr + 2, end, '\n');
        } else if (*(curr + 1) == '*') {
          constexpr std::string_view EndComment { "*/" };
          curr = std::search(curr + 2, end, EndComment.begin(), EndComment.end());
          if (curr != end) {
            curr += 2;
          }
        } else {
          break;
        }
        continue;
      }
      curr++;
    }

    return curr < end;
  }

  /**
   * @brief Reads characters until any delimiter is encountered (exclusive).
   *
   * Decodes JSON escape sequences in string literals, including:
   * - Single-character escapes (`\t`, `\r`, `\n`, `\"`, `\\`, etc.)
   * - 4-digit hexadecimal Unicode codepoints (`\uXXXX`) encoded into UTF-8.
   *
   * @param curr In-out reference to character cursor.
   * @param end Pointer past the end of the input buffer.
   * @param delims String view of delimiter characters that terminate reading.
   * @param inString True if parsing inside a string literal.
   * @return Decoded string contents.
   */
  static std::string readUntil(
      stringpos& curr, stringpos end, std::string_view delims, bool inString = false) noexcept {
    std::string text;
    while (curr < end) {
      auto ch = *curr++;
      if (ch == '\\') {
        if (curr == end) {
          break;
        }
        ch = *curr++;
        switch (ch) {
          case 't':
            text += '\t';
            break;
          case 'r':
            text += '\r';
            break;
          case 'n':
            text += '\n';
            break;
          case 'u': {
            char32_t chVal = 0;
            for (auto i = 0; i < 4; i++) {
              ch = *curr++;
              if (ch >= '0' && ch <= '9') {
                chVal = chVal * 16 + (ch - '0');
              } else if (ch >= 'a' && ch <= 'f') {
                chVal = chVal * 16 + (ch - 'a');
              } else if (ch >= 'A' && ch <= 'F') {
                chVal = chVal * 16 + (ch - 'A');
              } else {
                curr = end;
                return "";
              }
            }
            text += Utf32Char(chVal).string();
          } break;
          default:
            text += ch;
            break;
        }
      } else if (delims.find(ch) != delims.npos) {
        --curr;
        break;
      } else {
        text += ch;
      }
    }

    return text;
  }
};
