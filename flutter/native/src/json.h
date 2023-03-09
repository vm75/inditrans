#include "utf.h"
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct JsonOject;
struct JsonArray;
using JsonValue = std::variant<void*, std::string, bool, int64_t, long double, JsonOject, JsonArray>;
struct JsonArray : public std::vector<JsonValue> { };
struct JsonOject : public std::vector<std::pair<std::string, JsonValue>> {
  template <typename T> std::optional<T> get(const std::string_view& key) const noexcept {
    for (const auto& entry : *this) {
      if (entry.first == key) {
        std::optional<T> res {};
        if (std::holds_alternative<T>(entry.second)) {
          res = std::get<T>(entry.second);
        }
        return res;
      }
    }
    return std::nullopt;
  }
};

class JsonReader {
public:
  template <typename T> static std::optional<JsonValue> parseJson(const T& str) noexcept {
    auto start = str.data(), end = start + str.length();
    auto val = parseValue(start, end);
    if (std::holds_alternative<std::string>(val)) {
      return std::nullopt;
    }
    return std::move(val);
  }

private:
  using stringpos = const char*;
  static JsonValue parseValue(stringpos& curr, stringpos end) noexcept {
    skipSpaces(curr, end);
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
          skipSpaces(curr, end);
          if (*curr != ',') {
            break;
          }
          curr++;
          skipSpaces(curr, end);
        }
        if (*curr == ']') {
          curr++;
          return arr;
        }
      } break;
      case '{': {
        curr++;
        JsonOject obj;
        skipSpaces(curr, end);
        while (curr < end && *curr != '}') {
          if (*curr != '"') {
            curr = end;
            return "";
          }
          curr++;
          auto key = readUntil(curr, end, "\"");
          if (*curr != '"') {
            curr = end;
            return "";
          }
          curr++;
          skipSpaces(curr, end);
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
          skipSpaces(curr, end);
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

  // Skip all chars until the delimitters (included)
  static char skipUntil(stringpos& curr, stringpos end, std::string_view delims) noexcept {
    while (curr < end) {
      auto ch = *curr++;
      if (delims.find(ch) != delims.npos) {
        return ch;
      }
    }

    return 0;
  }

  static void skipSpaces(stringpos& curr, stringpos end) noexcept {
    static constexpr std::string_view Spaces { " \t\n\r" };
    while (curr < end && Spaces.find(*curr) != Spaces.npos) {
      curr++;
    }
  }

  // Reads all chars until the delimitters (excluded).
  // If trim is true, skips leading whitespaces and trailing whitespaces and comma
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
