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
            .\emsdk install latest
            .\emsdk activate latest
            [Environment]::SetEnvironmentVariable("EMSDK", $emsdk, "User")
  
            Set-Location $currDir
        }
        else {
            # set EMSDK environment variable and persist it
            $env:EMSDK = $emsdk
            [Environment]::SetEnvironmentVariable("EMSDK", $emsdk, "User")
        }
    }
  
    # set emsdk environment variables
    . $env:EMSDK\emsdk_env.ps1
}

# create a function
function build_wasm_standalone {
    $exportedFunctions = "[""_malloc"", ""_free"", ""_transliterate"", ""_releaseBuffer""]"

    # get the path to the output directory
    $outDir = ".\flutter\assets"

    # create the output directory if it does not exist
    if (!(Test-Path $outDir)) {
        New-Item -ItemType Directory -Path $outDir
    }

    # build the function
    if ($args[1] -eq "debug") {
        emcc .\native\src\inditrans.cpp -I .\native\src `
            -std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address `
            "-Wl,--no-entry" `
            -DDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s FILESYSTEM=0 `
            -o $outDir\inditrans.wasm
    }
    else {
        emcc .\native\src\inditrans.cpp -I .\native\src `
            -std=c++20 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno `
            "-Wl,--gc-sections,--no-entry" `
            -DNDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s FILESYSTEM=0 `
            -o $outDir\inditrans.wasm
    }
}

function build_wasm_js {
    $exportedRuntimeMethods = "[""ccall"", ""cwrap""]"
    $exportedFunctions = "[""_malloc"", ""_free"", ""_transliterate"", ""_releaseBuffer""]"

    # get the path to the output directory
    $outDir = ".\js\dist"

    # create the output directory if it does not exist
    if (!(Test-Path $outDir)) {
        New-Item -ItemType Directory -Path $outDir
    }

    # build the function
    if ($args[1] -eq "debug") {
        emcc .\native\src\inditrans.cpp -I .\native\src `
            -std=c++20 -g3 --profiling-funcs -s ASSERTIONS=1 -fsanitize=address `
            "-Wl,--no-entry" `
            -DDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s EXPORTED_RUNTIME_METHODS=$exportedRuntimeMethods `
            -s WASM=1 `
            -s ENVIRONMENT='web,node' `
            -s SINGLE_FILE=1 `
            -s ALLOW_MEMORY_GROWTH=1 `
            -s NO_EXIT_RUNTIME=1 `
            -s FILESYSTEM=0 `
            --post-js .\js\src\inditrans.post.js `
            -o $outDir\inditrans.js
    }
    else {
        emcc .\native\src\inditrans.cpp -I .\native\src `
            -std=c++20 -Oz -fno-exceptions -fno-rtti -fno-stack-protector -ffunction-sections -fdata-sections -fno-math-errno `
            "-Wl,--gc-sections,--no-entry" `
            -DNDEBUG `
            -s EXPORTED_FUNCTIONS=$exportedFunctions `
            -s EXPORTED_RUNTIME_METHODS=$exportedRuntimeMethods `
            -s WASM=1 `
            -s ENVIRONMENT='web,node' `
            -s SINGLE_FILE=1 `
            -s ALLOW_MEMORY_GROWTH=1 `
            -s NO_EXIT_RUNTIME=1 `
            -s FILESYSTEM=0 `
            --post-js .\js\src\inditrans.post.js `
            -o $outDir\inditrans.js
    }
}

# initialize emcc
init_emcc

# build 
if ($args[0] -eq "standalone") {
    build_wasm_standalone $args[1]
}
elseif ($args[0] -eq "js") {
    build_wasm_js $args[1]
}
else {
    Write-Host "Invalid build type"
}
