#!/bin/bash

script=$(realpath "${0}")
scriptDir=$(dirname "${script}")
distDir=$(realpath "${scriptDir}/../../dist")
targetBin="${distDir}/inditrans_test"

mkdir "${distDir}" 2> /dev/null

if [[ $1 == -b || ! -f ${targetBin} ]] ; then
  ${scriptDir}/build.sh -n
fi

if [[ -f "${targetBin}" ]] ; then
  # /usr/bin/time -v "${targetBin}"
  "${targetBin}"
fi