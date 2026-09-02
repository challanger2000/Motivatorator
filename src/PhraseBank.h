#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace MotivatoratorPhrases {

constexpr std::size_t kMotivatorCount = 1000;
constexpr std::size_t kDemotivatorCount = 1000;
constexpr std::size_t kPhraseCount = kMotivatorCount + kDemotivatorCount;

using Part = const char16_t*;

inline constexpr std::array<Part, 20> kPosOpenDE {{
    u"Okay.", u"Ja!", u"Verdammt.", u"Wow.", u"Respekt.", u"Ganz ehrlich:",
    u"Ich sag's dir:", u"Alter.", u"Moment mal.", u"Also gut:", u"Ich geb's zu:",
    u"Keine Ahnung wie, aber", u"Das hier?", u"Na bitte.", u"Sieh dich an.",
    u"Was zur Hölle?", u"Jetzt hör mir zu:", u"Ich werde nervös.", u"Unverschämt.",
    u"Das ist nicht normal."
}};
inline constexpr std::array<Part, 20> kPosOpenEN {{
    u"Okay.", u"Yes!", u"Damn.", u"Wow.", u"Respect.", u"Seriously:",
    u"I'm telling you:", u"Damn.", u"Wait a second.", u"All right:", u"I'll admit it:",
    u"No idea how, but", u"This?", u"There we go.", u"Look at you.",
    u"What the hell?", u"Listen to me:", u"I'm getting nervous.", u"Ridiculous.",
    u"This is not normal."
}};
inline constexpr std::array<Part, 10> kPosCoreDE {{
    u"Das klingt fantastisch.", u"Du bist sooo gut!", u"Das ist richtig stark.",
    u"Das sitzt verdammt gut.", u"Das funktioniert einfach.", u"Das hat richtig Charakter.",
    u"Das klingt gefährlich gut.", u"Du hast es wirklich drauf.", u"Das ist absurd überzeugend.",
    u"Das klingt wie eine verdammt gute Entscheidung."
}};
inline constexpr std::array<Part, 10> kPosCoreEN {{
    u"That sounds fantastic.", u"You're sooo good!", u"That's seriously strong.",
    u"That sits ridiculously well.", u"That just works.", u"That has real character.",
    u"That sounds dangerously good.", u"You really know what you're doing.", u"That's absurdly convincing.",
    u"That sounds like a damn good decision."
}};
inline constexpr std::array<Part, 5> kPosCloseDE {{
    u"Finger weg.", u"Genau so!", u"Speichern. Sofort.", u"Mehr davon!", u"Nicht kaputtmachen."
}};
inline constexpr std::array<Part, 5> kPosCloseEN {{
    u"Hands off.", u"Exactly like that!", u"Save it. Now.", u"More of that!", u"Don't ruin it."
}};

inline constexpr std::array<Part, 20> kNegOpenDE {{
    u"Oh Gott.", u"Aha.", u"Okay.", u"Interessant.", u"Respekt.", u"Beeindruckend.",
    u"Ganz ehrlich:", u"Ich sag's ungern:", u"Moment mal.", u"Na toll.", u"Ach schön.",
    u"Nur damit wir uns verstehen:", u"Keine Panik.", u"Du machst wirklich weiter.",
    u"Ich hatte keine Erwartungen.", u"Das ist jetzt unangenehm.", u"Wo soll ich anfangen?",
    u"Ich schaue kurz weg.", u"Das bleibt unter uns.", u"Bitte sag, das war Absicht."
}};
inline constexpr std::array<Part, 20> kNegOpenEN {{
    u"Oh God.", u"Ah.", u"Okay.", u"Interesting.", u"Respect.", u"Impressive.",
    u"Seriously:", u"Hate to say it:", u"Wait a second.", u"Great.", u"Oh lovely.",
    u"Just so we're clear:", u"Don't panic.", u"You're really still going.",
    u"I had no expectations.", u"This is getting awkward.", u"Where do I even start?",
    u"I'll look away for a moment.", u"This stays between us.", u"Please say that was intentional."
}};
inline constexpr std::array<Part, 10> kNegCoreDE {{
    u"Das war keine Verbesserung.", u"Das klingt jetzt nur anders schlecht.",
    u"Das hat wirklich niemand verlangt.", u"Das war vorher weniger schlimm.",
    u"Du hast es nicht verbessert. Nur verändert.", u"Die Stille davor war besser.",
    u"Das ist erstaunlich konsequent daneben.", u"Das überzeugt nur den Papierkorb.",
    u"Das klingt nach einer Fehlentscheidung.", u"Das macht die Pause danach zum Highlight."
}};
inline constexpr std::array<Part, 10> kNegCoreEN {{
    u"That was not an improvement.", u"Now it just sounds differently bad.",
    u"Absolutely nobody asked for that.", u"It was less bad before.",
    u"You didn't improve it. You only changed it.", u"The silence before it was better.",
    u"That's impressively consistently wrong.", u"Only the trash can is convinced.",
    u"That sounds like a bad decision.", u"That makes the silence after it the highlight."
}};
inline constexpr std::array<Part, 5> kNegCloseDE {{
    u"Mach's rückgängig.", u"Bitte hör auf.", u"Die Polizei braucht Beweise.",
    u"Niemand muss davon erfahren.", u"Wir nennen es einfach Kunst."
}};
inline constexpr std::array<Part, 5> kNegCloseEN {{
    u"Undo it.", u"Please stop.", u"The police will need evidence.",
    u"Nobody needs to know.", u"We'll just call it art."
}};

inline std::u16string buildPhrase(bool positive, bool english, std::size_t index) {
    index %= 1000;
    const std::size_t opener = index / 50;
    const std::size_t core = (index / 5) % 10;
    const std::size_t close = index % 5;

    const auto& opens = positive
        ? (english ? kPosOpenEN : kPosOpenDE)
        : (english ? kNegOpenEN : kNegOpenDE);
    const auto& cores = positive
        ? (english ? kPosCoreEN : kPosCoreDE)
        : (english ? kNegCoreEN : kNegCoreDE);
    const auto& closes = positive
        ? (english ? kPosCloseEN : kPosCloseDE)
        : (english ? kNegCloseEN : kNegCloseDE);

    std::u16string result(opens[opener]);
    result += u" ";
    result += cores[core];
    result += u" ";
    result += closes[close];
    return result;
}

} // namespace MotivatoratorPhrases
