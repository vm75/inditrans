#!/bin/bash
set -euo pipefail

REQUIRED_EMSDK_VERSION="6.0.10"

setup_emsdk() {
  # Determine emsdk location: use existing EMSDK if valid, else default to ~/.local/share/emsdk
  local emsdk_root=""
  if [[ -n "${EMSDK:-}" && -d "${EMSDK}" ]]; then
    emsdk_root="${EMSDK}"
  else
    emsdk_root="${HOME}/.local/share/emsdk"
  fi

  # If not cloned yet, clone it
  if [[ ! -d "${emsdk_root}" ]]; then
    mkdir -p "$(dirname "${emsdk_root}")"
    git clone https://github.com/emscripten-core/emsdk.git "${emsdk_root}"
  fi

  # Explicitly install and activate required Emscripten version
  local currDir
  currDir=$(pwd)
  cd "${emsdk_root}"
  ./emsdk install "${REQUIRED_EMSDK_VERSION}"
  ./emsdk activate "${REQUIRED_EMSDK_VERSION}"
  export EMSDK="${emsdk_root}"

  if [[ -f "${emsdk_root}/emsdk_env.sh" ]]; then
    export EMSDK_QUIET=1
    # shellcheck source=/dev/null
    source "${emsdk_root}/emsdk_env.sh"
  fi
  cd "${currDir}"
}

setup_flutter() {
  # look for flutter in the default location - ~/.local/share/flutter
  local flutter_root="${HOME}/.local/share/flutter"

  if [[ -d "${flutter_root}" ]]; then
    return
  fi

  mkdir -p "${HOME}/.local/share"
  local currDir
  currDir=$(pwd)

  cd "${HOME}/.local/share"
  wget https://storage.googleapis.com/flutter_infra_release/releases/stable/linux/flutter_linux_3.7.7-stable.tar.xz
  tar xf flutter_linux_3.7.7-stable.tar.xz
  rm flutter_linux_3.7.7-stable.tar.xz
  cd "${currDir}"
}

setup_emsdk
setup_flutter
