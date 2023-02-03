#!/bin/bash

script=$(realpath "$0")
scriptdir=$(dirname "${script}")
projdir=$(dirname "${scriptdir}")
distdir=${projdir}/dist

echo ${distdir}

infile=${distdir}/perf.txt

if [[ -f ${infile} ]] ; then
  python "${scriptdir}/aksharamukha-proxy.py" --src Devanagari --dst Telugu --time 1 --file ${infile}
  python "${scriptdir}/aksharamukha-proxy.py" --src Devanagari --dst Tamil --time 1 --file ${infile}
  python "${scriptdir}/aksharamukha-proxy.py" --src Devanagari --dst RomanReadable --time 1 --file ${infile}
fi
