#!/bin/bash

setup_emsdk() {
  # check if EMSDK is not defined or if the path does not exist
  if [[ ! -z "${EMSDK}" || -d "${EMSDK}" ]]; then
    return
  fi

  # look for emsdk in the default location - AppData\Local\Programs\emsdk
  emsdk_root=~/.local/share/emsdk

  # if not installed clone https://github.com/emscripten-core/emsdk.git and install
  if [[ -d "${emsdk_root}" ]]; then
    return
  fi

  mkdir -p ~/.local/share
  git clone https://github.com/emscripten-core/emsdk.git ${emsdk_root}

  currDir=$(pwd)

  cd ${emsdk_root} || exit
  ./emsdk install latest
  ./emsdk activate latest
}

setup_flutter() {
  # look for emsdk in the default location - AppData\Local\Programs\emsdk
  flutter_root=~/.local/share/emsdk

  if [[ -d "${flutter_root}" ]]; then
    return
  fi

  mkdir -p ~/.local/share

  cd ~/.local/share || exit

  wget https://storage.googleapis.com/flutter_infra_release/releases/stable/linux/flutter_linux_3.7.7-stable.tar.xz
  tar xf flutter_linux_3.7.7-stable.tar.xz
  rm flutter_linux_3.7.7-stable.tar.xz
}

setup_emsdk
setup_flutter
