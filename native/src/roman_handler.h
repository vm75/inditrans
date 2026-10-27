#pragma once

#include "contextual_handlers.h"
#include "inditrans.h"
#include "script_constants.h"
#include "static_script_types.h"
#include "token_stream.h"

namespace inditrans::handlers {

/// Groups tokens for Roman transliteration schemes (ITRANS, IAST, SLP1, Harvard-Kyoto, etc.):
/// attaches subsequent vowels or vowel marks to a lead consonant.
/// If a consonant is not followed by a vowel, an explicit virama is synthesized.
class RomanReaderHandler {
public:
  template <typename Stream>
  [[gnu::always_inline]] static inline TokenUnit assemble(const ScriptToken& start, Stream& stream) noexcept {
    TokenUnit tokenUnit = { start };
    if (start.tokenType == TokenType::Consonant) {
      bool vowelAdded = false;

      while (true) {
        stream.ensure(1);
        if (stream.head >= stream.bufferSize || !HoldsScriptToken(stream.buffer[stream.head]))
          break;
        const auto nextToken = GetScriptToken(stream.buffer[stream.head]);
        bool consume = false;
        switch (nextToken.tokenType) {
          case TokenType::Vowel:
          case TokenType::VowelMark:
            if (!vowelAdded) {
              vowelAdded = true;
              // Roman vowel index 0 is 'a'; leaving the mark absent represents
              // the inherent vowel. The same index in VowelMark means virama.
              if (nextToken.tokenType == TokenType::VowelMark || nextToken.idx != Diacritic_Virama) {
                tokenUnit.vowelMark = { TokenType::VowelMark, nextToken.idx };
              }
              consume = true;
            }
            break;
          case TokenType::OtherDiacritic:
            if (tokenUnit.otherDiacritic.idx != InvalidToken) break;
            tokenUnit.otherDiacritic = nextToken;
            consume = true;
            break;
          case TokenType::Accent:
            if (tokenUnit.accent.idx != InvalidToken) break;
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

/// Writes a TokenUnit in Roman transliteration:
/// an absent vowel mark emits the inherent 'a'; an explicit virama emits no vowel.
/// Assimilates anuswara if no explicit diacritic character is mapped.
class RomanWriterHandler {
public:
  template <typename Writer>
  [[gnu::always_inline]] static inline void write(const TokenUnit& tokenUnit, const TokenUnitOrString& next,
      const ScriptWriterMap& map, TranslitOptions options, Writer& writer) noexcept {
    const auto& leadToken = tokenUnit.leadToken;
    writer.push(map.lookupChar(leadToken.tokenType, leadToken.idx));
    if (leadToken.tokenType == TokenType::Consonant) {
      if (tokenUnit.vowelMark.idx == InvalidToken) {
        writer.push(map.lookupChar(TokenType::Vowel, Diacritic_Virama));
      } else if (tokenUnit.vowelMark.idx != Diacritic_Virama) {
        writer.push(map.lookupChar(tokenUnit.vowelMark));
      }
    }
    if (tokenUnit.accent.idx != InvalidToken && options / TranslitOptions::IgnoreVedicAccents) {
      writer.push(map.lookupChar(tokenUnit.accent));
    }
    if (tokenUnit.otherDiacritic.idx != InvalidToken) {
      const auto lookup = map.lookupChar(tokenUnit.otherDiacritic);
      if (isAnuswara(tokenUnit.otherDiacritic) && (options / TranslitOptions::DisableInference)) {
        if (map.isIndlish()) {
          if (AnuswaraAssimilator::canAssimilate(next)) {
            AnuswaraAssimilator::emit<ScriptType::Roman>(next, map, writer);
          } else {
            writer.push(lookup);
          }
        } else if (lookup.empty()) {
          AnuswaraAssimilator::emit<ScriptType::Roman>(next, map, writer);
        } else {
          writer.push(lookup);
        }
      } else {
        writer.push(lookup);
      }
    }
  }
};

} // namespace inditrans::handlers
