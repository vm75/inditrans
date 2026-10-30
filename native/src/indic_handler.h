#pragma once

#include "contextual_handlers.h"
#include "inditrans.h"
#include "script_constants.h"
#include "static_script_types.h"
#include "token_stream.h"

namespace inditrans::handlers {

/// Groups tokens for Indic Brahmic scripts (Devanagari, Bengali, Telugu, Kannada, etc.):
/// attaches dependent vowel marks (matras), virama, diacritics (anuswara/visarga),
/// and Vedic accents to the initiating consonant or independent vowel.
class IndicReaderHandler {
public:
  template <typename Stream>
  [[gnu::always_inline]] static inline TokenUnit assemble(const ScriptToken& start, Stream& stream) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      while (true) {
        stream.ensure(1);
        if (stream.head >= stream.bufferSize || !HoldsScriptToken(stream.buffer[stream.head]))
          break;
        const auto nextToken = GetScriptToken(stream.buffer[stream.head]);
        switch (nextToken.tokenType) {
          case TokenType::OtherDiacritic:
            if (tokenUnit.otherDiacritic.idx != InvalidToken) return tokenUnit;
            tokenUnit.otherDiacritic = nextToken;
            break;
          case TokenType::VowelMark:
            if (tokenUnit.vowelMark.idx != InvalidToken) return tokenUnit;
            tokenUnit.vowelMark = nextToken;
            break;
          case TokenType::Accent:
            if (tokenUnit.accent.idx != InvalidToken) return tokenUnit;
            tokenUnit.accent = nextToken;
            break;
          default:
            return tokenUnit;
        }
        stream.head++;
      }
    } else if (start.tokenType == TokenType::Vowel) {
      while (true) {
        stream.ensure(1);
        if (stream.head >= stream.bufferSize || !HoldsScriptToken(stream.buffer[stream.head]))
          break;
        const auto nextToken = GetScriptToken(stream.buffer[stream.head]);
        switch (nextToken.tokenType) {
          case TokenType::OtherDiacritic:
            if (tokenUnit.otherDiacritic.idx != InvalidToken) return tokenUnit;
            tokenUnit.otherDiacritic = nextToken;
            break;
          case TokenType::Accent:
            if (tokenUnit.accent.idx != InvalidToken) return tokenUnit;
            tokenUnit.accent = nextToken;
            break;
          default:
            return tokenUnit;
        }
        stream.head++;
      }
    }
    return tokenUnit;
  }
};

/// Writes a TokenUnit in standard Indic Brahmic scripts:
/// emits base consonant or vowel, attached matras, diacritics, and Vedic pitch accents.
class IndicWriterHandler {
public:
  template <typename Writer>
  [[gnu::always_inline]] static inline void write(const TokenUnit& tokenUnit, const ScriptWriterMap& map,
      TranslitOptions options, Writer& writer) noexcept {
    auto lead = tokenUnit.leadToken;
    auto mark = tokenUnit.vowelMark;

    if (map.isDevanagari() && (options * TranslitOptions::DevanagariTraditional)) {
      if (lead.tokenType == TokenType::Vowel) {
        if (lead.idx == 10) lead.idx = 11;
        else if (lead.idx == 13) lead.idx = 14;
      } else if (lead.tokenType == TokenType::Consonant) {
        if (lead.idx == 34) lead.idx = 33;
        else if (lead.idx == 35) lead.idx = 26;
        else if (lead.idx == 36) lead.idx = 19;
      }
      if (mark.idx == 10) mark.idx = 11;
      else if (mark.idx == 13) mark.idx = 14;
    }

    if (AsciiNumeralFormatter::matches(lead, options)) {
      AsciiNumeralFormatter::emit(lead.idx, writer);
    } else {
      writer.push(map.lookupChar(lead));
    }
    if (mark.idx != InvalidToken) {
      writer.push(map.lookupChar(mark));
    }
    if (tokenUnit.otherDiacritic.idx != InvalidToken) {
      writer.push(map.lookupChar(tokenUnit.otherDiacritic));
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      writer.push(map.lookupChar(tokenUnit.accent));
    }
  }
};

} // namespace inditrans::handlers
