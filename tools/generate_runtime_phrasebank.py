#!/usr/bin/env python3
"""Generate the deliberately small bilingual phrase bank for Motivator Demo."""
from __future__ import annotations

import argparse
from pathlib import Path

MOTIVATOR = [
    ("Das klingt sau geil.", "This sounds fucking awesome."),
    ("Du hast es echt gut drauf.", "You really know what you're doing."),
    ("Deine Musik ist der Wahnsinn.", "Your music is insane."),
    ("Das wird definitiv ein Hit.", "This is definitely going to be a hit."),
    ("Du bist sooo gut!", "You're sooo good!"),
    ("Alter, was für ein Brett!", "Damn, this hits hard!"),
    ("Wie geil ist das denn bitte?!", "How fucking good is this?!"),
    ("Das klingt verdammt professionell.", "This sounds damn professional."),
    ("Genau so muss das klingen!", "That's exactly how it should sound!"),
    ("Du bist Profi, das merkt man.", "You can tell you're a pro."),
]

DEMOTIVATOR = [
    ("Das klingt wie ein Unfall mit Ansage.", "This sounds like a disaster waiting to happen."),
    ("Vielleicht ist Musik einfach nicht dein Ding.", "Maybe music just isn't your thing."),
    ("Das war bestimmt besser, bevor du angefangen hast.", "I'm sure this was better before you started."),
    ("Selbst deine DAW schämt sich gerade.", "Even your DAW is embarrassed right now."),
    ("Mach ruhig weiter. Irgendwann wird's bestimmt noch schlimmer.", "Keep going. I'm sure it'll get even worse eventually."),
    ("Das klingt erstaunlich teuer für so wenig Ergebnis.", "That sounds surprisingly expensive for so little result."),
    ("Hast du das mit Absicht so gemacht?", "Did you actually mean to do that?"),
    ("Dein Talent hält sich heute auffällig zurück.", "Your talent is keeping a suspiciously low profile today."),
    ("Das braucht keinen Mix. Das braucht ein Wunder.", "This doesn't need a mix. It needs a miracle."),
    ("Speichern würde ich mir an deiner Stelle sparen.", "If I were you, I wouldn't bother saving this."),
]


def esc(text: str) -> str:
    return text.replace('\\', '\\\\').replace('"', '\\"')


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, required=True)  # kept for build-interface compatibility
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    lines = [
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
        "// Motivator Demo showcase: 10 positive + 10 negative bilingual phrases.",
        "inline constexpr std::array<Phrase, 10> kMotivator {{",
    ]
    for de, en in MOTIVATOR:
        lines.append(f'    {{STR16("{esc(de)}"), STR16("{esc(en)}")}},')
    lines += ["}};", "", "inline constexpr std::array<Phrase, 10> kDemotivator {{"]
    for de, en in DEMOTIVATOR:
        lines.append(f'    {{STR16("{esc(de)}"), STR16("{esc(en)}")}},')
    lines += [
        "}};",
        "",
        "inline constexpr std::size_t kMotivatorCount = kMotivator.size();",
        "inline constexpr std::size_t kDemotivatorCount = kDemotivator.size();",
        "inline constexpr std::size_t kPhraseCount = kMotivatorCount + kDemotivatorCount;",
        "",
        "static_assert(kMotivatorCount == 10, \"Demo must contain exactly 10 motivator phrases\");",
        "static_assert(kDemotivatorCount == 10, \"Demo must contain exactly 10 demotivator phrases\");",
        "",
        "} // namespace MotivatoratorPhrases",
        "",
    ]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"Generated {args.output}: 10 MOTIVATOR + 10 DEMOTIVATOR bilingual demo pairs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
