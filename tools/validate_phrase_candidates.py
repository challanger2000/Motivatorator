#!/usr/bin/env python3
"""Static quality checks for German phrase candidate batches.

This script NEVER generates or combines phrases. It only reports candidates that
need human curation before they can enter the runtime PhraseBank.
"""
from __future__ import annotations

import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PHRASE_DIR = ROOT / "src" / "phrases"
FILES = sorted(PHRASE_DIR.glob("CuratedGermanBatch*.inc"))

PHRASE_RE = re.compile(r'^u"(.*)",\s*$')

# Direct-address words we deliberately avoid so every comment remains neutral.
FORBIDDEN = {
    "habibi", "bruder", "junge", "mann", "meister", "könig", "koenig",
    "chef", "professor", "eier",
}

# These are not automatic failures, but too many make the bank sound formulaic.
WATCH_OPENERS = {
    "okay", "boah", "uff", "verdammt", "scheiße", "scheisse", "alter",
}


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
            print(f"ERROR forbidden {hit}: {path.name}:{lineno}: {text}")

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

    # Raw batches are allowed to contain more than the final target. They must
    # never silently contain fewer than the first agreed curation target.
    for section in ("motivator", "demotivator"):
        if counts[section] < 500:
            errors += 1
            print(f"ERROR only {counts[section]} {section} candidates; target is at least 500")

    if errors:
        print(f"\nFAILED: {errors} issue(s) require curation.")
        return 1
    print("\nPASS: structural checks clean. Semantic/funniness review is still required.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
