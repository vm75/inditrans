#pragma once

#include "contextual_handlers.h"
#include "inditrans.h"
#include "script_constants.h"
#include "static_script_types.h"
#include "token_stream.h"

namespace inditrans::handlers {

/// Groups tokens for Latin transliteration schemes (ITRANS, IAST, SLP1, Harvard-Kyoto, etc.):
/// attaches subsequent vowels or vowel marks to a lead consonant.
/// If a consonant is not followed by a vowel, an explicit virama is synthesized.
class LatinReaderHandler {
public:
  template <typename Stream>
  static TokenUnit assemble(const ScriptToken& start, Stream& stream) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool vowelAdded = false;

      while (true) {
        const auto* peek = stream.peek();
        if (!peek || !HoldsScriptToken(*peek))
          break;
        const auto nextToken = GetScriptToken(*peek);
        bool consume = false;
        switch (nextToken.tokenType) {
          case TokenType::Vowel:
          case TokenType::VowelMark:
            if (!vowelAdded) {
              vowelAdded = true;
              // Roman vowel index 0 is 'a'; leaving the mark absent represents
              // the inherent vowel. The same index in VowelMark means virama.
              if (nextToken.idx != Diacritic_Virama) {
                tokenUnit.vowelMark = { TokenType::VowelMark, nextToken.idx };
              }
              consume = true;
            }
            break;
          case TokenType::OtherDiacritic:
            tokenUnit.otherDiacritic = nextToken;
            consume = true;
            break;
          case TokenType::Accent:
            tokenUnit.accent = nextToken;
            consume = true;
            break;
          default:
            break;
        }
        if (!consume) {
          break;
        }
        stream.advance();
      }

      if (!vowelAdded) {
        tokenUnit.vowelMark = { TokenType::VowelMark, 0 };
      }
    }
    return tokenUnit;
  }
};

/// Writes a TokenUnit in Latin transliteration:
/// an absent vowel mark emits the inherent 'a'; an explicit virama emits no vowel.
/// Assimilates anuswara if no explicit diacritic character is mapped.
class LatinWriterHandler {
public:
  template <typename PushFn>
  static void write(const TokenUnit& tokenUnit, const TokenUnitOrString& next,
      const ScriptWriterMap& map, TranslitOptions options, PushFn&& push) noexcept {
    const auto& leadToken = tokenUnit.leadToken;
    push(map.lookupChar(leadToken.tokenType, leadToken.idx));
    if (leadToken.tokenType == TokenType::Consonant) {
      if (tokenUnit.vowelMark.idx == InvalidToken) {
        push(map.lookupChar(TokenType::Vowel, Diacritic_Virama));
      } else if (tokenUnit.vowelMark.idx != Diacritic_Virama) {
        push(map.lookupChar(tokenUnit.vowelMark));
      }
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      push(map.lookupChar(tokenUnit.accent));
    }
    if (tokenUnit.otherDiacritic.idx != InvalidToken) {
      const auto lookup = map.lookupChar(tokenUnit.otherDiacritic);
      if (tokenUnit.otherDiacritic.idx == Diacritic_Anuswara && lookup.empty()) {
        AnuswaraAssimilator::emit<ScriptType::Latin>(next, map, push);
      } else {
        push(lookup);
      }
    }
  }
};

} // namespace inditrans::handlers
