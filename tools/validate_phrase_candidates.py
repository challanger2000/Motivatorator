#!/usr/bin/env python3
"""Static quality checks for German phrase candidate batches.

This script NEVER generates or combines phrases. It reports structural and style
risks that need curation before candidates enter the runtime PhraseBank.
"""
from __future__ import annotations

import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PHRASE_DIR = ROOT / "src" / "phrases"
FILES = sorted(PHRASE_DIR.glob("CuratedGermanBatch*.inc"))

PHRASE_RE = re.compile(r'^u"(.*)",\s*$')

FORBIDDEN = {
    "habibi", "bruder", "junge", "mann", "meister", "könig", "koenig",
    "chef", "professor", "eier",
}

CONTENT_FORBIDDEN = {
    "gitarre", "gitarren", "bassgitarre", "schlagzeug", "drums", "drum",
    "klavier", "piano", "synth", "synthesizer", "serum", "spire", "dune",
    "studio one", "ableton", "cubase", "logic", "fl studio", "reaper",
    "vst", "vst3", "plugin", "plugins", "techno", "metal", "rock", "house",
}

# A tiny explicit exception list preserves the universal-bank rule while allowing
# deliberately wrong, clueless studio guesses whose incorrect specificity is the joke.
CONTENT_EXCEPTIONS = {
    "die gitarre ist wohl toll verstimmt",
}

WATCH_OPENERS = {
    "okay", "boah", "uff", "verdammt", "scheiße", "scheisse", "alter",
}
WARN_WATCHED_OPENER = 35
HARD_MAX_WATCHED_OPENER = 80


def norm(text: str) -> str:
    text = text.casefold().replace("ß", "ss")
    text = re.sub(r"[^a-z0-9äöü]+", " ", text)
    return " ".join(text.split())


def phrases():
    for path in FILES:
        section = "unknown"
        for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if "// MOTIVATOR" in raw:
                section = "motivator"
            elif "// DEMOTIVATOR" in raw:
                section = "demotivator"
            m = PHRASE_RE.match(raw.strip())
            if m:
                yield path, lineno, section, m.group(1)


def tokens(text: str) -> set[str]:
    return set(norm(text).split())


def main() -> int:
    rows = list(phrases())
    errors = 0
    counts = Counter(section for _, _, section, _ in rows)
    print(f"Files: {len(FILES)}")
    print(f"Candidates: MOTIVATOR={counts['motivator']} DEMOTIVATOR={counts['demotivator']}")

    seen: dict[str, tuple[Path, int]] = {}
    opener_counts = Counter()

    for path, lineno, section, text in rows:
        n = norm(text)
        words = n.split()
        if words:
            opener_counts[words[0]] += 1

        hit = sorted(w for w in FORBIDDEN if re.search(rf"\b{re.escape(norm(w))}\b", n))
        if hit:
            errors += 1
            print(f"ERROR forbidden address {hit}: {path.name}:{lineno}: {text}")

        content_hit = sorted(w for w in CONTENT_FORBIDDEN if re.search(rf"\b{re.escape(norm(w))}\b", n))
        if content_hit and n not in CONTENT_EXCEPTIONS:
            errors += 1
            print(f"ERROR concrete content {content_hit}: {path.name}:{lineno}: {text}")

        if n in seen:
            errors += 1
            old_path, old_line = seen[n]
            print(f"ERROR duplicate: {path.name}:{lineno} == {old_path.name}:{old_line}: {text}")
        else:
            seen[n] = (path, lineno)

        wc = len(words)
        if wc > 16:
            print(f"WARN long ({wc} words): {path.name}:{lineno}: {text}")

    print("\nFrequent watched openers:")
    for opener in sorted(WATCH_OPENERS):
        count = opener_counts[opener]
        if count:
            print(f"  {opener}: {count}")
        if count > HARD_MAX_WATCHED_OPENER:
            errors += 1
            print(f"ERROR opener '{opener}' used {count} times; hard max {HARD_MAX_WATCHED_OPENER}")
        elif count > WARN_WATCHED_OPENER:
            print(f"WARN opener '{opener}' used {count} times; editorial target <= {WARN_WATCHED_OPENER}")

    for i, (p1, l1, s1, t1) in enumerate(rows):
        a = tokens(t1)
        if len(a) < 5:
            continue
        for p2, l2, s2, t2 in rows[i + 1:]:
            b = tokens(t2)
            if len(b) < 5:
                continue
            overlap = len(a & b) / len(a | b)
            if overlap >= 0.72:
                print(f"WARN near-duplicate ({overlap:.2f}): {p1.name}:{l1} <-> {p2.name}:{l2}")

    for section in ("motivator", "demotivator"):
        if counts[section] != 500:
            errors += 1
            print(f"ERROR {section} has {counts[section]} candidates; final German target is exactly 500")

    if errors:
        print(f"\nFAILED: {errors} issue(s) require curation.")
        return 1
    print("\nPASS: structural/style checks clean. Funniness still requires editorial review.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
