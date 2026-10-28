#pragma once

#include "type_defs.h"
#include "exports.h"
#include <span>
#include <string>
#include <string_view>

/// Borrowed, immutable pronunciation data. Entries contain whole Urdu words and
/// canonical token units, not target-script strings. Storage must outlive a call.
struct UrduLexiconEntry {
  std::string_view word;
  std::span<const TokenUnit> pronunciation;
};

using UrduLexicon = std::span<const UrduLexiconEntry>;

/// Experimental explicit Urdu pronunciation resolver. The supplied entries and
/// their token units are borrowed for this call. Unknown words use the regular
/// Urdu reader. DisableInference bypasses the lexicon.
bool transliterateUrduWithLexicon(const std::string_view& input, const std::string_view& to,
    TranslitOptions options, UrduLexicon lexicon, std::string& out,
    const std::string_view& skipStart = "##", const std::string_view& skipEnd = "##") noexcept;
