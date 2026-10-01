#!/bin/bash
set -euo pipefail

DLL="${1:-native/build_win/inditrans.dll}"

if [ ! -f "$DLL" ]; then
  echo "Error: DLL file not found: $DLL" >&2
  exit 1
fi

OBJDUMP="${OBJDUMP:-x86_64-w64-mingw32-objdump}"
if ! command -v "$OBJDUMP" &>/dev/null; then
  if command -v objdump &>/dev/null; then
    OBJDUMP="objdump"
  else
    echo "Error: objdump or x86_64-w64-mingw32-objdump not found" >&2
    exit 1
  fi
fi

echo "Verifying Windows DLL: $DLL"

# Check PE32+ (x86-64) architecture
if command -v file &>/dev/null; then
  FILE_INFO=$(file "$DLL")
  echo "  File info: $FILE_INFO"
  if ! echo "$FILE_INFO" | grep -E -q "PE32\+.*x86-64"; then
    echo "Error: $DLL is not a 64-bit Windows PE (PE32+ x86-64) binary" >&2
    exit 1
  fi
  echo "  ✓ Confirmed 64-bit Windows PE/COFF architecture"
fi

EXPORTS=$("$OBJDUMP" -p "$DLL" | awk '/\[Ordinal\/Name Pointer\] Table/,/^$/')

# 1. Verify expected C API symbols
for sym in transliterate isScriptSupported releaseBuffer; do
  if ! echo "$EXPORTS" | grep -w "$sym" &>/dev/null; then
    echo "Error: Missing expected export symbol '$sym'" >&2
    exit 1
  fi
  echo "  ✓ Found exported symbol: $sym"
done

# 2. Verify no decorated C++ exports
DECORATED=$(echo "$EXPORTS" | grep -E '_Z|\?' || true)
if [ -n "$DECORATED" ]; then
  echo "Error: Found unexpected decorated C++ export(s):" >&2
  echo "$DECORATED" >&2
  exit 1
fi
echo "  ✓ No decorated C++ exports found in export table"

# 3. Verify no unintended MinGW C++ runtime DLL dependencies
IMPORTS=$("$OBJDUMP" -p "$DLL" | grep "DLL Name:" || true)
RUNTIME_DEPS=$(echo "$IMPORTS" | grep -E "libstdc\+\+|libgcc_s" || true)
if [ -n "$RUNTIME_DEPS" ]; then
  echo "Error: Found unintended MinGW C++ runtime DLL dependency:" >&2
  echo "$RUNTIME_DEPS" >&2
  exit 1
fi
echo "  ✓ No dynamic libstdc++ or libgcc runtime dependencies"

# 4. Optional Wine runtime smoke test
WINE_BIN=""
if command -v wine64 &>/dev/null; then
  WINE_BIN="wine64"
elif command -v wine &>/dev/null; then
  WINE_BIN="wine"
fi

if [ -n "$WINE_BIN" ]; then
  CXX="${CXX:-x86_64-w64-mingw32-g++}"
  if command -v "$CXX" &>/dev/null; then
    echo "Running optional runtime smoke test with $WINE_BIN..."
    SMOKE_SRC="tool/windows_smoke_test.cpp"
    if [ -f "$SMOKE_SRC" ]; then
      BUILD_DIR=$(dirname "$DLL")
      "$CXX" -std=c++23 -I native/src "$SMOKE_SRC" -L "$BUILD_DIR" -linditrans -static-libgcc -static-libstdc++ -o "$BUILD_DIR/windows_smoke_test.exe"
      (cd "$BUILD_DIR" && WINEDEBUG=-all "$WINE_BIN" windows_smoke_test.exe)
      echo "  ✓ Wine runtime smoke test passed"
    fi
  fi
else
  echo "  (Wine not installed; skipping optional runtime smoke test)"
fi

echo "All DLL validations passed successfully."
