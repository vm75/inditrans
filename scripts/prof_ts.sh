#!/bin/bash

script=$(realpath "${0}")
scriptDir=$(dirname "${script}")
workspaceDir=$(realpath "${scriptDir}/../..")
distDir=$(realpath "${scriptDir}/dist")

cd "${workspaceDir}"
rm isolate*.log
tsc -p tsconfig.json
node --prof --no-turbo-inlining --no-warnings --experimental-modules --es-module-specifier-resolution=node dist/ts/test/tests.js
node --prof-process isolate-*.log > dist/ts-prof.log
