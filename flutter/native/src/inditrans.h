#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "exports.h"

constexpr size_t MaxTranslitOptions = 8;

inline constexpr TranslitOptions operator&(TranslitOptions x, TranslitOptions y) noexcept {
  return static_cast<TranslitOptions>(static_cast<uint64_t>(x) & static_cast<uint64_t>(y));
}

inline constexpr TranslitOptions operator|(TranslitOptions x, TranslitOptions y) noexcept {
  return static_cast<TranslitOptions>(static_cast<uint64_t>(x) | static_cast<uint64_t>(y));
}

TranslitOptions getTranslitOptions(const std::string_view& optStr) noexcept;

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::unique_ptr<char>& out) noexcept;

std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options) noexcept;
