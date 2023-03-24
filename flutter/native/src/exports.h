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
};

/// transliterate text from one script to another
ext_def(char*) transliterate(const char* text, const char* from, const char* to, unsigned long options);

/// returns if the given script is supported
ext_def(int) isScriptSupported(const char* script);

/// free up the memory allocated by the library
ext_def(void) releaseBuffer(char* buffer);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __INDITRANS_H
