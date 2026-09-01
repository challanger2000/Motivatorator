#pragma once

#include "pluginterfaces/vst/vsttypes.h"
#include <array>
#include <cstddef>

namespace MotivatoratorPhrases {

struct Phrase {
    const Steinberg::Vst::TChar* de;
    const Steinberg::Vst::TChar* en;
};

// Compile-time/static phrase bank: no allocation, file I/O or background work while idle.
inline constexpr std::array<Phrase, 24> kMotivator {{
    {STR16("Nicht anfassen! Genau so lassen!"), STR16("Don't touch it. That's the one.")},
    {STR16("Okay ... DAS war gerade verdammt gut."), STR16("Okay. That was ridiculously good.")},
    {STR16("Der Groove hat gerade eine Betriebserlaubnis bekommen."), STR16("That groove just got officially licensed.")},
    {STR16("Wenn der Drop noch besser wird, brauchen wir eine Baugenehmigung."), STR16("If that drop gets any bigger, we'll need planning permission.")},
    {STR16("Ich weiß nicht, was du gemacht hast. Aber mach mehr davon."), STR16("I don't know what you did. Do more of it.")},
    {STR16("Das Riff darf bleiben."), STR16("That riff has earned permanent residency.")},
    {STR16("Ja. Genau dieser Sound."), STR16("Yes. That sound. Keep it.")},
    {STR16("Der Bass sitzt. Finger weg."), STR16("The bass is sitting perfectly. Hands off.")},
    {STR16("Das klingt gefährlich gut."), STR16("That sounds dangerously good.")},
    {STR16("Du hast gerade aus Versehen Geschmack bewiesen."), STR16("You accidentally demonstrated excellent taste.")},
    {STR16("Der Refrain weiß genau, was er tut."), STR16("That chorus knows exactly what it's doing.")},
    {STR16("Das Arrangement atmet. Selten genug."), STR16("The arrangement can breathe. Miracles happen.")},
    {STR16("Mehr davon. Sofort."), STR16("More of that. Immediately.")},
    {STR16("Das knallt. Sachlich festgestellt."), STR16("It hits. That's a technical assessment.")},
    {STR16("Heute gewinnt die Musik."), STR16("Today, the music wins.")},
    {STR16("Der Sound hat Rückgrat."), STR16("That sound has a spine.")},
    {STR16("Das ist kein Glück mehr. Das kannst du."), STR16("That's not luck anymore. You can actually do this.")},
    {STR16("Speichern. Bevor du auf Ideen kommst."), STR16("Save it before you get creative again.")},
    {STR16("Der Part funktioniert. Unfassbar, aber wahr."), STR16("The part works. Against all odds.")},
    {STR16("Dein Mix schaut mich gerade selbstbewusst an."), STR16("Your mix is making confident eye contact.")},
    {STR16("Das war die richtige Entscheidung."), STR16("That was the right call.")},
    {STR16("Der Synth darf heute vorne sitzen."), STR16("That synth has earned the front seat.")},
    {STR16("Der Groove lebt. Lass ihn leben."), STR16("The groove is alive. Let it live.")},
    {STR16("Okay, jetzt wird es ernsthaft gut."), STR16("Okay, this is becoming seriously good.")}
}};

inline constexpr std::array<Phrase, 24> kDemotivator {{
    {STR16("Noch ein EQ. Bestimmt liegt es diesmal daran."), STR16("Another EQ. Surely that's the problem this time.")},
    {STR16("Der Drop kommt? Danke für die Warnung."), STR16("The drop is coming? Thanks for the warning.")},
    {STR16("Das Mastering können wir uns sparen. Das Verbrechen ist bereits passiert."), STR16("Skip the mastering. The crime has already happened.")},
    {STR16("Ich würde eine Pause machen. Dein Song hat sich eine verdient."), STR16("Take a break. Your song has earned one.")},
    {STR16("Das Gute an deinem Mix ist, dass er irgendwann vorbei ist."), STR16("The best thing about this mix is that it eventually ends.")},
    {STR16("Du brauchst keinen neuen Synthesizer. Du brauchst eine neue Idee."), STR16("You don't need another synth. You need another idea.")},
    {STR16("Speicher das ruhig. Die Polizei braucht Beweise."), STR16("Go ahead and save it. The police will need evidence.")},
    {STR16("Vier Gitarrenspuren. Und trotzdem keine Idee."), STR16("Four guitar tracks. Still no idea.")},
    {STR16("Der Bass und die Kick haben sich offenbar getrennt."), STR16("The bass and kick appear to have filed for divorce.")},
    {STR16("Das Panorama ist mutig. Leider."), STR16("The stereo image is brave. Unfortunately.")},
    {STR16("Noch lauter macht es nicht richtiger."), STR16("Louder is not the same as better.")},
    {STR16("Der Hall ist groß. Die Idee eher nicht."), STR16("Huge reverb. Tiny idea.")},
    {STR16("Das Timing nennt man vermutlich künstlerische Freiheit."), STR16("I assume that timing is artistic freedom.")},
    {STR16("Der Refrain sucht noch nach seinem Zweck."), STR16("The chorus is still looking for a purpose.")},
    {STR16("Ich habe schlechteres gehört. Heute aber noch nicht."), STR16("I've heard worse. Just not today.")},
    {STR16("Dieser Sound braucht keinen Kompressor. Er braucht Hilfe."), STR16("That sound doesn't need compression. It needs help.")},
    {STR16("Zumachen ändert den Mix auch nicht."), STR16("Closing the window won't fix the mix.")},
    {STR16("Ach. Du schon wieder."), STR16("Oh. You again.")},
    {STR16("Na endlich. Genug gelogen."), STR16("Finally. Enough pretending.")},
    {STR16("Der Song hat Fragen. Ich leider auch."), STR16("The song has questions. So do I.")},
    {STR16("Interessante Entscheidung. Wirklich sehr interessant."), STR16("Interesting decision. Extremely interesting.")},
    {STR16("Der Groove stolpert, aber wenigstens konsequent."), STR16("The groove is stumbling, but at least consistently.")},
    {STR16("Das klingt teuer. Nicht gut. Nur teuer."), STR16("It sounds expensive. Not good. Just expensive.")},
    {STR16("Vielleicht war Version 17 doch die beste."), STR16("Maybe version 17 really was the best one.")}
}};

constexpr std::size_t kMotivatorCount = kMotivator.size();
constexpr std::size_t kDemotivatorCount = kDemotivator.size();
constexpr std::size_t kPhraseCount = kMotivatorCount + kDemotivatorCount;

} // namespace MotivatoratorPhrases
