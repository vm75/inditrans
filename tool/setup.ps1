$REQUIRED_EMSDK_VERSION = "6.0.10"

function setup_emsdk {
    # Determine emsdk location: use existing EMSDK if valid, else default to AppData\Local\Programs\emsdk
    if ($null -ne $env:EMSDK -and (Test-Path $env:EMSDK)) {
        $emsdk_root = $env:EMSDK
    }
    else {
        $emsdk_root = Join-Path $env:LOCALAPPDATA "Programs\emsdk"
    }

    # If not cloned yet, clone it
    if (!(Test-Path $emsdk_root)) {
        $parentDir = Split-Path -Parent $emsdk_root
        if (!(Test-Path $parentDir)) {
            New-Item -ItemType Directory -Path $parentDir | Out-Null
        }
        git clone https://github.com/emscripten-core/emsdk.git $emsdk_root
    }

    $currDir = Get-Location

    # Explicitly install and activate required Emscripten version
    Set-Location $emsdk_root
    .\emsdk install $REQUIRED_EMSDK_VERSION
    .\emsdk activate $REQUIRED_EMSDK_VERSION

    $env:EMSDK = $emsdk_root
    [Environment]::SetEnvironmentVariable("EMSDK", $emsdk_root, "User")

    if (Test-Path "$emsdk_root\emsdk_env.ps1") {
        $env:EMSDK_QUIET = "1"
        . "$emsdk_root\emsdk_env.ps1"
    }

    Set-Location $currDir
}

function setup_flutter {
    # look for flutter in the default location - AppData\Local\Programs\flutter
    $flutter_root = Join-Path $env:LOCALAPPDATA "Programs\flutter"

    # if already installed, return
    if (Test-Path $flutter_root) {
        return
    }

    $parentDir = Split-Path -Parent $flutter_root
    if (!(Test-Path $parentDir)) {
        New-Item -ItemType Directory -Path $parentDir | Out-Null
    }

    $flutter_zip = Join-Path $parentDir "flutter_windows_3.7.7-stable.zip"
    $flutter_zip_url = "https://storage.googleapis.com/flutter_infra_release/releases/stable/windows/flutter_windows_3.7.7-stable.zip"
    Invoke-WebRequest -Uri $flutter_zip_url -OutFile $flutter_zip
    Expand-Archive -Path $flutter_zip -DestinationPath $parentDir
    Remove-Item $flutter_zip
}

setup_emsdk
setup_flutter