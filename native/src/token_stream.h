#pragma once

#include "type_defs.h"
#include <bit>
#include <cassert>
#include <cstdint>
#include <string_view>
#include <variant>

/// Discriminated union without std::variant footprint overhead.
///
/// Stores either a 4-byte ScriptToken payload (indicated by text == nullptr, with the token
/// bit-cast into value) or a borrowed UTF-8 string slice (text pointing to the buffer and
/// value holding the length). Occupies 16 bytes on x86-64 and 8 bytes on Wasm32.
struct TokenOrString {
  const char* text; ///< Pointer to borrowed text, or nullptr if holding a ScriptToken
  size_t value; ///< String length if text != nullptr; bit-cast ScriptToken payload if text == nullptr
  TokenOrString() = default;
  TokenOrString(ScriptToken token) noexcept
      : text(nullptr)
      , value(std::bit_cast<uint32_t>(token)) { }
  TokenOrString(std::string_view view) noexcept
      : text(view.data() ? view.data() : "")
      , value(view.size()) { }
};
static_assert(sizeof(ScriptToken) == sizeof(uint32_t));

/// Returns true if the item holds a ScriptToken (indicated by text == nullptr).
inline bool HoldsScriptToken(const TokenOrString& item) noexcept { return item.text == nullptr; }

/// Extracts the ScriptToken payload from a TokenOrString item.
inline ScriptToken GetScriptToken(const TokenOrString& item) noexcept {
  return std::bit_cast<ScriptToken>(static_cast<uint32_t>(item.value));
}

template <typename T> inline bool HoldsScriptToken(const T& var) { return std::holds_alternative<ScriptToken>(var); }
template <typename T> inline ScriptToken GetScriptToken(const T& var) { return std::get<ScriptToken>(var); }

/// Represents either a fully grouped syllabic TokenUnit or an untransliterated string view.
using TokenUnitOrString = std::variant<TokenUnit, std::string_view>;
template <typename T> inline bool HoldsTokenUnit(const T& var) { return std::holds_alternative<TokenUnit>(var); }
template <typename T> inline TokenUnit GetTokenUnit(const T& var) { return std::get<TokenUnit>(var); }

// Common for TokenOrString & TokenUnitOrString
template <typename T> inline bool HoldsString(const T& var) { return std::holds_alternative<std::string_view>(var); }
template <typename T> inline std::string_view GetString(const T& var) { return std::get<std::string_view>(var); }

/// Returns true if the item holds a borrowed string view.
inline bool HoldsString(const TokenOrString& item) noexcept { return item.text != nullptr; }

/// Extracts the borrowed string view from a TokenOrString item.
inline std::string_view GetString(const TokenOrString& item) noexcept { return { item.text, item.value }; }

inline const TokenUnitOrString endOfText("");
inline constexpr TokenUnit invalidTokenUnit(invalidScriptToken);

inline bool operator==(const TokenUnitOrString& a, const TokenUnitOrString& b) noexcept {
  if (HoldsString(a) && HoldsString(b)) {
    return GetString(a) == GetString(b);
  }
  if (HoldsTokenUnit(a) && HoldsTokenUnit(b)) {
    return GetTokenUnit(a) == GetTokenUnit(b);
  }
  return false;
}
inline bool operator!=(const TokenUnitOrString& a, const TokenUnitOrString& b) noexcept { return !(a == b); }
