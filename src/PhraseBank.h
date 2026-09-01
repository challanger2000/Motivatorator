#pragma once

#include <array>
#include <cstddef>

namespace MotivatoratorPhrases {

struct Phrase {
    const char* de;
    const char* en;
};

// Keep both languages native in spirit rather than literal word-for-word translations.
// The bank is intentionally compile-time/static: no allocations, no disk I/O and no CPU work while idle.
inline constexpr std::array<Phrase, 24> kMotivator {{
    {"Nicht anfassen! Genau so lassen!", "Don't touch it. That's the one."},
    {"Okay ... DAS war gerade verdammt gut.", "Okay. That was ridiculously good."},
    {"Der Groove hat gerade eine Betriebserlaubnis bekommen.", "That groove just got officially licensed."},
    {"Wenn der Drop noch besser wird, brauchen wir eine Baugenehmigung.", "If that drop gets any bigger, we'll need planning permission."},
    {"Ich weiss nicht, was du gemacht hast. Aber mach mehr davon.", "I don't know what you did. Do more of it."},
    {"Das Riff darf bleiben.", "That riff has earned permanent residency."},
    {"Ja. Genau dieser Sound.", "Yes. That sound. Keep it."},
    {"Der Bass sitzt. Finger weg.", "The bass is sitting perfectly. Hands off."},
    {"Das klingt gefaehrlich gut.", "That sounds dangerously good."},
    {"Du hast gerade aus Versehen Geschmack bewiesen.", "You accidentally demonstrated excellent taste."},
    {"Der Refrain weiss genau, was er tut.", "That chorus knows exactly what it's doing."},
    {"Das Arrangement atmet. Selten genug.", "The arrangement can breathe. Miracles happen."},
    {"Mehr davon. Sofort.", "More of that. Immediately."},
    {"Das knallt. Sachlich festgestellt.", "It hits. That's a technical assessment."},
    {"Heute gewinnt die Musik.", "Today, the music wins."},
    {"Der Sound hat Rueckgrat.", "That sound has a spine."},
    {"Das ist kein Glueck mehr. Das kannst du.", "That's not luck anymore. You can actually do this."},
    {"Speichern. Bevor du auf Ideen kommst.", "Save it before you get creative again."},
    {"Der Part funktioniert. Unfassbar, aber wahr.", "The part works. Against all odds."},
    {"Dein Mix schaut mich gerade selbstbewusst an.", "Your mix is making confident eye contact."},
    {"Das war die richtige Entscheidung.", "That was the right call."},
    {"Der Synth darf heute vorne sitzen.", "That synth has earned the front seat."},
    {"Der Groove lebt. Lass ihn leben.", "The groove is alive. Let it live."},
    {"Okay, jetzt wird es ernsthaft gut.", "Okay, this is becoming seriously good."}
}};

inline constexpr std::array<Phrase, 24> kDemotivator {{
    {"Noch ein EQ. Bestimmt liegt es diesmal daran.", "Another EQ. Surely that's the problem this time."},
    {"Der Drop kommt? Danke fuer die Warnung.", "The drop is coming? Thanks for the warning."},
    {"Das Mastering koennen wir uns sparen. Das Verbrechen ist bereits passiert.", "Skip the mastering. The crime has already happened."},
    {"Ich wuerde eine Pause machen. Dein Song hat sich eine verdient.", "Take a break. Your song has earned one."},
    {"Das Gute an deinem Mix ist, dass er irgendwann vorbei ist.", "The best thing about this mix is that it eventually ends."},
    {"Du brauchst keinen neuen Synthesizer. Du brauchst eine neue Idee.", "You don't need another synth. You need another idea."},
    {"Speicher das ruhig. Die Polizei braucht Beweise.", "Go ahead and save it. The police will need evidence."},
    {"Vier Gitarrenspuren. Und trotzdem keine Idee.", "Four guitar tracks. Still no idea."},
    {"Der Bass und die Kick haben sich offenbar getrennt.", "The bass and kick appear to have filed for divorce."},
    {"Das Panorama ist mutig. Leider.", "The stereo image is brave. Unfortunately."},
    {"Noch lauter macht es nicht richtiger.", "Louder is not the same as better."},
    {"Der Hall ist gross. Die Idee eher nicht.", "Huge reverb. Tiny idea."},
    {"Das Timing nennt man vermutlich kuenstlerische Freiheit.", "I assume that timing is artistic freedom."},
    {"Der Refrain sucht noch nach seinem Zweck.", "The chorus is still looking for a purpose."},
    {"Ich habe schlechteres gehoert. Heute aber noch nicht.", "I've heard worse. Just not today."},
    {"Dieser Sound braucht keinen Kompressor. Er braucht Hilfe.", "That sound doesn't need compression. It needs help."},
    {"Zumachen aendert den Mix auch nicht.", "Closing the window won't fix the mix."},
    {"Ach. Du schon wieder.", "Oh. You again."},
    {"Na endlich. Genug gelogen.", "Finally. Enough pretending."},
    {"Der Song hat Fragen. Ich leider auch.", "The song has questions. So do I."},
    {"Interessante Entscheidung. Wirklich sehr interessant.", "Interesting decision. Extremely interesting."},
    {"Der Groove stolpert, aber wenigstens konsequent.", "The groove is stumbling, but at least consistently."},
    {"Das klingt teuer. Nicht gut. Nur teuer.", "It sounds expensive. Not good. Just expensive."},
    {"Vielleicht war Version 17 doch die beste.", "Maybe version 17 really was the best one."}
}};

constexpr std::size_t kMotivatorCount = kMotivator.size();
constexpr std::size_t kDemotivatorCount = kDemotivator.size();
constexpr std::size_t kPhraseCount = kMotivatorCount + kDemotivatorCount;

} // namespace MotivatoratorPhrases
