#include "inditrans.h"
#include "contextual_handlers.h"
#include "gurmukhi_handler.h"
#include "indic_handler.h"
#include "indic_orthography.h"
#include "roman_handler.h"
#include "perso_arabic_handler.h"
#include "preservation_policy.h"
#include "script_constants.h"
#include "static_scripts.h"
#include "tamil_handler.h"
#include "token_stream.h"
#include "type_defs.h"
#include "utf.h"
#include "reader_extension.h"
#include "urdu_lexicon_pack.h"
#include "urdu_embedded_pack.h"
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
template <bool ExtendedReader, bool WithExtension = false> class InputReader {
public:
  InputReader(const std::string_view& input, const ScriptReaderMap& map, const TranslitOptions& options,
      const std::string_view& skipStart, const std::string_view& skipEnd, ReaderExtension* extension = nullptr) noexcept
      : ptr(input.data())
      , end(input.data() + input.length())
      , map(map)
      , skipStart(skipStart.empty() ? "##" : skipStart)
      , skipEnd(skipEnd.empty() ? "##" : skipEnd)
      , options(options)
      , extension(extension) {
    if (map.nonRoman) {
      if (map.source)
        policyFn = [](InputReader& reader) { reader.template pull<ReaderPolicy::Explicit>(); };
      else
        policyFn = [](InputReader& reader) { reader.template pull<ReaderPolicy::Indic>(); };
    } else {
      if (map.folded)
        policyFn = [](InputReader& reader) { reader.template pull<ReaderPolicy::FoldedRoman>(); };
      else
        policyFn = [](InputReader& reader) { reader.template pull<ReaderPolicy::Roman>(); };
    }
  }

private:
  const char* ptr;
  const char* end;
  const ScriptReaderMap& map;
  const std::string_view skipStart;
  const std::string_view skipEnd;
  const TranslitOptions options;
  ReaderExtension* extension;
  // Encoded sequences contain at most 15 tokens; the reader needs one
  // lookahead token. Leave room for a full sequence before each refill.
  static constexpr size_t MaxSequence = (1u << (16 - inditrans::static_data::sequenceOffsetBits)) - 1;
  static constexpr size_t Lookahead = 1;
  static constexpr size_t BufferCapacity = MaxSequence + Lookahead;
  static_assert(BufferCapacity >= MaxSequence + Lookahead);
  static_assert(BufferCapacity >= ReaderExtension::MaxTokens);
  std::array<TokenOrString, BufferCapacity> buffer;
  size_t bufferSize = 0;
  size_t head = 0;
  void (*policyFn)(InputReader&);

  /// Ensures that at least @p requiredSize tokens/strings are available in the lookahead buffer.
  /// Refills only when the current expansion has been consumed, avoiding circular shifting.
  [[gnu::always_inline]] inline void ensure(size_t requiredSize) {
    if (bufferSize - head >= requiredSize)
      return;
    assert(requiredSize <= Lookahead);
    // All live calls request one token. Refill only after consuming the
    // current expansion, so no shifting or ring-buffer arithmetic is needed.
    bufferSize = 0;
    head = 0;
    while (bufferSize - head < requiredSize && (ptr < end || (WithExtension && extension->hasPending(extension->state)))) {
      if constexpr (WithExtension) {
        if (extension->pull(extension->state, ptr, end, skipStart, skipEnd, options, buffer.data(), bufferSize))
          continue;
      }
      policyFn(*this);
    }
  }

  /// Pulls the next longest token match using the configured Policy, appending extracted
  /// ScriptTokens into the buffer, or delegates to PreservedSpanHandler on a miss.
  template <ReaderPolicy Policy> [[gnu::always_inline]] inline void pull() noexcept {
    auto match = map.lookupToken<Policy>(ptr, end);
    if (match.sequence) {
      const auto tokens = match.tokens();
      if (tokens.front().tokenType != TokenType::Accent || options / TranslitOptions::IgnoreVedicAccents) {
        assert(bufferSize + tokens.size() <= BufferCapacity);
        auto lead = tokens.front();
        if constexpr (Policy == ReaderPolicy::Indic) {
          // The virtual source needs Thai's preposed-vowel assembly too. Keep
          // this provenance on the copied token, never on the immutable table.
          if (end - ptr >= 3 && uint8_t(ptr[0]) == 0xe0
              && (uint8_t(ptr[1]) == 0xb8 || uint8_t(ptr[1]) == 0xb9))
            lead.reserved = 1;
        }
        buffer[bufferSize++] = TokenOrString(lead);
        for (size_t i = 1; i < tokens.size(); ++i)
          buffer[bufferSize++] = TokenOrString(tokens[i]);
      }
      ptr += match.matchLen;
    } else {
      ptr = handlers::PreservedSpanHandler::pullUnrecognized<Policy, TokenOrString>(
          ptr, end, map, skipStart, skipEnd, options, buffer.data(), bufferSize);
    }
  }

public:
  /// Returns true if more tokens or raw string spans are available in the input stream.
  inline bool hasMore() noexcept {
    ensure(1);
    if constexpr (ExtendedReader)
      return head < bufferSize || persoArabicReader.hasPending();
    return head < bufferSize;
  }

  [[gnu::always_inline]] inline bool isEndOfWord() noexcept {
    ensure(1);
    if (head >= bufferSize) {
      return true;
    }
    const auto& next = buffer[head];
    if (HoldsString(next)) {
      return true;
    }
    const auto token = GetScriptToken(next);
    return token.tokenType == TokenType::Symbol;
  }

  [[gnu::always_inline]] inline const TokenOrString* peek() noexcept {
    ensure(1);
    return head < bufferSize ? &buffer[head] : nullptr;
  }

  [[gnu::always_inline]] inline void advance(size_t bytes = 0) noexcept {
    if (bytes == 0) {
      head++;
      return;
    }
    if (head < bufferSize && HoldsString(buffer[head])) {
      const auto str = GetString(buffer[head]);
      if (bytes >= str.size()) {
        head++;
      } else {
        buffer[head] = TokenOrString(str.substr(bytes));
      }
    } else {
      head++;
    }
  }

  TokenUnit lastToken = invalidTokenUnit;
  [[no_unique_address]] std::conditional_t<ExtendedReader,
      handlers::PersoArabicReaderHandler, handlers::EmptyWriterHandler> persoArabicReader { };
  uint8_t thaiVowel { InvalidToken };

  friend class handlers::IndicReaderHandler;
  friend class handlers::RomanReaderHandler;
  friend class handlers::TamilReaderHandler;

public:
  /// Consumes the next token or string run and groups it into a complete TokenUnit cluster.
  /// Delegates script-specific assembly to isolated handlers.
  TokenUnitOrString getNext() noexcept {
    if constexpr (ExtendedReader) {
      if (persoArabicReader.hasPending()) [[unlikely]]
        return persoArabicReader.takePending();
    }
  readToken:
    ensure(1);
    const auto& next = buffer[head++];
    if (HoldsString(next)) {
      wordStart = true;
      if constexpr (ExtendedReader)
        thaiVowel = InvalidToken;
      return GetString(next);
    }

    const auto token = GetScriptToken(next);
    const bool thai = ExtendedReader && (token.reserved == 1
        || map.source == inditrans::static_data::scriptMask(inditrans::static_data::ScriptId::Thai));
    if constexpr (ExtendedReader) {
      if (thai && token.tokenType == TokenType::VowelMark
          && (token.idx == 11 || token.idx == 12 || token.idx == 14 || token.idx == 17)) [[unlikely]] {
        const auto* following = peek();
        if (following && HoldsScriptToken(*following) && GetScriptToken(*following).tokenType == TokenType::Consonant) {
          thaiVowel = token.idx;
          goto readToken;
        }
      }
    }

    if (handlers::GurmukhiAdhakHandler::isAdhak(token)) {
      return handlers::GurmukhiAdhakHandler::handleAdhak([this]() -> const TokenOrString* {
        ensure(1);
        if (head >= bufferSize) return nullptr;
        return &buffer[head];
      });
    }
    if (token == Skip) {
      return invalidTokenUnit;
    }

    TokenUnit tokenUnit = { token };
    switch (token.scriptType) {
      case ScriptType::Indic:
        tokenUnit = handlers::IndicReaderHandler::assemble(token, *this);
        break;
      case ScriptType::Tamil:
        tokenUnit = tamilReader.assemble(token, *this, wordStart, lastToken, options);
        break;
      case ScriptType::Roman:
        tokenUnit = handlers::RomanReaderHandler::assemble(token, *this);
        break;
      case ScriptType::PersoArabic:
        if constexpr (ExtendedReader)
          tokenUnit = persoArabicReader.assemble(token, *this);
        break;
      default:
        break;
    }
    if constexpr (ExtendedReader) {
      if (thai && thaiVowel == InvalidToken && token.tokenType == TokenType::Consonant && tokenUnit.vowelMark.idx == 18) [[unlikely]] {
        // Without a preposed e, the suffix aa + sara a is long aa + visarga.
        tokenUnit.vowelMark.idx = 1;
        tokenUnit.otherDiacritic = { TokenType::OtherDiacritic, 2 };
      }
      if (thai && thaiVowel != InvalidToken && token.tokenType == TokenType::Consonant && tokenUnit.vowelMark.idx != 0) [[unlikely]] {
        auto vowel = thaiVowel == 17 ? uint8_t(21) : thaiVowel;
        const bool shortens = tokenUnit.otherDiacritic.idx == 2 && thaiVowel != 12;
        const auto suffix = shortens ? 10 : tokenUnit.vowelMark.idx;
        if (suffix == 10) vowel = thaiVowel == 14 ? 13 : thaiVowel == 17 ? 19 : 10;
        else if (thaiVowel == 11 && suffix == 1) vowel = 15;
        else if (thaiVowel == 11 && suffix == 18) vowel = 18;
        tokenUnit.vowelMark = { TokenType::VowelMark, vowel };
        if (shortens) tokenUnit.otherDiacritic = {};
        thaiVowel = InvalidToken;
      }
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

/// Output generator specialized by ScriptType (Indic, Tamil, Roman) and output Sink.
///
/// Converts TokenUnit objects into target graphemes using the compiled ScriptWriterMap,
/// delegating orthographic rules, accent stripping, marker filtering, and vowel inferences
/// to compile-time selected handlers.
template <typename Sink, ScriptType Type, bool ContextualIndic = false> class OutputWriter {
public:
  OutputWriter(const ScriptWriterMap& map, const TranslitOptions options, Sink& sink) noexcept
      : map(map)
      , options(options)
      , buffer(sink) { }

  /// Writes a single TokenUnit cluster or raw string slice, with lookahead to the next token
  /// for contextual orthography rules (such as anuswara assimilation or end-of-word checks).
  [[gnu::always_inline]] inline void writeTokenUnit(const TokenUnitOrString& tokenUnitOrString, const TokenUnitOrString& next) noexcept {
    if (HoldsString(tokenUnitOrString)) {
      // Literal/protected/XML spans are already source text, not writer notation.
      buffer += GetString(tokenUnitOrString);
      wordStart = true;
      if constexpr (Type == ScriptType::Tamil) {
        tamilWriter.onWordBoundary();
      } else if constexpr (Type == ScriptType::Indic && ContextualIndic) {
        indicWriter.onWordBoundary();
      }
    } else {
      const auto tokenUnit = GetTokenUnit(tokenUnitOrString);
      if (tokenUnit.leadToken.tokenType == TokenType::Ignore) {
        return;
      }
      if constexpr (Type == ScriptType::Indic) {
        if constexpr (ContextualIndic)
          indicWriter.write(tokenUnit, next, map, options, *this, buffer);
        else
          handlers::IndicWriterHandler::write(tokenUnit, map, options, *this);
      } else if constexpr (Type == ScriptType::Tamil) {
        tamilWriter.write(tokenUnit, next, map, options, *this, buffer, wordStart);
      } else if constexpr (Type == ScriptType::Roman) {
        handlers::RomanWriterHandler::write(tokenUnit, next, map, options, *this);
      } else if constexpr (Type == ScriptType::PersoArabic) {
        persoArabicWriter.write(tokenUnit, next, map, options, *this, wordStart);
      }
      wordStart = (tokenUnit.leadToken.tokenType == TokenType::Symbol);
    }
  }

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
  [[no_unique_address]] std::conditional_t<Type == ScriptType::PersoArabic,
      handlers::PersoArabicWriterHandler, handlers::EmptyWriterHandler> persoArabicWriter { };
  [[no_unique_address]] std::conditional_t<Type == ScriptType::Indic && ContextualIndic,
      handlers::IndicOrthographyHandler, handlers::EmptyWriterHandler> indicWriter { };
};

/// Calculates an initial capacity estimation for the destination output buffer.
/// When expanding Roman schemes into multi-byte Indic UTF-8 graphemes,
/// reserves a 3x byte multiplier hint to avoid recurring reallocations.
size_t outputCapacity(size_t bytes, const ScriptReaderMap& reader, const ScriptWriterMap& writer) noexcept {
  if (!reader.nonRoman && writer.getType() != ScriptType::Roman
      && bytes <= (std::numeric_limits<size_t>::max() - 1) / 3)
    return bytes * 3 + 1;
  return bytes + 1;
}

/// Typed transliteration driver: instantiates InputReader and OutputWriter specialized on Type,
/// iterating through TokenUnits with 1-unit lookahead and passing them to the writer.
template <typename Sink, ScriptType Type, bool ContextualIndic = false, bool ExtendedReader = true, bool WithExtension = false>
bool transliterate_typed(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd, ReaderExtension* extension = nullptr) noexcept {
  InputReader<ExtendedReader, WithExtension> reader(input, readerMap, options, skipStart, skipEnd, extension);

  if (!writerMap.supportsVedicAccents()) {
    // Keep source tokenization unchanged; only the writer suppresses accents
    // that the target script cannot represent.
    options = options | TranslitOptions::IgnoreVedicAccents;
  }
  OutputWriter<Sink, Type, ContextualIndic> writer(writerMap, options, sink);

  TokenUnitOrString curr = (reader.hasMore() ? reader.getNext() : endOfText);
  while (curr != endOfText) {
    TokenUnitOrString next = (reader.hasMore() ? reader.getNext() : endOfText);
    writer.writeTokenUnit(curr, next);
    curr = next;
  }

  return true;
}

/// Core transliteration dispatcher: routes execution to the appropriate OutputWriter specialization
/// based on the target script's ScriptType (Indic, Tamil, or Roman).
template <typename Sink, bool ExtendedReader, bool WithExtension = false>
bool transliterate_dispatch(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd, ReaderExtension* extension = nullptr) noexcept {
  switch (writerMap.getType()) {
    case ScriptType::Indic:
      if (writerMap.orthography != IndicOrthography::Plain)
        return transliterate_typed<Sink, ScriptType::Indic, true, ExtendedReader, WithExtension>(
            input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
      return transliterate_typed<Sink, ScriptType::Indic, false, ExtendedReader, WithExtension>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
    case ScriptType::Tamil:
      return transliterate_typed<Sink, ScriptType::Tamil, false, ExtendedReader, WithExtension>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
    case ScriptType::Roman:
      return transliterate_typed<Sink, ScriptType::Roman, false, ExtendedReader, WithExtension>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
    case ScriptType::PersoArabic:
      return transliterate_typed<Sink, ScriptType::PersoArabic, false, ExtendedReader, WithExtension>(
          input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
    default:
      return false;
  }
}

/// Select reader capabilities once per call. Explicit non-Thai/non-Urdu sources
/// cannot produce preposed Thai vowels or pending Urdu shadda units.
template <typename Sink, bool WithExtension = false>
bool transliterate_core(const std::string_view& input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink, const std::string_view& skipStart,
    const std::string_view& skipEnd, ReaderExtension* extension = nullptr) noexcept {
  namespace data = inditrans::static_data;
  constexpr auto extendedSources = data::scriptMask(data::ScriptId::Thai, data::ScriptId::Urdu);
  if (readerMap.nonRoman && (!readerMap.source || (readerMap.source & extendedSources)))
    return transliterate_dispatch<Sink, true, WithExtension>(input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
  return transliterate_dispatch<Sink, false, WithExtension>(input, readerMap, writerMap, options, sink, skipStart, skipEnd, extension);
}

/// For explicit Urdu input, use the compiled-in pronunciation dictionary
/// unless inference is explicitly disabled. The original literal parser is
/// preserved for unknown words and for the DisableInference option.
///
/// The compact IUL1 asset remains immutable in the binary. Its index is
/// initialized only on the first eligible Urdu conversion, once per process;
/// non-Urdu scripts never allocate or parse it.
template <typename Sink>
bool transliterateWithDefaultUrdu(std::string_view input, const ScriptReaderMap& readerMap,
    const ScriptWriterMap& writerMap, TranslitOptions options, Sink& sink,
    std::string_view skipStart, std::string_view skipEnd) noexcept {
  if (options / TranslitOptions::DisableInference
      && &readerMap == getScriptReaderMap("urdu")
      && &writerMap != getScriptWriterMap("urdu")) {
    static UrduLexiconPack pack;
    static const bool valid = pack.load(std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(inditrans::embedded_urdu::pack),
        static_cast<size_t>(inditrans::embedded_urdu::pack_size)));
    if (valid) {
      inditrans::handlers::UrduPronunciationReader reader(input, pack.lexicon());
      auto extension = ReaderExtension::bind(reader);
      return transliterate_core<Sink, true>(input, readerMap, writerMap,
          options, sink, skipStart, skipEnd, &extension);
    }
  }
  return transliterate_core(input, readerMap, writerMap, options, sink, skipStart, skipEnd);
}

/// When reading explicitly selected Tamil with superscripted disambiguation, do not
/// additionally infer pronunciation or contextual nasals. Do not apply this to an
/// 'indic' virtual source (which may mix Tamil and other scripts), or to output-only
/// TamilSuperscripted formatting.
static constexpr TranslitOptions effectiveInferenceOptions(std::string_view from, TranslitOptions options) noexcept {
  if (inditrans::static_data::compareScriptName(from, "tamil") == 0
      && options * TranslitOptions::TamilSuperscripted)
    return options | TranslitOptions::DisableInference;
  return options;
}

// Enforce source-vs-target semantics even when no observable difference exists
// for a destination script with an explicit anuswara mapping.
static_assert(effectiveInferenceOptions("tamil", TranslitOptions::TamilSuperscripted)
    * TranslitOptions::DisableInference);
static_assert(effectiveInferenceOptions("TAMIL", TranslitOptions::TamilSuperscripted)
    * TranslitOptions::DisableInference);
static_assert(effectiveInferenceOptions("devanagari", TranslitOptions::TamilSuperscripted)
    / TranslitOptions::DisableInference);
static_assert(effectiveInferenceOptions("indic", TranslitOptions::TamilSuperscripted)
    / TranslitOptions::DisableInference);
static_assert(effectiveInferenceOptions("tamil", TranslitOptions::None)
    / TranslitOptions::DisableInference);

/// Same-script copy: no script resolution, token lookup, inference, or writer dispatch.
template <typename Sink>
static void copyText(std::string_view input, TranslitOptions options, Sink& sink,
    std::string_view skipStart, std::string_view skipEnd) noexcept {
  if (skipStart.empty()) skipStart = "##";
  if (skipEnd.empty()) skipEnd = "##";
  while (!input.empty()) {
    if (options / TranslitOptions::NoXMLTagHandling && input.front() == '<') {
      const auto close = input.find('>');
      const auto length = close == input.npos ? input.size() : close + 1;
      sink += input.substr(0, length);
      input.remove_prefix(length);
    } else if (input.starts_with(skipStart)) {
      input.remove_prefix(skipStart.size());
      const auto close = input.find(skipEnd);
      // Match the transliteration path: an unmatched opener discards the remainder.
      if (close == input.npos) return;
      sink += input.substr(0, close);
      input.remove_prefix(close + skipEnd.size());
    } else {
      const auto length = std::min(UtfUtils::nextCharLen(input.data()), input.size());
      const auto ch = input.substr(0, length);
      const bool remove = static_cast<uint8_t>(ch.front()) >= 0x80
          && ((options / TranslitOptions::RetainSpecialMarkers && SpecialMarkers.find(ch) != SpecialMarkers.npos)
              || (options / TranslitOptions::TamilSuperscripted
                  && (TamilSuperscripts.find(ch) != TamilSuperscripts.npos
                      || TamilSubscripts.find(ch) != TamilSubscripts.npos))
              || (options * TranslitOptions::IgnoreVedicAccents
                  && static_data::copyAccents.find(ch) != static_data::copyAccents.npos));
      if (!remove) sink += ch;
      input.remove_prefix(length);
    }
  }
}

/// Transliterates input into a heap-allocated TranslitBuffer via Utf8StringBuilder.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to) {
    Utf8StringBuilder sink;
    sink.reserve(input.size() + 1);
    copyText(input, options, sink, skipStart, skipEnd);
    output.reset(sink.release());
    return true;
  }
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap)
    return false;
  Utf8StringBuilder sink;
  sink.reserve(outputCapacity(input.length(), *readerMap, *writerMap));
  if (!transliterateWithDefaultUrdu(input, *readerMap, *writerMap, effectiveInferenceOptions(from, options), sink, skipStart, skipEnd)) {
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
  void replace(size_t offset, size_t count, std::string_view text) { str.replace(offset, count, text); }
  Utf8Char back() const {
    auto len = UtfUtils::prevCharLen(str.data() + str.size());
    return Utf8Char(str.data() + str.size() - len);
  }
  void pop_back() {
    auto len = UtfUtils::prevCharLen(str.data() + str.size());
    str.resize(str.size() - len);
  }
};

bool transliterateWithReaderExtension(std::string_view input, std::string_view from, std::string_view to,
    TranslitOptions options, std::string& output, ReaderExtension& extension,
    std::string_view skipStart, std::string_view skipEnd) noexcept {
  if (from == to) return transliterate(input, from, to, options, output, skipStart, skipEnd);
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap) return false;
  output.reserve(output.empty() ? outputCapacity(input.length(), *readerMap, *writerMap) : input.length() + 1);
  StdStringSink sink(output);
  return transliterate_core<StdStringSink, true>(input, *readerMap, *writerMap,
      effectiveInferenceOptions(from, options), sink, skipStart, skipEnd, &extension);
}

/// Transliterates input directly into an existing std::string buffer.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::string& output, const std::string_view& skipStart,
    const std::string_view& skipEnd) noexcept {
  if (from == to) {
    StdStringSink sink(output);
    copyText(input, options, sink, skipStart, skipEnd);
    return true;
  }
  const auto* readerMap = getScriptReaderMap(from);
  const auto* writerMap = getScriptWriterMap(to);
  if (!readerMap || !writerMap)
    return false;
  // Append calls use the existing input-sized hint. Reserving extra prefix
  // capacity could invalidate a borrowed input view before the reader starts.
  output.reserve(output.empty() ? outputCapacity(input.length(), *readerMap, *writerMap) : input.length() + 1);
  StdStringSink sink(output);
  return transliterateWithDefaultUrdu(input, *readerMap, *writerMap, effectiveInferenceOptions(from, options), sink, skipStart, skipEnd);
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

#include "urdu_c_api.h"
