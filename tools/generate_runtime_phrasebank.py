#!/usr/bin/env python3
"""Generate the runtime C++ phrase bank from the curated German/English batches.

HARD RULE: this script only pairs already-written standalone comments by batch and
position. It never composes, expands, rewrites or synthesizes phrase text.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

PHRASE_RE = re.compile(r'^u"(.*)",\s*$')
BATCH_RE = re.compile(r'Batch(\d+)\.inc$')


def read_batch(path: Path) -> dict[str, list[str]]:
    out = {"motivator": [], "demotivator": []}
    section: str | None = None
    for raw in path.read_text(encoding="utf-8").splitlines():
        if "// MOTIVATOR" in raw:
            section = "motivator"
            continue
        if "// DEMOTIVATOR" in raw:
            section = "demotivator"
            continue
        m = PHRASE_RE.match(raw.strip())
        if m and section:
            # Keep the literal contents exactly as curated; they are already C++ escaped.
            out[section].append(m.group(1))
    return out


def indexed_files(root: Path, language: str) -> dict[int, Path]:
    result: dict[int, Path] = {}
    for path in (root / "src" / "phrases").glob(f"Curated{language}Batch*.inc"):
        m = BATCH_RE.search(path.name)
        if not m:
            continue
        result[int(m.group(1))] = path
    return result


def cpp_literal(body: str) -> str:
    return f'u"{body}"'


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    root = args.root.resolve()
    de_files = indexed_files(root, "German")
    en_files = indexed_files(root, "English")
    if set(de_files) != set(en_files):
        raise SystemExit(f"German/English batch mismatch: DE={sorted(de_files)} EN={sorted(en_files)}")
    if sorted(de_files) != list(range(1, 11)):
        raise SystemExit(f"Expected batches 01..10, got {sorted(de_files)}")

    motivator: list[tuple[str, str]] = []
    demotivator: list[tuple[str, str]] = []
    for batch in sorted(de_files):
        de = read_batch(de_files[batch])
        en = read_batch(en_files[batch])
        for section, target in (("motivator", motivator), ("demotivator", demotivator)):
            if len(de[section]) != 50 or len(en[section]) != 50:
                raise SystemExit(
                    f"Batch {batch:02d} {section}: expected 50/50, got DE={len(de[section])} EN={len(en[section])}"
                )
            target.extend(zip(de[section], en[section]))

    if len(motivator) != 500 or len(demotivator) != 500:
        raise SystemExit(f"Expected 500/500, got {len(motivator)}/{len(demotivator)}")

    lines: list[str] = [
        "#pragma once",
        "",
        "#include \"pluginterfaces/vst/vsttypes.h\"",
        "#include <array>",
        "#include <cstddef>",
        "",
        "namespace MotivatoratorPhrases {",
        "",
        "struct Phrase {",
        "    const Steinberg::Vst::TChar* de;",
        "    const Steinberg::Vst::TChar* en;",
        "};",
        "",
        "// AUTO-GENERATED from curated standalone phrase batches.",
        "// One array entry = one complete German comment plus its English localization.",
        "inline constexpr std::array<Phrase, 500> kMotivator {{",
    ]
    for de, en in motivator:
        lines.append(f"    {{STR16({cpp_literal(de)}), STR16({cpp_literal(en)})}},")
    lines.extend(["}};", "", "inline constexpr std::array<Phrase, 500> kDemotivator {{"])
    for de, en in demotivator:
        lines.append(f"    {{STR16({cpp_literal(de)}), STR16({cpp_literal(en)})}},")
    lines.extend([
        "}};",
        "",
        "inline constexpr std::size_t kMotivatorCount = kMotivator.size();",
        "inline constexpr std::size_t kDemotivatorCount = kDemotivator.size();",
        "inline constexpr std::size_t kPhraseCount = kMotivatorCount + kDemotivatorCount;",
        "",
        "static_assert(kMotivatorCount == 500, \"Motivator runtime bank must contain 500 phrases\");",
        "static_assert(kDemotivatorCount == 500, \"Demotivator runtime bank must contain 500 phrases\");",
        "",
        "} // namespace MotivatoratorPhrases",
        "",
    ])

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"Generated {args.output}: 500 MOTIVATOR + 500 DEMOTIVATOR bilingual pairs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
