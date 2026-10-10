#pragma once

#include "script_constants.h"
#include "token_stream.h"

namespace inditrans::handlers {

/// Handles the Gurmukhi adhak gemination symbol (ੱ):
/// peeks ahead at the following consonant and emits a halved version of it with virama
/// (e.g. adhak + k -> k + virama + k). Aspiration within the varga is stripped.
class GurmukhiAdhakHandler {
public:
  static constexpr bool isAdhak(const ScriptToken& token) noexcept {
    return token == GurmukhiAdhak;
  }

  /// Evaluates adhak against the next buffered item without consuming it.
  template <typename PeekFn>
  static TokenUnit handleAdhak(PeekFn&& peek) noexcept {
    const auto peekItem = peek();
    if (peekItem && HoldsScriptToken(*peekItem)) {
      const auto nextToken = GetScriptToken(*peekItem);
      if (nextToken.tokenType == TokenType::Consonant) {
        // Leave the following consonant buffered: this unit supplies only the
        // first half of the geminate, with aspiration removed within its varga.
        TokenUnit tokenUnit = { nextToken };
        if (tokenUnit.leadToken.idx < 24) {
          tokenUnit.leadToken.idx -= tokenUnit.leadToken.idx % 5 % 2;
        }
        tokenUnit.vowelMark = Virama;
        return tokenUnit;
      }
    }
    return invalidTokenUnit;
  }
};

} // namespace inditrans::handlers
