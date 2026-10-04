#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "exports.h"

constexpr size_t MaxTranslitOptions = 7;

inline constexpr TranslitOptions operator&(TranslitOptions x, TranslitOptions y) noexcept {
  return static_cast<TranslitOptions>(static_cast<uint64_t>(x) & static_cast<uint64_t>(y));
}

inline constexpr TranslitOptions operator|(TranslitOptions x, TranslitOptions y) noexcept {
  return static_cast<TranslitOptions>(static_cast<uint64_t>(x) | static_cast<uint64_t>(y));
}

TranslitOptions getTranslitOptions(const std::string_view& optStr) noexcept;

struct TranslitBufferDeleter {
  void operator()(char* buffer) const noexcept { std::free(buffer); }
};

using TranslitBuffer = std::unique_ptr<char, TranslitBufferDeleter>;

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, TranslitBuffer& out, const std::string_view& skipStart = "##",
    const std::string_view& skipEnd = "##") noexcept;

bool transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, std::string& out, const std::string_view& skipStart = "##",
    const std::string_view& skipEnd = "##") noexcept;

std::string transliterate(const std::string_view& input, const std::string_view& from, const std::string_view& to,
    TranslitOptions options, const std::string_view& skipStart = "##", const std::string_view& skipEnd = "##") noexcept;
