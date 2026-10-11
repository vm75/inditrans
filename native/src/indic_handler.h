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
            tokenUnit.otherDiacritic = nextToken;
            break;
          case TokenType::VowelMark:
            tokenUnit.vowelMark = nextToken;
            break;
          case TokenType::Accent:
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
            tokenUnit.otherDiacritic = nextToken;
            break;
          case TokenType::Accent:
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
    if (AsciiNumeralFormatter::matches(tokenUnit.leadToken, options)) {
      AsciiNumeralFormatter::emit(tokenUnit.leadToken.idx, writer);
    } else {
      writer.push(map.lookupChar(tokenUnit.leadToken));
    }
    if (tokenUnit.vowelMark.idx != InvalidToken) {
      writer.push(map.lookupChar(tokenUnit.vowelMark));
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
