#!/usr/bin/env python3
"""Reproducible, strictly pinned, experimental Urdu IUL1 lexicon builder.

Inputs are independently exported WikiPron and Wiktextract datasets both
ultimately derived from English Wiktionary. Agreement is a conservative
extraction screen, NOT independently verified pronunciation accuracy.
"""
import argparse
import hashlib
import json
from pathlib import Path

from build_lexicon import ROOT, binary_pack, build, normalize
from import_wiktextract import convert as wiktextract_convert
from import_wikipron import convert as wikipron_convert

WIKIPRON_SHA256 = "5530b71670423fc810e245680a9077ca3c72c79de0cdf6d2df21bbec5130d1e5"
WIKTEXTRACT_SHA256 = "6fef1d227ce1cbc309d38f7475e502bc0f6335dc63f4133c7757c44ed2271ec9"
MIN_CONCORDANT = 4_000
MAX_CONCORDANT = 5_000


def sha(data):
    return hashlib.sha256(data).hexdigest()


def compile_lexicon(wikipron, wiktextract, demo, *, verify_sources=True, min_concordant=MIN_CONCORDANT):
    hashes = {"wikipron": sha(wikipron), "wiktextract": sha(wiktextract)}
    if verify_sources and (hashes["wikipron"] != WIKIPRON_SHA256
                           or hashes["wiktextract"] != WIKTEXTRACT_SHA256):
        raise ValueError("Source SHA-256 mismatch: refusing to build an unpinned pronunciation pack")

    second, rejected_wiktextract = wiktextract_convert(wiktextract)
    confirmed = {entry["word"]: entry["readings"] for entry in second["entries"]}
    first, rejected_wikipron = wikipron_convert(wikipron)
    screened, rejected_concordance = wikipron_convert(wikipron, confirmed)
    candidate_count = len(screened["entries"])
    if verify_sources and not min_concordant <= candidate_count <= MAX_CONCORDANT:
        raise ValueError(f"Unexpected concordant count: {candidate_count}")

    # Project-authored examples are explicit overrides and are not a claim of
    # corpus-wide verification. Preserve the existing proverb regression.
    merged = {entry["word"]: entry for entry in screened["entries"]}
    override_count = 0
    for entry in demo["entries"]:
        word = normalize(entry["word"])
        if word in merged and merged[word]["readings"] != entry["readings"]:
            override_count += 1
        merged[word] = {"word": word, "readings": entry["readings"]}

    source = {
        "format": 1,
        "version": "urdu-wiktionary-concordant-2026-10-01-experimental-1",
        "provenance": (
            "Experimental Urdu word pronunciations adapted from English Wiktionary "
            "contributors via WikiPron (Lee et al., LREC 2020) and Wiktextract "
            "(Tatu Ylonen, LREC 2022), CC BY-SA 4.0; "
            "source WikiPron SHA-256 " + hashes["wikipron"] + "; "
            "Wiktextract SHA-256 " + hashes["wiktextract"] +
            "; additionally includes 5 Inditrans-authored demonstration readings. "
            "Unreviewed: concordance does not prove pronunciation correctness."
        ),
        "entries": list(merged.values()),
    }
    table = json.loads((ROOT / "scripts/script_data.json").read_text(encoding="utf-8"))
    canonical = build(source, table)
    # Fully specify the source digest for the IUL1 header without requiring
    # a verbose JSON source artifact to ship with the binary.
    source_bytes = (json.dumps(source, ensure_ascii=False, sort_keys=True,
                               separators=(",", ":")) + "\n").encode("utf-8")
    canonical["source_sha256"] = sha(source_bytes)
    pack = binary_pack(canonical)
    report = {
        "format": 1,
        "status": "EXPERIMENTAL / NOT LINGUISTICALLY REVIEWED",
        "license": "CC-BY-SA-4.0 (Wiktionary-derived pronunciation data)",
        "license_url": "https://creativecommons.org/licenses/by-sa/4.0/",
        "sources": {
            "wikipron": {"url": "https://github.com/CUNY-CL/wikipron/blob/1b6fce9d894e38eb65431676e4b2fcfc3db19db8/data/scrape/tsv/urd_arab_broad.tsv",
                          "sha256": hashes["wikipron"]},
            "wiktextract": {"url": "https://kaikki.org/dictionary/Urdu/kaikki.org-dictionary-Urdu.jsonl",
                            "sha256": hashes["wiktextract"]},
        },
        "wiki_pron_candidate_entries": len(first["entries"]),
        "wiktextract_candidate_entries": len(second["entries"]),
        "concordant_entries": candidate_count,
        "manual_demo_entries": len(demo["entries"]),
        "manual_demo_overrides": override_count,
        "packed_entries": len(canonical["entries"]),
        "pack_bytes": len(pack),
        "pack_sha256": sha(pack),
        "unpacked_canonical_source_sha256": canonical["source_sha256"],
        "limitations": (
            "Both extraction sources ultimately derive from Wiktionary; no independent "
            "human gold labels, contextual homograph disambiguation, or pronunciation "
            "accuracy claim. Unlisted spellings retain ordinary literal fallback."
        ),
        "rejected": {
            "wiktextract": dict(sorted(rejected_wiktextract.items())),
            "wikipron": dict(sorted(rejected_wikipron.items())),
            "concordance": dict(sorted(rejected_concordance.items())),
        },
    }
    return pack, report, canonical


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wikipron", type=Path, required=True)
    parser.add_argument("--wiktextract", type=Path, required=True)
    parser.add_argument("--demo-source", type=Path, default=ROOT / "scripts/urdu/demo-lexicon.json")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--candidate-json", type=Path, help="optional validated canonical JSON output")
    parser.add_argument("--allow-unpinned-test-data", action="store_true", help="ONLY for unit test fixtures")
    args = parser.parse_args()

    demo = json.loads(args.demo_source.read_text(encoding="utf-8"))
    pack, report, candidate = compile_lexicon(args.wikipron.read_bytes(), args.wiktextract.read_bytes(),
                                                demo, verify_sources=not args.allow_unpinned_test_data)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(pack)
    args.report.write_text(json.dumps(report, ensure_ascii=False, sort_keys=True, indent=2) + "\n",
                           encoding="utf-8")
    if args.candidate_json:
        args.candidate_json.write_text(
            json.dumps(candidate, ensure_ascii=False, separators=(",", ":")) + "\n",
            encoding="utf-8")
    print(f"Urdu experimental lexicon: {report['packed_entries']} entries, {len(pack)} bytes, "
          f"sha256 {report['pack_sha256']}")


if __name__ == "__main__":
    main()
