#!/usr/bin/env python3
"""Check that the shell and PowerShell release Wasm commands stay in sync."""

from __future__ import annotations

import re
import shlex
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
BUILDERS = {
    "Unix": (ROOT / "tool/build_wasm.sh", "\\"),
    "PowerShell": (ROOT / "tool/build_wasm.ps1", "`"),
}
REQUIRED = {
    "standalone": {
        "-fPIC",
        "-Wl,--gc-sections,--no-entry,--export=__wasm_call_ctors",
        "STANDALONE_WASM=1",
        "ENVIRONMENT=web,worker",
        "FILESYSTEM=0",
        'EXPORTED_FUNCTIONS=["_malloc", "_free"]',
    },
    "js": {
        "-Wl,--gc-sections,--no-entry",
        "WASM=1",
        "ENVIRONMENT=web,node",
        "SINGLE_FILE=1",
        "ALLOW_MEMORY_GROWTH=1",
        "EXIT_RUNTIME=0",
        "FILESYSTEM=0",
        'EXPORTED_FUNCTIONS=["_malloc", "_free", "_transliterate", "_isScriptSupported", "_releaseBuffer"]',
        'EXPORTED_RUNTIME_METHODS=["cwrap", "UTF8ToString"]',
        "--post-js",
        "./js/src/inditrans.post.js",
    },
}
COMMON_RELEASE_FLAGS = {
    "-std=c++23",
    "-Oz",
    "-flto",
    "-fno-exceptions",
    "-fno-rtti",
    "-fno-stack-protector",
    "-ffunction-sections",
    "-fdata-sections",
    "-fno-math-errno",
    "-DNDEBUG",
}


def release_args(path: Path, continuation: str, target: str) -> list[str]:
    lines = path.read_text(encoding="utf-8").splitlines()
    start = next(
        (i for i, line in enumerate(lines) if re.match(rf"^(?:function\s+)?build_wasm_{target}\b", line)),
        None,
    )
    if start is None:
        raise ValueError(f"{path.name}: missing build_wasm_{target}")
    end = next((i for i in range(start + 1, len(lines)) if lines[i] == "}"), None)
    if end is None:
        raise ValueError(f"{path.name}: cannot find end of build_wasm_{target}")
    body = lines[start:end]
    debug = next((i for i, line in enumerate(body) if "debug" in line and line.lstrip().startswith("if ")), None)
    release = next((i for i in range((debug or 0) + 1, len(body)) if body[i].strip() in {"else", "else {"}), None)
    command_start = next(
        (i for i in range((release or 0) + 1, len(body)) if body[i].lstrip().startswith("em++ ")),
        None,
    )
    if debug is None or release is None or command_start is None:
        raise ValueError(f"{path.name}: cannot parse {target} release command")

    command_lines = []
    for line in body[command_start:]:
        line = line.strip()
        continued = line.endswith(continuation)
        command_lines.append(line[:-1].rstrip() if continued else line)
        if not continued:
            break
    command = " ".join(command_lines).replace("\\", "/")
    tokens = shlex.split(command)
    if not tokens or tokens[0] != "em++":
        raise ValueError(f"{path.name}: cannot tokenize {target} release command")

    variables = {}
    for name in ("exportedFunctions", "exportedRuntimeMethods"):
        match = re.search(rf"^\s*\$?{name}\s*=\s*'([^']*)'", "\n".join(body), re.MULTILINE)
        if match:
            variables[name] = match.group(1)
    args = []
    for token in tokens[1:]:
        token = token.replace("${outDir}", "$outDir")
        for name, value in variables.items():
            token = token.replace("${" + name + "}", value).replace("$" + name, value)
        args.append(token)
    return args


def main() -> int:
    try:
        for target, required in REQUIRED.items():
            commands = {
                name: release_args(path, continuation, target)
                for name, (path, continuation) in BUILDERS.items()
            }
            for name, args in commands.items():
                missing = (COMMON_RELEASE_FLAGS | required) - set(args)
                if missing:
                    raise ValueError(f"{name} {target} release command missing: {', '.join(sorted(missing))}")
            unix, powershell = commands.values()
            if unix != powershell:
                for index in range(max(len(unix), len(powershell))):
                    left = unix[index] if index < len(unix) else "<missing>"
                    right = powershell[index] if index < len(powershell) else "<missing>"
                    if left != right:
                        raise ValueError(
                            f"{target} release arguments differ at {index + 1}: Unix={left!r}, PowerShell={right!r}"
                        )
            print(f"✓ {target} Unix and PowerShell release settings match")
    except (OSError, ValueError) as error:
        print(f"Wasm build parity check failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
