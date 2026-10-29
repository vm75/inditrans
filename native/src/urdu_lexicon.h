#pragma once

#include "pronunciation_lexicon.h"
#include "exports.h"

using UrduLexiconEntry = PronunciationLexiconEntry;
using UrduLexicon = PronunciationLexicon;
using UrduLexiconLookup = PronunciationLexiconLookup;

/// Experimental explicit Urdu pronunciation resolver. The supplied entries and
/// their token units are borrowed for this call. Unknown words use the regular
/// Urdu reader. DisableInference bypasses the lexicon.
bool transliterateUrduWithLexicon(const std::string_view& input, const std::string_view& to,
    TranslitOptions options, UrduLexicon lexicon, std::string& out,
    const std::string_view& skipStart = "##", const std::string_view& skipEnd = "##") noexcept;
