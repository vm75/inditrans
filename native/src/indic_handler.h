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
  static TokenUnit assemble(const ScriptToken& start, Stream& stream) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      while (true) {
        const auto* peek = stream.peek();
        if (!peek || !HoldsScriptToken(*peek))
          break;
        const auto nextToken = GetScriptToken(*peek);
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
        stream.advance();
      }
    } else if (start.tokenType == TokenType::Vowel) {
      while (true) {
        const auto* peek = stream.peek();
        if (!peek || !HoldsScriptToken(*peek))
          break;
        const auto nextToken = GetScriptToken(*peek);
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
        stream.advance();
      }
    }
    return tokenUnit;
  }
};

/// Writes a TokenUnit in standard Indic Brahmic scripts:
/// emits base consonant or vowel, attached matras, diacritics, and Vedic pitch accents.
class IndicWriterHandler {
public:
  template <typename PushFn>
  static void write(const TokenUnit& tokenUnit, const ScriptWriterMap& map,
      TranslitOptions options, PushFn&& push) noexcept {
    if (AsciiNumeralFormatter::matches(tokenUnit.leadToken, options)) {
      AsciiNumeralFormatter::emit(tokenUnit.leadToken.idx, push);
    } else {
      push(map.lookupChar(tokenUnit.leadToken));
    }
    if (tokenUnit.vowelMark.idx != InvalidToken) {
      push(map.lookupChar(tokenUnit.vowelMark));
    }
    if (tokenUnit.otherDiacritic.idx != InvalidToken) {
      push(map.lookupChar(tokenUnit.otherDiacritic));
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      push(map.lookupChar(tokenUnit.accent));
    }
  }
};

} // namespace inditrans::handlers
