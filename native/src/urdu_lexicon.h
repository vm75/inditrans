#pragma once

#include "type_defs.h"
#include "exports.h"
#include <span>
#include <string>
#include <string_view>

/// Immutable pronunciation data. Entries contain strictly sorted, canonical
/// UTF-8 Urdu words and canonical token units, not target-script strings.
/// Both word and pronunciation spans must outlive the conversion call.
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
