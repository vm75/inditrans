#!/bin/bash

script=$(realpath "${0}")
scriptDir=$(dirname "${script}")
distDir=$(realpath "${scriptDir}/../../dist")

mkdir "${distDir}" 2> /dev/null

targetBin="${distDir}/indic_trans_prof"
rm -f "${targetBin}"
g++ -std=c++20 -O1 -fno-exceptions -pg -Wno-normalized ${cppoptions} -I "${scriptDir}/src" "${scriptDir}/src"/*.cpp "${scriptDir}/test"/*.cpp -o "${targetBin}"
"${targetBin}" -p
gprof "${targetBin}" gmon.out > "${distDir}/wasm-prof.log"