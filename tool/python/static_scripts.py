"""Resolve transliteration reader data and emit the generated C++ tables.

This module intentionally performs the old reader/equivalent expansion while
generating the header. Runtime reader construction and Unicode parsing stay
out of the engine. Keep all ordering explicit: it determines terminal IDs,
sequence offsets, source masks, and therefore the compiled lookup tables.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path

from .codegen_data import ScriptInfo


CLASSES = ["vowels", "vowelMarks", "consonants", "otherDiacritics", "accents",
           "symbols", "vedicSymbols", "exclusiveSymbols"]
TOKEN_TYPES = ["Vowel", "VowelMark", "Consonant", "OtherDiacritic", "Accent",
               "Symbol", "VedicSymbol", "ExclusiveSymbol"]
TYPE_CHARS = "vmcoasSx"
SEQUENCE_OFFSET_BITS = 12


@dataclass(frozen=True)
class Token:
    type: int
    index: int
    script: str

    @property
    def key(self) -> str:
        return f"{self.type}:{self.index}:{self.script}"

    @property
    def cpp(self) -> str:
        return f"{{TokenType::{TOKEN_TYPES[self.type]}, {self.index}, ScriptType::{self.script}}}"

    @property
    def cpp_semantic(self) -> str:
        name = TOKEN_TYPES[self.type]
        fn = name[0].lower() + name[1:]
        return f"{fn}({self.index}, ScriptType::{self.script})"


@dataclass(frozen=True)
class ReaderToken:
    lead: Token
    extra: tuple[Token, ...] = ()

    @property
    def emitted_key(self) -> str:
        return "|".join(token.key for token in (self.lead, *self.extra))


@dataclass(frozen=True)
class Match:
    token: ReaderToken | None
    length: int


@dataclass(frozen=True)
class Range:
    begin: int
    count: int


def utf8_key(text: str) -> bytes:
    return text.encode("utf-8")


def cpp_string(text: str) -> str:
    """Escape each UTF-8 byte separately to avoid C++ hex-escape ambiguity."""
    return '"' + "".join(f"\\x{byte:02x}" for byte in utf8_key(text)) + '"'


def cpp_u8(text: str) -> str:
    """Emit a C++23 UTF-8 string literal with minimal necessary escaping."""
    escaped = text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\r", "\\r")
    return f'u8"{escaped}"'


class StaticScripts:
    """Build deduplicated reader tries, writer maps, and Tamil-prefix keys."""

    def __init__(self, scripts: list[ScriptInfo], constants_path: str | Path):
        self.scripts = scripts
        self.constants = json.loads(Path(constants_path).read_text(encoding="utf-8"))
        self.audit: list[dict] = []
        self.sequences: list[list[Token]] = [[]]  # Sequence zero is the no-match value.
        self.sequence_offset = 1  # Low bits hold token offsets; zero stays reserved.
        self.sequence_ids: dict[str, int] = {}

    def script_type(self, script: ScriptInfo) -> str:
        if script.category == "latin":
            return "Latin"
        return "Tamil" if script.category == "tamil" else "Indic"

    def chars(self, script: ScriptInfo, kind: str) -> list[str]:
        if kind == "accents":
            return self.constants["LatinAccents" if script.category == "latin" else "VedicAccents"]
        if kind == "exclusiveSymbols":
            return [] if script.category == "latin" else self.constants["ExclusiveSymbols"]
        return script.info.get(kind, [])

    def _add(self, table: dict[str, ReaderToken], spelling: str, token: ReaderToken, mode: str) -> None:
        if not spelling:
            return
        old = table.get(spelling)
        if old is None:
            table[spelling] = token
        elif old.emitted_key != token.emitted_key:
            self.audit.append({"mode": mode, "spelling": spelling,
                               "winner": old.emitted_key, "ignored": token.emitted_key})

    def _add_base(self, table: dict[str, ReaderToken], script: ScriptInfo, mode: str) -> None:
        for kind, category in enumerate(CLASSES):
            for index, spelling in enumerate(self.chars(script, category)):
                if script.category == "latin" and kind == 1 and index != 0:
                    continue
                token = Token(kind, index, self.script_type(script))
                self._add(table, spelling, ReaderToken(token), mode)

    @staticmethod
    def _fold_ascii(text: str) -> str:
        return "".join(chr(ord(char) + 32) if "A" <= char <= "Z" else char for char in text)

    def _match(self, table: dict[str, ReaderToken], text: str, fold: bool) -> Match:
        # Generator keys are valid Unicode. Longest-prefix selection matches the
        # former reader's byte-based lookup, including ASCII-only case folding.
        for length in range(len(text), 0, -1):
            candidate = text[:length]
            token = table.get(self._fold_ascii(candidate) if fold else candidate)
            if token is not None:
                return Match(token, length)
        return Match(None, 0)

    def _reader(self, script: ScriptInfo, fold: bool = False) -> dict[str, ReaderToken]:
        table: dict[str, ReaderToken] = {}
        mode = script.name + (":folded" if fold else "")
        self._add_base(table, script, mode)
        equivalents = script.info.get("equivalents", {})
        for target in sorted(equivalents, key=utf8_key):
            token: ReaderToken | None = None
            if len(target) >= 3 and target[1] == ":":
                kind = TYPE_CHARS.find(target[0])
                if kind < 0:
                    continue
                token = ReaderToken(Token(kind, int(target[2:]) & 255, self.script_type(script)))
            else:
                rest = target
                lead = self._match(table, rest, fold)
                if lead.token is not None:
                    extra = []
                    rest = rest[lead.length:]
                    while rest:
                        next_match = self._match(table, rest, fold)
                        if next_match.token is None:
                            break
                        # Legacy expansion emits the lead token of each following match.
                        extra.append(next_match.token.lead)
                        rest = rest[next_match.length:]
                    token = ReaderToken(lead.token.lead, tuple(extra)) if extra else lead.token
                    if rest:
                        self.audit.append({"mode": mode, "partialTarget": target, "remainder": rest})
            if token is None:
                self.audit.append({"mode": mode, "unresolvedTarget": target})
                continue
            for alias in equivalents[target]:
                self._add(table, alias, token, mode)
        return table

    def _sequence(self, token: ReaderToken) -> int:
        if token.emitted_key in self.sequence_ids:
            return self.sequence_ids[token.emitted_key]
        tokens = [token.lead, *token.extra]
        if (self.sequence_offset + len(tokens) > 1 << SEQUENCE_OFFSET_BITS
                or len(tokens) >= 1 << (16 - SEQUENCE_OFFSET_BITS)):
            raise ValueError("Token sequence exceeds the 16-bit span capacity")
        result = self.sequence_offset | (len(tokens) << SEQUENCE_OFFSET_BITS)
        self.sequence_offset += len(tokens)
        self.sequences.append(tokens)
        self.sequence_ids[token.emitted_key] = result
        return result

    @staticmethod
    def _check16(value: int, what: str) -> None:
        if value > 65535:
            raise ValueError(f"{what} exceeds 16-bit capacity")

    @staticmethod
    def _array(output: list[str], cpp_type: str, name: str, values: list[str]) -> None:
        output.append(f"inline constexpr std::array<{cpp_type}, {len(values)}> {name} {{{{")
        output.extend(f"    {value}," for value in values)
        output.append("}};\n")

    def _prefix_key(self, unit: list[Token]) -> int:
        lead = unit[0]
        value = lead.type | (lead.index << 4) | (["Indic", "Tamil", "Latin", "Others"].index(lead.script) << 12)
        for slot, token in enumerate(unit[1:]):
            value |= (token.type | (token.index << 4)) << (14 + slot * 12)
        return value

    def write_header(self, path: str | Path) -> None:
        if len(self.scripts) > 32:
            raise ValueError("source mask capacity exceeded")
        explicit = [self._reader(script) for script in self.scripts]
        folded = [self._reader(script, True) if script.name in {"iast", "iso"} else explicit[i]
                  for i, script in enumerate(self.scripts)]
        devanagari = next(i for i, script in enumerate(self.scripts) if script.name == "devanagari")

        indic = dict(explicit[devanagari])
        ordered = sorted(self.scripts, key=lambda script: script.name.lower())
        for script in ordered:
            if script.name != "devanagari" and script.category != "latin":
                self._add_base(indic, script, "indic")

        # Merge equal token sequences into source masks, retaining alternatives
        # only when a spelling maps to different tokens across source scripts.
        non_roman: dict[str, dict[int, int]] = {}
        for source, script in enumerate(self.scripts):
            if script.category == "latin":
                continue
            for spelling, token in explicit[source].items():
                sequence = self._sequence(token)
                variants = non_roman.setdefault(spelling, {})
                variants[sequence] = variants.get(sequence, 0) | (1 << source)
        for spelling in indic:
            non_roman.setdefault(spelling, {})

        keys = sorted(non_roman, key=utf8_key)
        script_id_names = [s.name[0].upper() + s.name[1:] for s in self.scripts]

        def format_mask(mask: int) -> str:
            if mask == 0:
                return "0u"
            matched = [f"ScriptId::{script_id_names[i]}" for i in range(len(script_id_names)) if (mask & (1 << i))]
            return f"scriptMask({', '.join(matched)})"

        def semantic_sequence(identifier: int) -> str:
            if identifier == 0:
                return "seq()"
            offset = identifier & ((1 << SEQUENCE_OFFSET_BITS) - 1)
            cursor = 1
            for sequence in self.sequences[1:]:
                if cursor == offset:
                    return "seq(" + ", ".join(token.cpp_semantic for token in sequence) + ")"
                cursor += len(sequence)
            raise ValueError(f"Missing canonical sequence {identifier}")

        terminals: list[str] = []
        alternatives: list[str] = []
        union: dict[str, int] = {}
        for spelling in keys:
            variants = list(non_roman[spelling].items())
            primary = variants[0] if variants else (0, 0)
            begin = len(alternatives)
            for sequence, mask in variants[1:]:
                alternatives.append(f"/* {cpp_u8(spelling)} */ {{{format_mask(mask)}, sequenceId({semantic_sequence(sequence)})}}")
            unrestricted = self._sequence(indic[spelling]) if spelling in indic else 0
            alt_range = f"{{{begin}, {max(len(variants) - 1, 0)}}}"
            terminals.append(f"{{{cpp_u8(spelling)}, {{{format_mask(primary[1])}, sequenceId({semantic_sequence(primary[0])}), sequenceId({semantic_sequence(unrestricted)}), {alt_range}}}}}")
            union[spelling] = len(terminals)
        self._check16(len(terminals), "terminal IDs")
        self._check16(len(alternatives), "source variants")

        graphs = [union]
        graph_tokens: list[dict[str, ReaderToken] | None] = [None]
        readers = []
        for index, script in enumerate(self.scripts):
            if script.category != "latin":
                readers.append(f"/* ScriptId::{script_id_names[index]} */ {{0, 0, {format_mask(1 << index)}}}")
                continue
            normal = {key: self._sequence(token) for key, token in explicit[index].items()}
            fold = {key: self._sequence(token) for key, token in folded[index].items()}
            normal_id = len(graphs)
            graphs.append(normal)
            graph_tokens.append(explicit[index])
            folded_id = normal_id
            if normal != fold:
                folded_id = len(graphs)
                graphs.append(fold)
                graph_tokens.append(folded[index])
            readers.append(f"/* ScriptId::{script_id_names[index]} */ {{{normal_id}, {folded_id}, 0}}")

        # Pool identical output strings. WriterChar stores UTF-8 byte offsets,
        # so the generated C++ table is independent of source-file encoding.
        text_pool: list[str] = []
        text_refs: dict[str, Range] = {}
        pool_bytes = 0

        def text_ref(value: str) -> Range:
            nonlocal pool_bytes
            if value not in text_refs:
                encoded = utf8_key(value)
                text_refs[value] = Range(pool_bytes, len(encoded))
                text_pool.append(f"    {cpp_u8(value)}")
                pool_bytes += len(encoded)
                self._check16(pool_bytes, "writer string pool")
            return text_refs[value]

        char_entries: list[str] = []
        ranges: dict[str, Range] = {}
        writers: list[str] = []
        for i, script in enumerate(self.scripts):
            script_ranges = []
            for kind in CLASSES:
                array = self.chars(script, kind)
                array_key = json.dumps(array, ensure_ascii=False, separators=(",", ":"))
                if array_key not in ranges:
                    begin = len(char_entries)
                    for value in array:
                        ref = text_ref(value)
                        if ref.count > 255:
                            raise ValueError("writer char length exceeds uint8_t")
                        char_entries.append(f"writerChar<writerText>({cpp_u8(value)}, {ref.begin})")
                    self._check16(len(char_entries), "writer entries")
                    ranges[array_key] = Range(begin, len(array))
                item = ranges[array_key]
                script_ranges.append(f"std::span{{writerChars}}.subspan({item.begin}, {item.count})")
            writers.append(f"/* ScriptId::{script_id_names[i]} */ {{ScriptType::{self.script_type(script)}, {str(script.category == 'vedic').lower()}, "
                            f"{{{{{', '.join(script_ranges)}}}}}}}")

        names = {script.name.lower(): i for i, script in enumerate(self.scripts)}
        for i, script in enumerate(self.scripts):
            for alias in script.info.get("aliases", []):
                names.setdefault(alias.lower(), i)
        sorted_names = sorted(names)

        output = ["// GENERATED by tool/generate_headers.py. Do not edit.", "#pragma once", "",
                  '#include "static_script_types.h"', "", "namespace inditrans::static_data {", ""]
        output += [
            "enum class ScriptId : uint8_t {",
            *(f"    {name} = {idx}," for idx, name in enumerate(script_id_names)),
            "};\n",
        ]
        output += ["inline constexpr auto writerText = packUtf8(", *text_pool, ");\n"]
        self._array(output, "WriterChar", "writerChars", char_entries)
        self._array(output, "ScriptWriterMap", "writers", writers)
        self._array(output, "ScriptName", "names",
                    [f'{{"{name}", static_cast<uint16_t>(ScriptId::{script_id_names[names[name]]})}}' for name in sorted_names])
        self._array(output, "ReaderInfo", "readerInfo", readers)
        output += [f"inline constexpr size_t devanagariId = static_cast<size_t>(ScriptId::{script_id_names[devanagari]});",
                   f"inline constexpr size_t tamilId = static_cast<size_t>(ScriptId::{script_id_names[next(i for i, s in enumerate(self.scripts) if s.name == 'tamil')]});",
                   "",
                   f"inline constexpr unsigned sequenceOffsetBits = {SEQUENCE_OFFSET_BITS};\n"]
        seq_entries = [f"    seq({', '.join(token.cpp_semantic for token in sequence)})"
                       for sequence in self.sequences[1:]]
        output += [
            "inline constexpr auto sequencePool = makeSequencePool(\n" + ",\n".join(seq_entries) + "\n);",
            "inline constexpr auto sequenceTokens = sequencePool.tokens;\n",
        ]
        output += [
            "inline constexpr auto canonicalSequences = sequenceLookup(sequencePool, sequenceOffsetBits);",
            "",
            "template <size_t N> consteval uint16_t sequenceId(const TokenSequence<N>& sequence) {",
            "    if constexpr (N == 0) return 0;",
            "    if constexpr (N == 1)",
            "        if (const auto id = canonicalSequences.lookup(sequence[0])) return id;",
            "    for (const auto range : sequencePool.ranges) {",
            "        if (range.count != N) continue;",
            "        bool same = true;",
            "        for (size_t i = 0; i < N; ++i)",
            "            if (sequenceTokens[range.begin + i] != sequence[i]) { same = false; break; }",
            "        if (same) return uint16_t(range.begin | (N << sequenceOffsetBits));",
            "    }",
            "    std::abort();",
            "}\n",
        ]
        self._array(output, "SourceEntry", "sourceMappings", terminals)
        output.append("inline constexpr auto sourceTerminals = sourcePayloads(sourceMappings);\n")
        self._array(output, "SourceVariant", "sourceAlternatives", alternatives)

        capacity = max(1, *(1 + sum(len(utf8_key(key)) for key in graph) for graph in graphs))
        output.append(f"using ReaderIndex = SmallestIndex<{capacity * 2}>;\n")
        for index, graph in enumerate(graphs):
            graph_keys = sorted(graph, key=utf8_key)
            if index == 0:
                output.append("inline constexpr auto readerEntries0 = sourceEntries(sourceMappings);\n")
            else:
                tokens_map = graph_tokens[index]
                mapping_entries = []
                for key in graph_keys:
                    token = tokens_map[key]
                    if token.extra:
                        tok_str = f"seq({token.lead.cpp_semantic}, {', '.join(t.cpp_semantic for t in token.extra)})"
                    else:
                        tok_str = token.lead.cpp_semantic
                    mapping_entries.append(f"{{{cpp_u8(key)}, {tok_str}}}")
                self._array(output, "SemanticMapping", f"romanMappings{index}", mapping_entries)
                output.append(f"inline constexpr std::array<ReaderEntry, {len(graph_keys)}> readerEntries{index} = deriveReaderEntries(sequenceTokens, sequenceOffsetBits, romanMappings{index});\n")
            output.append(f"inline constexpr auto readerTrie{index} = makeStaticTrie<readerEntries{index}, ReaderIndex>();\n")
            output.append(f"inline constexpr auto packedReaderTrie{index} = packTrie<readerTrie{index}>();\n")
        self._array(output, "PackedTrieView<ReaderIndex>", "readerTries",
                    [f"packedReaderTrie{i}.view()" for i in range(len(graphs))])

        tamil_prefixes = [f"    Utf8Key({cpp_u8(p)})," for p in self.constants["TamilPrefixes"]]
        output += [
            "inline constexpr auto tamilPrefixes = std::array {",
            *tamil_prefixes,
            "};\n",
            "inline constexpr auto tamilEntries = deriveTamilEntries(",
            "    readerTrie0,",
            "    sourceTerminals,",
            "    sourceAlternatives,",
            "    sequenceTokens,",
            "    sequenceOffsetBits,",
            "    1u << tamilId,",
            "    tamilPrefixes",
            ");\n",
            "inline constexpr auto tamilTrie = makeStaticTrie<tamilEntries>();",
            "",
            "} // namespace inditrans::static_data",
            "",
        ]

        Path(path).write_text("\n".join(output), encoding="utf-8")
        audit_path = Path("out/static-lookup-collisions.json")
        audit_path.parent.mkdir(parents=True, exist_ok=True)
        audit_path.write_text(json.dumps(self.audit, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
