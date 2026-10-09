#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "exports.h"

/// Maximum number of configurable transliteration option flags.
constexpr size_t MaxTranslitOptions = 7;

/// Bitwise AND operator for combining/querying TranslitOptions bitmasks.
inline constexpr TranslitOptions operator&(TranslitOptions x, TranslitOptions y) noexcept {
  return static_cast<TranslitOptions>(static_cast<uint64_t>(x) & static_cast<uint64_t>(y));
}

/// Bitwise OR operator for combining TranslitOptions bitmasks.
inline constexpr TranslitOptions operator|(TranslitOptions x, TranslitOptions y) noexcept {
  return static_cast<TranslitOptions>(static_cast<uint64_t>(x) | static_cast<uint64_t>(y));
}

/// Parses a delimited string of option names (e.g. "TamilTraditional,IgnoreVedicAccents")
/// into a bitmask of TranslitOptions flags.
TranslitOptions getTranslitOptions(const std::string_view& optStr) noexcept;

/// Custom deleter for buffers allocated via std::malloc / realloc by the C API engine.
struct TranslitBufferDeleter {
  void operator()(char* buffer) const noexcept { std::free(buffer); }
};

/// Smart pointer wrapper for C-allocated null-terminated transliteration output buffers.
using TranslitBuffer = std::unique_ptr<char, TranslitBufferDeleter>;

/// Transliterates text from a source script to a target script, writing the result into a TranslitBuffer.
///
/// @param input The UTF-8 input text to transliterate.
/// @param from Source script identifier or alias (e.g. "itrans", "devanagari").
/// @param to Target script identifier or alias (e.g. "bengali", "iast").
/// @param options Transliteration flags controlling orthography, Vedic accents, etc.
/// @param out Output TranslitBuffer receiving the newly allocated null-terminated result.
/// @param skipStart Delimiter marking the beginning of protected/untransliterated blocks (default: "##").
/// @param skipEnd Delimiter marking the end of protected/untransliterated blocks (default: "##").
/// @return True if transliteration succeeded; false if script names were identical or unrecognized.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& out, const std::string_view& skipStart = "##",
    const std::string_view& skipEnd = "##") noexcept;

/// Transliterates text from a source script to a target script, appending directly into a std::string sink.
///
/// Existing contents are preserved; the string may grow as output is appended.
///
/// @param input The UTF-8 input text to transliterate.
/// @param from Source script identifier or alias.
/// @param to Target script identifier or alias.
/// @param options Transliteration flags controlling orthography, Vedic accents, etc.
/// @param out Reference to std::string receiving the transliterated text.
/// @param skipStart Delimiter marking the beginning of protected blocks (default: "##").
/// @param skipEnd Delimiter marking the end of protected blocks (default: "##").
/// @return True on success; false if script names were identical or unrecognized.
bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::string& out, const std::string_view& skipStart = "##",
    const std::string_view& skipEnd = "##") noexcept;

/// Transliterates text from a source script to a target script, returning the result by value.
///
/// @param input The UTF-8 input text to transliterate.
/// @param from Source script identifier or alias.
/// @param to Target script identifier or alias.
/// @param options Transliteration flags controlling orthography, Vedic accents, etc.
/// @param skipStart Delimiter marking the beginning of protected blocks (default: "##").
/// @param skipEnd Delimiter marking the end of protected blocks (default: "##").
/// @return The transliterated string on success, or an empty string on failure.
std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, const std::string_view& skipStart = "##", const std::string_view& skipEnd = "##") noexcept;
