#include "inditrans.h"
#include "contextual_handlers.h"
#include "gurmukhi_handler.h"
#include "indic_handler.h"
#include "latin_handler.h"
#include "preservation_policy.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "tamil_handler.h"
#include "token_stream.h"
#include "type_defs.h"
#include "utf.h"
#include <array>
#include <bit>
#include <cassert>
#include <limits>
#include <string>
#include <type_traits>

using namespace inditrans;

using InditransLogger = void(const std::string&);
InditransLogger* inditransLogger = nullptr;

/// Streaming input tokenizer that scans UTF-8 text on demand and groups tokens into TokenUnits.
///
/// Operates with a bounded 16-element stack-allocated buffer (MaxSequence + Lookahead) without
/// dynamic allocation, ring-buffer arithmetic, or array shifting.
class InputReader {
public:
  InputReader(const std::string_view& input, const ScriptReaderMap& map, const TranslitOptions& options,
      const std::string_view& skipStart, const std::string_view& skipEnd) noexcept
      : ptr(input.data())
      , end(input.data() + input.length())
      , map(map)
      , skipStart(skipStart.empty() ? "##" : skipStart)
      , skipEnd(skipEnd.empty() ? "##" : skipEnd)
      , options(options) {
    if (map.nonRoman) {
      if (map.source)
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::Explicit>(); };
      else
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::Indic>(); };
    } else {
      if (map.folded)
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::FoldedRoman>(); };
      else
        policyFn = [](InputReader& reader) { reader.pull<ReaderPolicy::Roman>(); };
    }
  }

private:
  const char* ptr;
  const char* end;
  const ScriptReaderMap& map;
  const std::string_view skipStart;
  const std::string_view skipEnd;
  const TranslitOptions options;
  // Encoded sequences contain at most 15 tokens; the reader needs one
  // lookahead token. Leave room for a full sequence before each refill.
  static constexpr size_t MaxSequence = (1u << (16 - inditrans::static_data::sequenceOffsetBits)) - 1;
  static constexpr size_t Lookahead = 1;
  static constexpr size_t BufferCapacity = MaxSequence + Lookahead;
  static_assert(BufferCapacity >= MaxSequence + Lookahead);
  std::array<TokenOrString, BufferCapacity> buffer;
  size_t bufferSize = 0;
  size_t head = 0;
  void (*policyFn)(InputReader&);

  /// Ensures that at least @p requiredSize tokens/strings are available in the lookahead buffer.
  /// Refills only when the current expansion has been consumed, avoiding circular shifting.
  void ensure(size_t requiredSize) {
    if (bufferSize - head >= requiredSize)
      return;
    assert(requiredSize <= Lookahead);
    // All live calls request one token. Refill only after consuming the
    // current expansion, so no shifting or ring-buffer arithmetic is needed.
    bufferSize = 0;
    head = 0;
    while (bufferSize - head < requiredSize && ptr < end) {
      policyFn(*this);
    }
  }

  /// Pulls the next longest token match using the configured Policy, appending extracted
  /// ScriptTokens into the buffer, or delegates to PreservedSpanHandler on a miss.
  template <ReaderPolicy Policy> void pull() noexcept {
    auto match = map.lookupToken<Policy>(ptr, end);
    if (match.sequence) {
      const auto tokens = match.tokens();
      if (tokens.front().tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
        assert(bufferSize + tokens.size() <= BufferCapacity);
        buffer[bufferSize++] = TokenOrString(tokens.front());
        for (size_t i = 1; i < tokens.size(); ++i)
          buffer[bufferSize++] = TokenOrString(tokens[i]);
      }
      ptr += match.matchLen;
    } else {
      ptr = handlers::PreservedSpanHandler::pullUnrecognized<Policy>(
          ptr, end, map, skipStart, skipEnd, options,
          [this](std::string_view span) { buffer[bufferSize++] = TokenOrString(span); });
    }
  }

public:
  /// Returns true if more tokens or raw string spans are available in the input stream.
  inline bool hasMore() noexcept {
    ensure(1);
    return head < bufferSize;
  }

  TokenUnit lastToken = invalidTokenUnit;

  /// Stream adapter providing zero-allocation token lookahead and boundary checks to handlers.
  struct StreamAdaptor {
    InputReader& reader;

    const TokenOrString* peek() noexcept {
      reader.ensure(1);
      if (reader.head >= reader.bufferSize)
        return nullptr;
      return &reader.buffer[reader.head];
    }

    void advance() noexcept { reader.head++; }

    bool isEndOfWord() noexcept {
      reader.ensure(1);
      if (reader.head >= reader.bufferSize) {
        return true;
      }
      const auto& next = reader.buffer[reader.head];
      if (HoldsString(next)) {
        return true;
      }
      const auto token = GetScriptToken(next);
      return token.tokenType == TokenType::Symbol;
    }
  };

  /// Consumes the next token or string run and groups it into a complete TokenUnit cluster.
  /// Delegates script-specific assembly to isolated handlers.
  TokenUnitOrString getNext() noexcept {
    ensure(1);
    const auto& next = buffer[head++];
    if (HoldsString(next)) {
      wordStart = true;
      tamilReader.onWordBoundary();
      return GetString(next);
    }

    const auto& token = GetScriptToken(next);
    StreamAdaptor stream { *this };

    if (handlers::GurmukhiAdhakHandler::isAdhak(token)) {
      return handlers::GurmukhiAdhakHandler::handleAdhak([&stream]() { return stream.peek(); });
    }
    if (token == Skip) {
      return invalidTokenUnit;
    }

    TokenUnit tokenUnit = { token };
    switch (token.scriptType) {
      case ScriptType::Indic:
        tokenUnit = handlers::IndicReaderHandler::assemble(token, stream);
        break;
      case ScriptType::Tamil:
        tokenUnit = tamilReader.assemble(token, stream, wordStart, lastToken, options);
        break;
      case ScriptType::Latin:
        tokenUnit = handlers::LatinReaderHandler::assemble(token, stream);
        break;
      default:
        break;
    }
    wordStart = (token.tokenType == TokenType::Symbol);
    if (wordStart) {
      lastToken = invalidTokenUnit;
    } else {
      lastToken = tokenUnit;
    }
    return tokenUnit;
  }

private:
  bool wordStart { true };
  handlers::TamilReaderHandler tamilReader { };
};

/// Output generator specialized by ScriptType (Indic, Tamil, Latin) and output Sink.
///
/// Converts TokenUnit objects into target graphemes using the compiled ScriptWriterMap,
/// delegating orthographic rules, accent stripping, marker filtering, and vowel inferences
/// to compile-time selected handlers.
template <typename Sink, ScriptType Type> class OutputWriter {
public:
  OutputWriter(const ScriptWriterMap& map, const TranslitOptions options, Sink& sink) noexcept
      : map(map)
      , options(options)
      , buffer(sink) { }

  /// Writes a single TokenUnit cluster or raw string slice, with lookahead to the next token
  /// for contextual orthography rules (such as anuswara assimilation or end-of-word checks).
  void writeTokenUnit(const TokenUnitOrString& tokenUnitOrString, const TokenUnitOrString& next) noexcept {
    if (HoldsString(tokenUnitOrString)) {
      push(GetString(tokenUnitOrString));
      wordStart = true;
      if constexpr (Type == ScriptType::Tamil) {
        tamilWriter.onWordBoundary();
      }
    } else {
      const auto tokenUnit = GetTokenUnit(tokenUnitOrString);
      if (tokenUnit.leadToken.tokenType == TokenType::Ignore) {
        return;
      }
      auto pushFn = [this](std::string_view text) { push(text); };
      if constexpr (Type == ScriptType::Indic) {
        handlers::IndicWriterHandler::write(tokenUnit, map, options, pushFn);
      } else if constexpr (Type == ScriptType::Tamil) {
        tamilWriter.write(tokenUnit, next, map, options, pushFn, buffer, wordStart);
      } else if constexpr (Type == ScriptType::Latin) {
        handlers::LatinWriterHandler::write(tokenUnit, next, map, options, pushFn);
      }
      wordStart = (tokenUnit.leadToken.tokenType == TokenType::Symbol);
    }
  }

protected:
  /// Appends text to the destination sink, filtering out special disambiguation markers
  /// unless RetainSpecialMarkers is configured.
  inline void push(const std::string_view& text) {
    handlers::SpecialMarkerFilter::push(text, buffer, options);
  }

private:
  const ScriptWriterMap& map;
  const TranslitOptions options;
  Sink& buffer;
  bool wordStart { true };
  [[no_unique_address]] std::conditional_t<Type == ScriptType::Tamil,
      handlers::TamilWriterHandler, handlers::EmptyWriterHandler> tamilWriter { };
};

/// Calculates an initial capacity estimation for the destination output buffer.
/// When expanding Roman / Latin scripts into multi-byte Indic UTF-8 graphemes,
/// reserves a 3x byte multiplier hint to avoid recurring reallocations.
size_t outputCapacity(size_t bytes, const ScriptReaderMap& reader, const ScriptWriterMap& writer) noexcept {
  if (!reader.nonRoman && writer.getType() != ScriptType::Latin
      && bytes <= (std::numeric_limits<size_t>::max() - 1) / 3)
    return bytes * 3 + 1;
  return bytes + 1;
}

/// Typed transliteration driver: instantiates InputReader and OutputWriter specialized on Type,
/// iterating through TokenUnits with 1-unit lookahead and passing them to the writer.
template <typename Sink, ScriptType Type>
bool transliterate_typed(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  InputReader reader(input, readerMap, options, skipStart, skipEnd);

  if (writerMap.getType() == ScriptType::Indic && !writerMap.isVedic()) {
    // Keep source tokenization unchanged; only the writer suppresses accents
    // that the target script cannot represent.
    options = options | TranslitOptions::IgnoreVedicAccents;
  }
  OutputWriter<Sink, Type> writer(writerMap, options, sink);

  TokenUnitOrString curr = (reader.hasMore() ? reader.getNext() : endOfText);
  while (curr != endOfText) {
    TokenUnitOrString next = (reader.hasMore() ? reader.getNext() : endOfText);
    writer.writeTokenUnit(curr, next);
    curr = next;
  }

  return true;
}

/// Core transliteration dispatcher: routes execution to the appropriate OutputWriter specialization
/// based on the target script's ScriptType (Indic, Tamil, or Latin).
template <typename Sink>
bool transliterate_core(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  switch (writerMap.getType()) {
    case ScriptType::Indic:
      return transliterate_typed<Sink, ScriptType::Indic>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd);
    case ScriptType::Tamil:
      return transliterate_typed<Sink, ScriptType::Tamil>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd);
    case ScriptType::Latin:
      return transliterate_typed<Sink, ScriptType::Latin>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd);
    default:
      return false;
  }
}

/// Transliterates input into a heap-allocated TranslitBuffer via Utf8StringBuilder.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to)
    return false;
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap)
    return false;
  Utf8StringBuilder sink;
  sink.reserve(outputCapacity(input.length(), *readerMap, *writerMap));
  if (!transliterate_core(input, *readerMap, *writerMap, options, sink, skipStart, skipEnd)) {
    return false;
  }
  output.reset(sink.release());
  return true;
}

/// Zero-copy sink adaptor wrapping an existing std::string reference.
struct StdStringSink {
  std::string& str;
  StdStringSink(std::string& s)
      : str(s) { }
  void operator+=(char ch) { str += ch; }
  void operator+=(std::string_view view) { str += view; }
  size_t size() const { return str.size(); }
  Utf8Char back() const {
    auto len = UtfUtils::prevCharLen(str.data() + str.size());
    return Utf8Char(str.data() + str.size() - len);
  }
  void pop_back() {
    auto len = UtfUtils::prevCharLen(str.data() + str.size());
    str.resize(str.size() - len);
  }
};

/// Transliterates input directly into an existing std::string buffer.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::string& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to)
    return false;
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap)
    return false;
  // Append calls use the existing input-sized hint. Reserving extra prefix
  // capacity could invalidate a borrowed input view before the reader starts.
  output.reserve(output.empty() ? outputCapacity(input.length(), *readerMap, *writerMap) : input.length() + 1);
  StdStringSink sink(output);
  return transliterate_core(input, *readerMap, *writerMap, options, sink, skipStart, skipEnd);
}

/// Transliterates input and returns the result as a new std::string by value.
std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, const std::string_view& skipStart, const std::string_view& skipEnd) noexcept {
  std::string output;
  if (!transliterate(input, from, to, options, output, skipStart, skipEnd)) {
    return std::string();
  }

  return output;
}

extern "C" {

/// transliterate
char* CALL_CONV transliterate(const char* input, const char* from, const char* to, unsigned long options,
    const char* skipStart, const char* skipEnd) {
  TranslitBuffer output;
  std::string_view inputView(input);
  std::string_view skipStartView(skipStart);
  std::string_view skipEndView(skipEnd);
  if (!transliterate(inputView, from, to, static_cast<TranslitOptions>(options), output, skipStartView, skipEndView)) {
    return nullptr;
  } else {
    auto retval = output.release();
    return retval;
  }
}

/// Checks concrete script names and aliases against the compiled name table.
int CALL_CONV isScriptSupported(const char* script) { return inditrans::static_data::findScript(script) >= 0; }

/// releaseBuffer
void CALL_CONV releaseBuffer(char* buffer) { std::free(buffer); }

} // extern "C"
