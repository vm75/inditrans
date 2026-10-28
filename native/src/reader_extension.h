#pragma once

#include "exports.h"
#include "token_stream.h"
#include <string>
#include <string_view>

/// A borrowed, per-call token source. A successful pull consumes source input
/// and/or emits tokens. Each pull may emit at most MaxTokens, and must respect
/// XML and protected spans using the supplied options and delimiters.
struct ReaderExtension {
  static constexpr size_t MaxTokens = 16;
  void* state;
  bool (*hasPending)(const void*) noexcept;
  bool (*pull)(void*, const char*&, const char*, std::string_view, std::string_view,
      TranslitOptions, TokenOrString*, size_t&) noexcept;

  template <typename T> static ReaderExtension bind(T& source) noexcept {
    return {&source,
      [](const void* p) noexcept { return static_cast<const T*>(p)->hasPending(); },
      [](void* p, const char*& ptr, const char* end, std::string_view start, std::string_view finish,
          TranslitOptions options, TokenOrString* out, size_t& size) noexcept {
        return static_cast<T*>(p)->pull(ptr, end, start, finish, options, out, size);
      }};
  }
};

/// Internal C++ seam for a caller-owned source extension; ordinary conversion
/// does not install one. Source/target resolution and writer behavior are shared.
bool transliterateWithReaderExtension(std::string_view input, std::string_view from, std::string_view to,
    TranslitOptions options, std::string& output, ReaderExtension& extension,
    std::string_view skipStart, std::string_view skipEnd) noexcept;
