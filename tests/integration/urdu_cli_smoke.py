#!/usr/bin/env python3
"""CLI opt-in Urdu pronunciation smoke test; no external datasets required."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CLI = ROOT / "out/inditrans"
PACK = ROOT / "demo/urdu-example.iul"
SOURCE = ROOT / "scripts/urdu/demo-lexicon.json"

def run(*args, input_text=None):
    return subprocess.run([str(CLI), *args], cwd=ROOT, input=input_text,
                          capture_output=True, text=True, encoding="utf-8")

def main():
    with tempfile.TemporaryDirectory(prefix="urdu-demo-pack-") as directory:
        regenerated = Path(directory) / "demo.iul"
        subprocess.run(["python3", "scripts/urdu/build_lexicon.py", str(SOURCE), str(regenerated),
                        "--binary"], cwd=ROOT, check=True)
        assert regenerated.read_bytes() == PACK.read_bytes(), "The checked-in demo lexicon must be reproducible."
    source = "جیسی کرنی ویسی بھرنی"
    base = run("-f", "urdu", "-t", "indlish", source)
    assert base.returncode == 0, base.stderr
    phonetic = run("-f", "urdu", "-t", "indlish", "--urdu-lexicon", str(PACK), source)
    assert phonetic.returncode == 0, phonetic.stderr
    assert phonetic.stdout.strip() == "jaisI karnI vaisI bharnI", phonetic.stdout
    assert base.stdout != phonetic.stdout
    punctuation = run("-f", "urdu", "-t", "indlish", "۔")
    full = run("-f", "urdu", "-t", "indlish", "-l", str(PACK), source + "۔")
    assert full.stdout.strip() == phonetic.stdout.strip() + punctuation.stdout.strip(), full.stdout
    strict = run("-f", "urdu", "-t", "indlish", "-o", "DisableInference",
                 "-l", str(PACK), source)
    assert strict.returncode == 0 and strict.stdout == base.stdout, strict.stderr
    protected = run("-f", "urdu", "-t", "indlish", "-l", str(PACK), "##جیسی## جیسی")
    assert protected.stdout.strip() == "جیسی jaisI", protected.stdout
    interactive = run("-f", "urdu", "-t", "indlish", input_text=f"-l {PACK}\n{source}\nquit\n")
    assert interactive.returncode == 0 and "jaisI karnI vaisI bharnI" in interactive.stdout, interactive.stdout
    invalid = run("-f", "urdu", "-t", "indlish", "-l", str(SOURCE), source)
    assert invalid.returncode != 0 and "Invalid IUL1" in invalid.stderr, invalid.stderr
    corpus = ROOT / "demo/urdu-pronunciation.iul"
    assert corpus.stat().st_size < 3_000_000
    corpus_run = run("-f", "urdu", "-t", "indlish", "-l", str(corpus), source)
    assert corpus_run.returncode == 0 and corpus_run.stdout == phonetic.stdout, corpus_run.stderr
    unknown = run("-f", "urdu", "-t", "indlish", "-l", str(corpus), "زززززززززززز")
    unknown_legacy = run("-f", "urdu", "-t", "indlish", "زززززززززززز")
    assert unknown.stdout == unknown_legacy.stdout
    print("Urdu demo lexicon, CLI opt-in, strict mode, punctuation and protected spans: passed")

if __name__ == "__main__":
    main()
