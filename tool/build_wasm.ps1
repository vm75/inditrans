$REQUIRED_EMSDK_VERSION = "6.0.10"

function init_emcc {
    # check if $env:EMSDK is not defined or if the path does not exist
    if ($null -eq $env:EMSDK -or !(Test-Path $env:EMSDK)) {
        # look for emsdk in the default location - AppData\Local\Programs\emsdk
        $emsdk = Join-Path $env:LOCALAPPDATA "Programs\emsdk"

        # if not installed clone https://github.com/emscripten-core/emsdk.git and install
        if (!(Test-Path $emsdk)) {
            git clone https://github.com/emscripten-core/emsdk.git $emsdk

            $currDir = Get-Location

            Set-Location $emsdk
            .\emsdk install $REQUIRED_EMSDK_VERSION
            .\emsdk activate $REQUIRED_EMSDK_VERSION
            $env:EMSDK = $emsdk
            [Environment]::SetEnvironmentVariable("EMSDK", $emsdk, "User")
            Set-Location $currDir
        }
        else {
            # set EMSDK environment variable for current process and persist it
            $env:EMSDK = $emsdk
            [Environment]::SetEnvironmentVariable("EMSDK", $emsdk, "User")
        }
    }

    # set emsdk environment variables
    if (Test-Path "$env:EMSDK\emsdk_env.ps1") {
        $env:EMSDK_QUIET = "1"
        . "$env:EMSDK\emsdk_env.ps1"
    }

    # Verify active compiler
    $empp = Get-Command em++ -ErrorAction SilentlyContinue
    if ($null -eq $empp) {
        Write-Error "inditrans requires Emscripten $REQUIRED_EMSDK_VERSION (em++ not found in PATH or EMSDK)"
        exit 1
    }

    $versionOutput = & em++ --version 2>$null | Select-Object -First 1
    if ($versionOutput -match '(\d+\.\d+\.\d+)') {
        $activeVersion = $Matches[1]
    } else {
        $activeVersion = "unknown"
    }

    if ($activeVersion -ne $REQUIRED_EMSDK_VERSION) {
        Write-Error "inditrans requires Emscripten $REQUIRED_EMSDK_VERSION (found $activeVersion)"
        exit 1
    }
}

# create a function
function build_wasm_standalone([string]$mode) {
    $exportedFunctions = '["_malloc", "_free", "_transliterate", "_isScriptSupported", "_releaseBuffer"]'

    # get the path to the output directory
    $outDir = ".\flutter\assets"

    # create the output directory if it does not exist
    if (!(Test-Path $outDir)) {
        New-Item -ItemType Directory -Path $outDir | Out-Null
    }

    # build the function
    if ($mode -eq "debug") {
        em++ .\native\src\inditrans.cpp -I .\native\src `
            -std=c++23 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address `
            "-Wl,--no-entry,--export=__wasm_call_ctors" `
            -DDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s ENVIRONMENT='web,worker' `
            -s FILESYSTEM=0 `
            -o "$outDir\inditrans.wasm"
    }
    else {
        em++ .\native\src\inditrans.cpp -I .\native\src `
            -std=c++23 -fPIC -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno -DNDEBUG `
            "-Wl,--gc-sections,--no-entry,--export=__wasm_call_ctors" `
            -s EXPORTED_FUNCTIONS='["_malloc", "_free"]' `
            -s STANDALONE_WASM=1 `
            -s ENVIRONMENT='web,worker' `
            -s FILESYSTEM=0 `
            -o "$outDir\inditrans.wasm"
    }
}

function build_wasm_js([string]$mode) {
    $exportedRuntimeMethods = '["cwrap", "UTF8ToString"]'
    $exportedFunctions = '["_malloc", "_free", "_transliterate", "_isScriptSupported", "_releaseBuffer"]'

    # get the path to the output directory
    $outDir = ".\js\public"

    # create the output directory if it does not exist
    if (!(Test-Path $outDir)) {
        New-Item -ItemType Directory -Path $outDir | Out-Null
    }

    # build the function
    if ($mode -eq "debug") {
        em++ .\native\src\inditrans.cpp -I .\native\src `
            -std=c++23 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address `
            "-Wl,--no-entry" `
            -DDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s EXPORTED_RUNTIME_METHODS=$exportedRuntimeMethods `
            -s WASM=1 `
            -s ENVIRONMENT='web,node' `
            -s SINGLE_FILE=1 `
            -s ALLOW_MEMORY_GROWTH=1 `
            -s EXIT_RUNTIME=0 `
            -s FILESYSTEM=0 `
            --post-js .\js\src\inditrans.post.js `
            -o "$outDir\inditrans.js"
    }
    else {
        em++ .\native\src\inditrans.cpp -I .\native\src `
            -std=c++23 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno `
            "-Wl,--gc-sections,--no-entry" `
            -DNDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s EXPORTED_RUNTIME_METHODS=$exportedRuntimeMethods `
            -s WASM=1 `
            -s ENVIRONMENT='web,node' `
            -s SINGLE_FILE=1 `
            -s ALLOW_MEMORY_GROWTH=1 `
            -s EXIT_RUNTIME=0 `
            -s FILESYSTEM=0 `
            --post-js .\js\src\inditrans.post.js `
            -o "$outDir\inditrans.js"
    }
}

# initialize emcc
init_emcc

Set-Location "$PSScriptRoot\.."

# build
$target = if ($args.Count -gt 0) { $args[0] } else { "" }
$mode = if ($args.Count -gt 1) { $args[1] } else { "" }

if ($target -eq "standalone") {
    build_wasm_standalone $mode
}
elseif ($target -eq "js") {
    build_wasm_js $mode
}
else {
    Write-Error "Invalid build target '$target'"
    exit 1
}
