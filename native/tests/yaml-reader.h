#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <utf.h>
#include <variant>
#include <vector>

struct YamlObject;
struct YamlArray;
using YamlValue = std::variant<void*, std::string, bool, int64_t, long double, YamlObject, YamlArray>;
struct YamlArray : public std::vector<YamlValue> {
  void add(YamlValue&& val) noexcept;
};
struct YamlObject : public std::vector<std::pair<std::string, YamlValue>> {
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
  void add(const std::string_view& key, YamlValue&& val) noexcept;
};

YamlValue resolveValue(YamlValue&& val) noexcept {
  if (!std::holds_alternative<std::string>(val)) {
    return std::move(val);
  }

  const auto& str = std::get<std::string>(val);
  if (str == "true") {
    return true;
  } else if (str == "false") {
    return false;
  } else if (str == "null") {
    return nullptr;
  } else if (str == "NaN") {
    return std::numeric_limits<long double>::quiet_NaN();
  } else if (str == "Infinity") {
    return std::numeric_limits<long double>::infinity();
  } else if (str == "-Infinity") {
    return -std::numeric_limits<long double>::infinity();
  }
  // int regex
  std::smatch match {};
  static const std::regex intRegex { R"((^[-+]?[0-9]+$))" };
  if (std::regex_match(str, match, intRegex)) {
    return std::stoll(str);
  }
  // double regex
  static const std::regex doubleRegex { R"((^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$))" };
  if (std::regex_match(str, match, doubleRegex)) {
    return std::stold(str);
  }
  return std::move(val);
}

void YamlArray::add(YamlValue&& val) noexcept { emplace_back(resolveValue(std::move(val))); }

void YamlObject::add(const std::string_view& key, YamlValue&& val) noexcept {
  emplace_back(key, resolveValue(std::move(val)));
}

const char* beginOf(const std::string_view& str) noexcept { return str.data(); }
const char* endOf(const std::string_view& str) noexcept { return str.data() + str.length(); }

class YamlReader {
public:
  template <typename T> static std::vector<YamlValue> parse(const T& str) noexcept {
    std::vector<YamlValue> res;
    auto curr = str.data(), end = curr + str.length();

    while (curr < end) {
      skipEmptyLines(curr, end);
      if (std::string_view(curr, end).starts_with("---")) {
        curr += 3;
        skipEmptyLines(curr, end);
      }

      auto line = peekNextLine(curr, end);
      if (!line) {
        break;
      }
      if (!line->isListItem && line->contents.empty()) {
        curr = line->contents.data() + line->contents.length();
        continue;
      }
      if (line->isListItem) {
        res.push_back(parseArray(curr, end, *line));
      } else {
        res.push_back(parseObject(curr, end, *line));
      }
    }

    return res;
  }

private:
  using stringpos = const char*;
  struct LineInfo {
    int indent {};
    bool isListItem {};
    std::string_view contents {};

    bool empty() const noexcept { return !isListItem && contents.empty(); }
  };
  struct Line {
    int indent {};
    int indent2 {};
    bool isListItem {};
    std::string key {};
    std::string val {};

    bool empty() const noexcept { return !isListItem && key.empty() && val.empty(); }
  };

  static std::optional<YamlValue> parseValue(stringpos& curr, stringpos end, int parentIndent) noexcept {
    std::optional<Line> line {};
    while ((line = parseNextLine(curr, end)) != std::nullopt && line->empty())
      ;
    if (!line) {
      return std::nullopt; // error
    }

    if (line->indent <= parentIndent) {
      return std::nullopt; // error
    }

    if (line->isListItem) {
      YamlArray res {};
    }
    return std::nullopt;
  }

  static void skipTillEndOfLine(stringpos& curr, stringpos end) noexcept {
    while (curr < end && *curr != '\n') {
      curr++;
    }
  }

  static std::string_view trimEnd(stringpos start, stringpos end) noexcept {
    while (end > start && *(end - 1) == ' ') {
      end--;
    }
    return std::string_view(start, end - start);
  }

  static std::optional<std::string> parseSingleQuoted(stringpos& curr, stringpos end) noexcept {
    curr++; // skip '
    auto start = curr;
    std::string res {};
    while (curr < end) {
      if (*curr == '\'') {
        if (curr + 1 < end && *(curr + 1) == '\'') {
          curr++;
        } else {
          break;
        }
      } else if (*curr == '\n') {
        return std::nullopt; // error
      }
      res += *curr++;
    }
    if (curr == end || *curr != '\'') {
      return std::nullopt; // error
    }
    curr++; // skip '

    return res;
  }

  static std::optional<std::string> parseDoubleQuoted(stringpos& curr, stringpos end) noexcept {
    curr++; // skip "
    auto start = curr;
    std::string res {};
    while (curr < end && *curr != '"') {
      if (*curr == '\\') {
        curr++;
        if (curr == end) {
          return std::nullopt; // error
        }
        auto ch = *curr++;
        if (std::string_view("0turn").find(ch) == std::string::npos) {
          res += ch;
          continue;
        }
        if (ch == 't') {
          res += '\t';
        } else if (ch == 'n') {
          res += '\n';
        } else if (ch == 'r') {
          res += '\r';
        } else if (ch == 'u') {
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
          res += Utf32Char(chVal).string();
        } else if (ch == '0') {
          res += '\0';
        }
      } else if (*curr == '\n') {
        return std::nullopt; // error
      }
      res += *curr++;
    }
    if (curr == end || *curr != '"') {
      return std::nullopt; // error
    }
    curr++; // skip "

    return res;
  }

  static std::optional<std::string> parseVariable(
      stringpos& curr, stringpos end, bool isValue, int prevIndent) noexcept {
    int minIndent = isValue ? prevIndent + 1 : prevIndent;

    if (*curr == '\'') {
      return parseSingleQuoted(curr, end);
    } else if (*curr == '"') {
      return parseDoubleQuoted(curr, end);
    } else {
      auto varStart = curr;
      while (curr < end) {
        if (*curr == '\n') {
          break;
        }
        if (*curr == '#' && *(curr - 1) == ' ') {
          break;
        }
        if (!isValue && *curr == ':') {
          if (curr + 1 < end && *(curr + 1) != ' ' && *(curr + 1) != '\n') {
            curr++;
          } else {
            break;
          }
        }
        curr++;
      }
      std::string res { trimEnd(varStart, curr) };
      if (curr == end || *curr == ':') {
        return res;
      }
      if (isValue) {
        if (res == "|") {
          res.clear();
          appendString(curr, end, res, minIndent, AppendType::Lined);
        } else if (res == ">") {
          res.clear();
          appendString(curr, end, res, minIndent, AppendType::Spaced);
        }
      }

      return res;
    }
  }

  enum AppendType { Trimmed, Lined, Spaced };

  static void appendString(stringpos& curr, stringpos end, std::string& str, int minIndent, AppendType type) noexcept {
    std::optional<LineInfo> addnLine {};
    int prevIndent = 0;
    while (curr < end && *curr == '\n' && (addnLine = peekNextLine(curr + 1, end)) != std::nullopt) {
      if (addnLine->indent < minIndent) {
        break;
      }
      if (prevIndent == 0) {
        prevIndent = addnLine->indent;
      }
      if (!str.empty()) {
        if (type == AppendType::Lined) {
          str += "\n";
        } else {
          str += " ";
        }
      }
      if (type == AppendType::Trimmed) {
        str += trimEnd(beginOf(addnLine->contents), endOf(addnLine->contents));
      } else {
        str += trimEnd(curr + 1 + prevIndent, endOf(addnLine->contents));
      }
      curr = endOf(addnLine->contents);
    }
    if (type != AppendType::Trimmed) {
      str += "\n";
    }
  }

  static void skipSpaces(stringpos& curr, stringpos end) noexcept {
    while (curr < end && *curr == ' ') {
      curr++;
    }
    if (curr < end && *curr == '#') {
      skipTillEndOfLine(curr, end);
    }
  }

  static void skipEmptyLines(stringpos& curr, stringpos end) noexcept {
    static const std::regex emptyLineRegex { R"(^\s*(#.*)?\n)" };
    std::match_results<stringpos> match {};
    while (curr < end && std::regex_search(curr, end, match, emptyLineRegex)) {
      curr = match[0].second;
    }
  }

  static std::optional<Line> parseNextLine(stringpos& curr, stringpos end) noexcept {
    skipEmptyLines(curr, end);
    if (curr >= end) {
      return std::nullopt;
    }
    Line line {};

    if (end - curr >= 3 && *curr == '-' && *(curr + 1) == '-' && *(curr + 2) == '-') {
      curr += 3;
      skipTillEndOfLine(curr, end);
      line.key = "---";
      return line;
    }

    auto start = curr;
    skipSpaces(curr, end);
    if (curr == end || *curr == '\n') {
      return line; // empty line
    }
    line.indent = curr - start;
    line.isListItem = *curr == '-';
    if (line.isListItem) {
      curr++; // skip -
      skipSpaces(curr, end);
      if (curr == end || *curr == '\n') {
        return line;
      }
      line.indent2 = curr - start;
    }
    auto var = parseVariable(curr, end, false /* isValue */, line.indent);
    if (!var) {
      return std::nullopt; // error
    }
    line.key = *var;
    skipSpaces(curr, end);
    if (curr == end || *curr == '\n') {
      return line;
    }
    if (*curr != ':') {
      return std::nullopt; // error
    }
    curr++; // skip :
    skipSpaces(curr, end);
    var = parseVariable(curr, end, true /* isValue */, line.indent);
    if (!var) {
      return std::nullopt; // error
    }
    line.val = *var;
    skipSpaces(curr, end);
    if (curr < end && *curr != '\n') {
      return std::nullopt; // error
    }
    return line;
  }

  static std::optional<LineInfo> peekNextLine(stringpos curr, stringpos end) noexcept {
    skipEmptyLines(curr, end);
    if (curr >= end) {
      return std::nullopt;
    }
    LineInfo lineInfo {};
    auto start = curr;
    while (curr < end && *curr == ' ') {
      curr++;
    }
    lineInfo.indent = curr - start;

    if (curr < end && *curr == '-') {
      lineInfo.isListItem = true;
    }

    start = curr;
    while (curr < end && *curr != '\n') {
      curr++;
    }

    lineInfo.contents = std::string_view(start, curr - start);

    return lineInfo;
  }

  static YamlArray parseArray(stringpos& curr, stringpos end, std::optional<YamlReader::LineInfo> lineInfo) noexcept {
    YamlArray list {};

    int indent = lineInfo->indent;

    do {
      if (lineInfo->indent == 0 && lineInfo->contents == "---") {
        curr = endOf(lineInfo->contents);
        break;
      }
      auto line = parseNextLine(curr, end);
      if (!line) {
        break;
      }
      if (line->val.empty()) {
        auto next = peekNextLine(curr, end);
        if (next && next->indent > indent) {
          if (next->isListItem) {
            list.add(parseArray(curr, end, *next));
          } else {
            curr = beginOf(lineInfo->contents) - lineInfo->indent;
            lineInfo->indent = line->indent2;
            list.add(parseObject(curr, end, lineInfo));
          }
        } else {
          if (line->key.empty()) {
            list.add(nullptr);
          } else {
            list.add(line->key);
          }
        }
      } else {
        curr = beginOf(lineInfo->contents) - lineInfo->indent;
        lineInfo->indent = line->indent2;
        list.add(parseObject(curr, end, lineInfo));
      }
    } while ((lineInfo = peekNextLine(curr, end)) && lineInfo->isListItem && lineInfo->indent == indent);

    return list;
  }

  static YamlObject parseObject(stringpos& curr, stringpos end, std::optional<YamlReader::LineInfo> lineInfo) noexcept {
    YamlObject obj {};

    int indent = lineInfo->indent;

    do {
      if (lineInfo->indent == 0 && lineInfo->contents == "---") {
        curr = endOf(lineInfo->contents);
        break;
      }
      auto line = parseNextLine(curr, end);
      if (!line) {
        break;
      }
      if (line->val.empty()) {
        auto next = peekNextLine(curr, end);
        if (next && next->indent > indent) {
          if (next->isListItem) {
            obj.add(line->key, parseArray(curr, end, next));
          } else {
            obj.add(line->key, parseObject(curr, end, next));
          }
        } else {
          obj.add(line->key, YamlValue(nullptr));
        }
      } else {
        obj.add(line->key, YamlValue(line->val));
      }
    } while ((lineInfo = peekNextLine(curr, end)) && !lineInfo->isListItem && lineInfo->indent == indent);

    return obj;
  }
};
