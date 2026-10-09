#ifndef __INDITRANS_H
#define __INDITRANS_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

/* DLL defines
  Define UNDECO_DLL for un-decorated dll
  verify compiler option __cdecl for un-decorated and __stdcall for decorated */
/*#define UNDECO_DLL*/
#ifdef MAKE_DLL
#if defined(PASCAL) || defined(__stdcall)
#if defined UNDECO_DLL
#define CALL_CONV EMSCRIPTEN_KEEPALIVE __cdecl
#else
#define CALL_CONV EMSCRIPTEN_KEEPALIVE __stdcall
#endif
#else
#define CALL_CONV EMSCRIPTEN_KEEPALIVE
#endif
/* To export symbols in the new DLL model of Win32, Microsoft
   recommends the following approach */
#define EXP32 __declspec(dllexport)
#else
#define CALL_CONV EMSCRIPTEN_KEEPALIVE
#define EXP32
#endif

/* ext_def(x) evaluates to x on Unix */
#define ext_def(x) extern EXP32 x CALL_CONV

/***********************************************************
 * exported functions
 ***********************************************************/
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// Transliteration options
/// Flags to control transliteration
enum TranslitOptions {
  /// No options (default)
  None = 0,
  /// Use traditional Tamil consonants only
  TamilTraditional = 1,
  /// Use superscripted Tamil consonants
  TamilSuperscripted = 2,
  /// Force ASCII numerals in transliterated text
  ASCIINumerals = 4,
  /// Ignore Vedic accents in transliterated text
  IgnoreVedicAccents = 8,
  /// Retain special markers which are used to identify non-standard chars
  RetainSpecialMarkers = 16,
  /// Do not check for xml/html tags, and treat them as normal text
  NoXMLTagHandling = 32,
};

/// Transliterates null-terminated UTF-8 text from source script to target script.
///
/// Allocates a new null-terminated C string on the heap containing the transliterated result.
/// The caller assumes ownership of the returned pointer and MUST release it using releaseBuffer().
///
/// @param text The null-terminated UTF-8 input string.
/// @param from Source script identifier or alias (e.g. "itrans", "devanagari").
/// @param to Target script identifier or alias (e.g. "bengali", "iast").
/// @param options Bitwise combination of TranslitOptions flags.
/// @param skipStart Delimiter marking the start of protected text blocks; empty defaults to "##".
/// @param skipEnd Delimiter marking the end of protected text blocks; empty defaults to "##".
/// @return Pointer to heap-allocated result string, or nullptr on failure.
ext_def(char*) transliterate(const char* text, const char* from, const char* to, unsigned long options,
    const char* skipStart, const char* skipEnd);

/// Checks whether a script identifier or alias is physically supported by the engine.
///
/// Note: Virtual sources such as "indic" are handled dynamically by wrapper layers;
/// this function returns non-zero only if a concrete script dictionary exists for the name.
///
/// @param script The script identifier or alias to query (case-insensitive).
/// @return Non-zero (1) if supported, 0 otherwise.
ext_def(int) isScriptSupported(const char* script);

/// Deallocates memory previously allocated by transliterate().
///
/// Must be called across the FFI/ABI boundary for every non-null buffer returned by transliterate().
///
/// @param buffer Pointer to heap buffer to free (safe to pass nullptr).
ext_def(void) releaseBuffer(char* buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __INDITRANS_H
