function setup_emsdk {
    # check if $env:EMSDK is not defined or if the path does not exist
    if ($null -ne $env:EMSDK -or (Test-Path $env:EMSDK)) {
        return
    }
    # look for emsdk in the default location - AppData\Local\Programs\emsdk
    $emsdk_root = Join-Path $env:LOCALAPPDATA "Programs\emsdk"

    # if not installed clone https://github.com/emscripten-core/emsdk.git and install
    if (Test-Path $emsdk_root) {
        return
    }
    git clone https://github.com/emscripten-core/emsdk.git $emsdk_root

    $currDir = Get-Location

    Set-Location $emsdk_root
    .\emsdk install latest
    .\emsdk activate latest
    [Environment]::SetEnvironmentVariable("EMSDK", $emsdk_root, "User")
}

function setup_flutter {
    # check if $env:EMSDK is not defined or if the path does not exist
    if ($null -ne $env:EMSDK -or (Test-Path $env:EMSDK)) {
        return
    }
    # look for flutter in the default location - AppData\Local\Programs\flutter
    $flutter_root = Join-Path $env:LOCALAPPDATA "Programs\flutter"

    # if not installed clone https://github.com/emscripten-core/emsdk.git and install
    if (Test-Path $flutter_root) {
        return
    }

    $flutter_zip = Join-Path $flutter_root "flutter_windows_3.7.7-stable.zip"
    $flutter_zip_url = "https://storage.googleapis.com/flutter_infra_release/releases/stable/windows/flutter_windows_3.7.7-stable.zip"
    Invoke-WebRequest -Uri $flutter_zip_url -OutFile $flutter_zip
    Expand-Archive -Path $flutter_zip -DestinationPath $flutter_root
    Remove-Item $flutter_zip
}

setup_emsdk
setup_flutter