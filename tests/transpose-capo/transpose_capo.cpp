#include "capoarrangement.h"
#include "toolkit.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {

bool Expect(bool condition, const std::string &message)
{
    if (condition) return true;
    std::cerr << message << '\n';
    return false;
}

int Mod12(int value)
{
    value %= 12;
    return value < 0 ? value + 12 : value;
}

bool TestParser()
{
    bool ok = true;
    const auto flat = vrv::ParseCapoChord(U"B♭m7/F");
    ok &= Expect(flat && flat->root == 10 && flat->quality == "m7" && flat->bass == 5,
        "Unicode accidental or slash bass was parsed incorrectly");
    const auto sharp = vrv::ParseCapoChord(U"F#min");
    ok &= Expect(sharp && sharp->root == 6 && sharp->quality == "m", "ASCII accidental or min alias failed");
    const auto majorSeven = vrv::ParseCapoChord(U"CΔ7");
    ok &= Expect(majorSeven && majorSeven->quality == "maj7", "delta major-seven alias failed");
    ok &= Expect(!vrv::ParseCapoChord(U"Cdim"), "unsupported quality was accepted by the shape generator");
    return ok;
}

bool TestGenerator()
{
    static constexpr std::array<int, 6> tuning{ 40, 45, 50, 55, 59, 64 };
    static constexpr std::array<std::array<int, 5>, 5> fingerSpans{ {
        { 0, 0, 0, 0, 0 },
        { 0, 0, 2, 3, 4 },
        { 0, 2, 0, 1, 3 },
        { 0, 3, 1, 0, 1 },
        { 0, 4, 3, 1, 0 },
    } };
    const std::vector<std::string> qualities{ "", "m", "7", "maj7", "m7", "sus2", "sus4", "add9", "6", "6add9" };
    const std::map<std::string, std::pair<std::set<int>, std::set<int>>> formulas{
        { "", { { 0, 4, 7 }, { 0, 4 } } },
        { "m", { { 0, 3, 7 }, { 0, 3 } } },
        { "7", { { 0, 4, 7, 10 }, { 0, 4, 10 } } },
        { "maj7", { { 0, 4, 7, 11 }, { 0, 4, 11 } } },
        { "m7", { { 0, 3, 7, 10 }, { 0, 3, 10 } } },
        { "sus2", { { 0, 2, 7 }, { 0, 2 } } },
        { "sus4", { { 0, 5, 7 }, { 0, 5 } } },
        { "add9", { { 0, 2, 4, 7 }, { 0, 2, 4 } } },
        { "6", { { 0, 4, 7, 9 }, { 0, 4, 9 } } },
        { "6add9", { { 0, 2, 4, 7, 9 }, { 0, 2, 4, 9 } } },
    };
    bool ok = true;
    for (int root = 0; root < 12; ++root) {
        for (const std::string &quality : qualities) {
            const vrv::CapoChord chord{ root, quality, std::nullopt };
            const std::vector<vrv::CapoFingering> candidates = vrv::GenerateCapoFingerings(chord);
            ok &= Expect(!candidates.empty(), "supported chord produced no fingering");
            if (candidates.empty()) continue;
            const vrv::CapoFingering &selected = candidates.front();
            std::set<int> present;
            int sounding = 0;
            int minimum = 8;
            int maximum = 0;
            for (int string = 0; string < 6; ++string) {
                const int fret = selected.frets[string];
                ok &= Expect(fret >= -1 && fret <= 7, "generated fret is outside the search range");
                if (fret < 0) continue;
                ++sounding;
                if (fret > 0) {
                    ok &= Expect(selected.fingers[string] >= 1 && selected.fingers[string] <= 4,
                        "a stopped string has no concrete finger assignment");
                    minimum = std::min(minimum, fret);
                    maximum = std::max(maximum, fret);
                }
                else {
                    ok &= Expect(selected.fingers[string] == 0, "an open string was assigned a fretting finger");
                }
                present.insert(Mod12(tuning[string] + fret - root));
            }
            ok &= Expect(sounding >= 4, "generated accompaniment fingering has fewer than four sounding strings");
            int firstSounding = 0;
            while (firstSounding < 6 && selected.frets[firstSounding] < 0) ++firstSounding;
            int lastSounding = 5;
            while (lastSounding >= 0 && selected.frets[lastSounding] < 0) --lastSounding;
            for (int string = firstSounding; string <= lastSounding; ++string) {
                ok &= Expect(selected.frets[string] >= 0, "generated accompaniment fingering has an inner mute");
            }
            ok &= Expect(minimum == 8 || maximum - minimum <= 4, "generated fingering exceeds the fret span");
            std::set<int> assignedFingers;
            for (int finger : selected.fingers) {
                if (finger > 0) assignedFingers.insert(finger);
            }
            if (selected.barre) {
                const vrv::CapoBarre &barre = *selected.barre;
                ok &= Expect(barre.firstString >= 0 && barre.firstString < barre.lastString && barre.lastString < 6,
                    "generated barre does not span contiguous strings");
                int pressed = 0;
                for (int string = barre.firstString; string <= barre.lastString; ++string) {
                    ok &= Expect(selected.frets[string] != 0, "generated barre crosses an open sounding string");
                    pressed += selected.frets[string] == barre.fret;
                }
                ok &= Expect(pressed >= 2, "generated barre stops fewer than two strings at its fret");
            }
            ok &= Expect(assignedFingers.size() <= 4, "generated fingering uses more than four fingers");
            for (int left = 0; left < 6; ++left) {
                if (selected.fingers[left] <= 0) continue;
                for (int right = left + 1; right < 6; ++right) {
                    if (selected.fingers[right] <= 0 || selected.fingers[left] == selected.fingers[right]) continue;
                    ok &= Expect(std::abs(selected.frets[left] - selected.frets[right])
                            <= fingerSpans[selected.fingers[left]][selected.fingers[right]],
                        "generated fingering exceeds a pairwise finger span");
                }
            }
            for (int pitch : present)
                ok &= Expect(formulas.at(quality).first.contains(pitch), "generated fingering contains a foreign tone");
            for (int pitch : formulas.at(quality).second)
                ok &= Expect(present.contains(pitch), "generated fingering omits a required tone");
            const auto again = vrv::GenerateCapoFingerings(chord);
            ok &= Expect(
                !again.empty() && again.front().frets == selected.frets, "candidate ordering is not deterministic");
        }
    }
    const auto expectShape
        = [&ok](const vrv::CapoChord &chord, const std::array<int, 6> &frets, const std::string &name) {
              const auto candidates = vrv::GenerateCapoFingerings(chord);
              std::string actual;
              if (!candidates.empty()) {
                  for (int fret : candidates.front().frets)
                      actual += actual.empty() ? std::to_string(fret) : "," + std::to_string(fret);
              }
              ok &= Expect(!candidates.empty() && candidates.front().frets == frets,
                  name + " did not select the expected accompaniment shape (selected " + actual + ")");
          };
    expectShape({ 0, "", std::nullopt }, { -1, 3, 2, 0, 1, 0 }, "C major");
    expectShape({ 2, "", std::nullopt }, { -1, -1, 0, 2, 3, 2 }, "D major");
    expectShape({ 4, "", std::nullopt }, { 0, 2, 2, 1, 0, 0 }, "E major");
    expectShape({ 7, "", std::nullopt }, { 3, 2, 0, 0, 0, 3 }, "G major");
    expectShape({ 9, "", std::nullopt }, { -1, 0, 2, 2, 2, 0 }, "A major");
    expectShape({ 9, "m", std::nullopt }, { -1, 0, 2, 2, 1, 0 }, "A minor");
    expectShape({ 2, "m", std::nullopt }, { -1, -1, 0, 2, 3, 1 }, "D minor");
    expectShape({ 4, "m", std::nullopt }, { 0, 2, 2, 0, 0, 0 }, "E minor");
    expectShape({ 9, "7", std::nullopt }, { -1, 0, 2, 0, 2, 0 }, "A dominant seventh");
    expectShape({ 0, "maj7", std::nullopt }, { -1, 3, 2, 0, 0, 0 }, "C major seventh");
    expectShape({ 4, "m7", std::nullopt }, { 0, 2, 0, 0, 0, 0 }, "E minor seventh");
    expectShape({ 2, "sus2", std::nullopt }, { -1, -1, 0, 2, 3, 0 }, "D suspended second");
    expectShape({ 2, "sus4", std::nullopt }, { -1, -1, 0, 2, 3, 3 }, "D suspended fourth");
    expectShape({ 0, "add9", std::nullopt }, { -1, 3, 0, 0, 1, 0 }, "C add ninth");
    expectShape({ 0, "6", std::nullopt }, { -1, 3, 2, 2, 1, 0 }, "C sixth");
    expectShape({ 0, "6add9", std::nullopt }, { -1, 3, 0, 2, 1, 0 }, "C sixth add ninth");
    expectShape({ 2, "", 6 }, { 2, 0, 0, 2, 3, 2 }, "D over F sharp");

    ok &= Expect(vrv::SolveCapoFingeringPatternForTesting({ -1, 3, -1, 0, 1, 0 }).empty(),
        "the physical solver accepted an inner muted string");
    ok &= Expect(vrv::SolveCapoFingeringPatternForTesting({ 1, 3, 3, 0, 1, 1 }).empty(),
        "the physical solver accepted an open string inside the only possible barre");
    ok &= Expect(vrv::SolveCapoFingeringPatternForTesting({ 1, 5, 5, 5, -1, -1 }).empty(),
        "the physical solver accepted an unreachable four-fret finger spread");
    ok &= Expect(!vrv::ValidateCapoFingeringForTesting({ -1, -1, 0, 2, 3, 2 }, { 0, 0, 0, 3, 1, 2 }),
        "the fingering validator accepted fingers in the wrong fret order");
    for (int firstFinger = 1; firstFinger <= 4; ++firstFinger) {
        for (int secondFinger = firstFinger + 1; secondFinger <= 4; ++secondFinger) {
            const int limit = fingerSpans[firstFinger][secondFinger];
            const std::array<int, 6> validFrets{ 1, 1 + limit, 0, 0, 0, 0 };
            const std::array<int, 6> invalidFrets{ 1, 2 + limit, 0, 0, 0, 0 };
            const std::array<int, 6> fingers{ firstFinger, secondFinger, 0, 0, 0, 0 };
            ok &= Expect(vrv::ValidateCapoFingeringForTesting(validFrets, fingers),
                "the fingering validator rejected a pair at its exact span limit");
            ok &= Expect(!vrv::ValidateCapoFingeringForTesting(invalidFrets, fingers),
                "the fingering validator accepted a pair beyond its span limit");
        }
    }
    ok &= Expect(
        !vrv::ValidateCapoFingeringForTesting({ 1, 3, 3, 0, 1, 1 }, { 1, 3, 4, 0, 1, 1 }, vrv::CapoBarre{ 1, 0, 5 }),
        "the fingering validator accepted an open string inside a barre");
    ok &= Expect(
        !vrv::ValidateCapoFingeringForTesting({ 1, 3, 3, 2, 1, 1 }, { 2, 3, 4, 2, 2, 2 }, vrv::CapoBarre{ 1, 0, 5 }),
        "the fingering validator accepted a barre made with a non-index finger");
    const auto partialBarre = vrv::SolveCapoFingeringPatternForTesting({ -1, -1, 3, 2, 1, 1 });
    ok &= Expect(!partialBarre.empty()
            && std::any_of(partialBarre.begin(), partialBarre.end(),
                [](const vrv::CapoFingering &fingering) {
                    return fingering.barre && fingering.barre->fret == 1 && fingering.barre->firstString == 4
                        && fingering.barre->lastString == 5;
                }),
        "the physical solver rejected a valid short F-major barre");
    const auto fullBarre = vrv::SolveCapoFingeringPatternForTesting({ 1, 3, 3, 2, 1, 1 });
    ok &= Expect(!fullBarre.empty()
            && std::any_of(fullBarre.begin(), fullBarre.end(),
                [](const vrv::CapoFingering &fingering) {
                    return fingering.barre && fingering.barre->firstString == 0 && fingering.barre->lastString == 5;
                }),
        "the physical solver rejected a valid full F-major barre");
    const vrv::CapoChord slash{ 2, "", 6 };
    const auto slashCandidates = vrv::GenerateCapoFingerings(slash);
    ok &= Expect(!slashCandidates.empty(), "slash chord produced no fingering");
    if (!slashCandidates.empty()) {
        int lowest = 1000;
        for (int string = 0; string < 6; ++string) {
            if (slashCandidates.front().frets[string] >= 0)
                lowest = std::min(lowest, tuning[string] + slashCandidates.front().frets[string]);
        }
        ok &= Expect(Mod12(lowest) == 6, "slash bass is not the lowest sounding tone");
    }
    const std::vector<std::u32string> songbookChords{ U"D", U"Em7", U"Bm7", U"A", U"Gadd9", U"Asus4", U"A/D", U"G/D",
        U"D/F♯", U"G", U"A7", U"F♯7", U"D/A", U"G/A", U"G6/A" };
    for (const std::u32string &label : songbookChords) {
        const std::optional<vrv::CapoChord> chord = vrv::ParseCapoChord(label);
        ok &= Expect(chord.has_value(), "a chord type from the songbook fixture was not parsed");
        if (!chord) continue;
        const std::vector<vrv::CapoFingering> candidates = vrv::GenerateCapoFingerings(*chord);
        ok &= Expect(!candidates.empty(), "a chord type from the songbook fixture has no strict accompaniment shape");
    }
    return ok;
}

bool TestPlanning()
{
    bool ok = true;
    vrv::CapoArrangementRequest fixed;
    fixed.occurrences = { { vrv::CapoChord{ 5, "", std::nullopt }, std::nullopt } };
    fixed.transpositionSemitones = 2;
    fixed.fixedCapo = 5;
    const vrv::CapoArrangementPlan fixedPlan = vrv::PlanCapoArrangement(fixed);
    ok &= Expect(fixedPlan.capo == 5 && fixedPlan.shapeChords.size() == 1 && fixedPlan.shapeChords[0]
            && fixedPlan.shapeChords[0]->root == 2,
        "F/D, capo III, M2 did not produce G/D at target capo V");
    ok &= Expect(fixedPlan.fingerings.size() == 1 && fixedPlan.fingerings[0]
            && fixedPlan.fingerings[0]->frets == std::array<int, 6>{ -1, -1, 0, 2, 3, 2 },
        "fixed-capo planning did not retain the root-position D accompaniment shape");

    vrv::CapoArrangementRequest automatic;
    automatic.occurrences = { { vrv::CapoChord{ 0, "", std::nullopt }, std::nullopt },
        { vrv::CapoChord{ 5, "maj7", std::nullopt }, std::nullopt },
        { vrv::CapoChord{ 10, "add9", std::nullopt }, std::nullopt } };
    automatic.transpositionSemitones = -2;
    automatic.minimumCapo = 0;
    automatic.maximumCapo = 7;
    const vrv::CapoArrangementPlan first = vrv::PlanCapoArrangement(automatic);
    const vrv::CapoArrangementPlan second = vrv::PlanCapoArrangement(automatic);
    ok &= Expect(first.capo >= 0 && first.capo <= 7, "automatic capo is outside the configured range");
    ok &= Expect(first.capo == second.capo && first.fingerings.size() == second.fingerings.size(),
        "automatic capo planning is not deterministic");

    vrv::CapoArrangementRequest progression;
    progression.fixedCapo = 0;
    progression.occurrences = { { vrv::CapoChord{ 0, "", std::nullopt }, std::nullopt },
        { vrv::CapoChord{ 9, "m", std::nullopt }, std::nullopt },
        { vrv::CapoChord{ 0, "", std::nullopt }, std::nullopt } };
    const vrv::CapoArrangementPlan progressionPlan = vrv::PlanCapoArrangement(progression);
    ok &= Expect(progressionPlan.fingerings.size() == 3 && progressionPlan.fingerings[0]
            && progressionPlan.fingerings[1] && progressionPlan.fingerings[2],
        "ordered progression planning dropped a supported occurrence");
    if (progressionPlan.fingerings.size() == 3 && progressionPlan.fingerings[0] && progressionPlan.fingerings[1]
        && progressionPlan.fingerings[2]) {
        ok &= Expect(progressionPlan.fingerings[0]->frets == std::array<int, 6>{ -1, 3, 2, 0, 1, 0 }
                && progressionPlan.fingerings[1]->frets == std::array<int, 6>{ -1, 0, 2, 2, 1, 0 }
                && progressionPlan.fingerings[2]->frets == progressionPlan.fingerings[0]->frets,
            "progression planning did not retain the familiar anchored C-Am-C shapes");
    }

    vrv::CapoArrangementRequest segmented = progression;
    segmented.occurrences.insert(segmented.occurrences.begin() + 1, { std::nullopt, std::nullopt });
    const vrv::CapoArrangementPlan segmentedPlan = vrv::PlanCapoArrangement(segmented);
    ok &= Expect(segmentedPlan.fingerings.size() == 4 && !segmentedPlan.fingerings[1],
        "an unsupported chord did not split the fingering path");

    vrv::CapoArrangementRequest invalidPreference;
    invalidPreference.fixedCapo = 0;
    invalidPreference.occurrences
        = { { vrv::CapoChord{ 0, "", std::nullopt }, std::array<int, 6>{ 0, 0, -1, 0, 1, 0 } } };
    const vrv::CapoArrangementPlan preferencePlan = vrv::PlanCapoArrangement(invalidPreference);
    ok &= Expect(preferencePlan.fingerings.size() == 1 && preferencePlan.fingerings[0]
            && preferencePlan.fingerings[0]->frets != *invalidPreference.occurrences[0].preferredFrets,
        "an invalid existing chordDef preference bypassed the physical solver");
    return ok;
}

std::string MinimalMEI(const std::string &content)
{
    return "<mei xmlns=\"http://www.music-encoding.org/ns/mei\" meiversion=\"6.0-dev\"><meiHead><fileDesc>"
           "<titleStmt><title>test</title></titleStmt><pubStmt/></fileDesc></meiHead><music><body>"
        + content + "</body></music></mei>";
}

size_t Count(const std::string &text, const std::string &needle)
{
    size_t count = 0;
    size_t position = 0;
    while ((position = text.find(needle, position)) != std::string::npos) {
        ++count;
        position += needle.size();
    }
    return count;
}

std::string Mdiv(
    const std::string &id, const std::string &noteID, const std::string &sounding, const std::string &guitar)
{
    return "<mdiv xml:id=\"" + id
        + "\"><score><scoreDef keysig=\"0\"><staffGrp><staffDef n=\"1\" lines=\"5\"/></staffGrp></scoreDef>"
          "<section><measure n=\"1\"><staff n=\"1\"><layer n=\"1\"><note xml:id=\""
        + noteID + "\" pname=\"c\" oct=\"4\" dur=\"1\"/></layer></staff><harm type=\"sounding-chord\" startid=\"#"
        + noteID + "\">" + sounding + "</harm><harm type=\"guitar-chord-shape\" startid=\"#" + noteID + "\">" + guitar
        + "</harm><dir type=\"guitar-capo capo-3\" tstamp=\"1\">Capo III</dir></measure></section></score></mdiv>";
}

bool TestMultipleMdivs(const char *resourcePath)
{
    const std::string data = MinimalMEI(Mdiv("one", "one-note", "F", "D") + Mdiv("two", "two-note", "C", "A"));
    bool ok = true;
    vrv::Toolkit all(false);
    all.SetResourcePath(resourcePath);
    all.SetOptions(R"({"transpose":"M2","transposeCapo":"5","mdivAll":true})");
    ok &= Expect(all.LoadData(data), "multiple-mdiv fixture did not load");
    const std::string allMEI = all.GetMEI();
    ok &= Expect(
        Count(allMEI, "guitar-capo capo-5") == 2, "one target capo was not planned independently for each mdiv");

    vrv::Toolkit selected(false);
    selected.SetResourcePath(resourcePath);
    selected.SetOptions(R"({"transposeMdiv":{"one":"M2"},"transposeCapo":"5","mdivAll":true})");
    ok &= Expect(selected.LoadData(data), "transposeMdiv fixture did not load");
    const std::string selectedMEI = selected.GetMEI();
    ok &= Expect(Count(selectedMEI, "guitar-capo capo-5") == 1 && Count(selectedMEI, "guitar-capo capo-3") == 1,
        "transposeMdiv capo planning changed an unselected mdiv");
    return ok;
}

bool TestLegacyDetection(const char *resourcePath)
{
    const std::string pairs
        = R"(<mdiv xml:id="legacy"><score><scoreDef keysig="1f"><staffGrp><staffDef n="1" lines="5"/></staffGrp></scoreDef><section><measure n="1"><staff n="1"><layer n="1"><note xml:id="l1" pname="c" oct="4" dur="2"/><note xml:id="l2" pname="d" oct="4" dur="2"/></layer></staff><harm startid="#l1" chordref="#d">D</harm><harm startid="#l1">F</harm><harm startid="#l2" chordref="#a">A</harm><harm startid="#l2">C</harm><dir tstamp="1"><rend>Capo III:</rend></dir></measure></section></score></mdiv>)";
    bool ok = true;
    vrv::Toolkit legacy(false);
    legacy.SetResourcePath(resourcePath);
    legacy.SetOptions(R"({"transpose":"M2","transposeCapo":"5"})");
    ok &= Expect(legacy.LoadData(MinimalMEI(pairs)), "legacy-pair fixture did not load");
    const std::string legacyMEI = legacy.GetMEI();
    ok &= Expect(
        Count(legacyMEI, "type=\"guitar-chord-shape\"") == 2 && Count(legacyMEI, "type=\"sounding-chord\"") == 2,
        "unambiguous legacy pairs were not assigned controlled roles");
    ok &= Expect(
        legacyMEI.find("guitar-capo capo-5") != std::string::npos && legacyMEI.find("Capo V:") != std::string::npos,
        "legacy capo text or token was not updated without losing punctuation");

    const std::string ambiguous
        = R"(<mdiv xml:id="ambiguous"><score><scoreDef keysig="1f"><staffGrp><staffDef n="1" lines="5"/></staffGrp></scoreDef><section><measure n="1"><staff n="1"><layer n="1"><note xml:id="a1" pname="c" oct="4" dur="1"/></layer></staff><harm startid="#a1" chordref="#d">D</harm><harm startid="#a1">F</harm><harm startid="#a1">A</harm><dir tstamp="1">Capo III</dir></measure></section></score></mdiv>)";
    vrv::Toolkit rejected(false);
    rejected.SetResourcePath(resourcePath);
    rejected.SetOptions(R"({"transpose":"M2","transposeCapo":"5"})");
    ok &= Expect(rejected.LoadData(MinimalMEI(ambiguous)), "ambiguous legacy fixture did not load");
    const std::string rejectedMEI = rejected.GetMEI();
    ok &= Expect(rejectedMEI.find("guitar-chord-shape") == std::string::npos
            && rejectedMEI.find("guitar-capo") == std::string::npos,
        "ambiguous legacy harmonies were incorrectly classified");
    return ok;
}

bool TestFallbackAndOff(const char *resourcePath)
{
    const std::string score
        = R"(<mdiv xml:id="unknown"><score><scoreDef keysig="1f"><chordTable><chordDef xml:id="old"><chordMember tab.course="6" tab.fing="x"/></chordDef></chordTable><staffGrp><staffDef n="1" lines="5"/></staffGrp></scoreDef><section><measure n="1"><staff n="1"><layer n="1"><note xml:id="u1" pname="c" oct="4" dur="1"/></layer></staff><harm type="sounding-chord" startid="#u1">Fdim</harm><harm type="guitar-chord-shape" startid="#u1" chordref="#old" rendgrid="gridtext">Ddim</harm><dir type="guitar-capo capo-3" tstamp="1">Capo 3</dir></measure></section></score></mdiv>)";
    bool ok = true;
    vrv::Toolkit fallback(false);
    fallback.SetResourcePath(resourcePath);
    fallback.SetOptions(R"({"transpose":"M2","transposeCapo":"5"})");
    ok &= Expect(fallback.LoadData(MinimalMEI(score)), "unknown-quality fixture did not load");
    const std::string fallbackMEI = fallback.GetMEI();
    ok &= Expect(fallbackMEI.find(">Ddim</harm>") != std::string::npos,
        "unknown guitar quality did not receive capo-relative text transposition");
    ok &= Expect(fallbackMEI.find("rendgrid=\"text\"") != std::string::npos
            && fallbackMEI.find("chordref=\"#old\"") == std::string::npos,
        "unknown quality retained an invalid chord diagram");

    vrv::Toolkit off(false);
    off.SetResourcePath(resourcePath);
    off.SetOptions(R"({"transpose":"M2","transposeCapo":"off"})");
    ok &= Expect(off.LoadData(MinimalMEI(score)), "off fixture did not load");
    const std::string offMEI = off.GetMEI();
    ok &= Expect(
        offMEI.find(">Edim</harm>") != std::string::npos && offMEI.find("guitar-capo capo-3") != std::string::npos,
        "transposeCapo=off did not preserve the previous harmony behavior");

    const std::string withoutDirective
        = R"(<mdiv xml:id="new-capo"><score><scoreDef keysig="0"><staffGrp><staffDef n="1" lines="5"/></staffGrp></scoreDef><section><measure n="1"><staff n="1"><layer n="1"><note xml:id="c1" pname="c" oct="4" dur="1"/></layer></staff><harm type="sounding-chord" startid="#c1">C</harm><harm type="guitar-chord-shape" startid="#c1">C</harm></measure></section></score></mdiv>)";
    vrv::Toolkit created(false);
    created.SetResourcePath(resourcePath);
    created.SetOptions(R"({"transpose":"M2","transposeCapo":"2"})");
    ok &= Expect(created.LoadData(MinimalMEI(withoutDirective)), "missing-capo fixture did not load");
    ok &= Expect(created.GetMEI().find("guitar-capo capo-2") != std::string::npos,
        "a missing controlled capo directive was not added");
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 3) return 2;

    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(argv[2]);

    bool ok = true;
    ok &= TestParser();
    ok &= TestGenerator();
    ok &= TestPlanning();
    ok &= TestFallbackAndOff(argv[2]);
    ok &= TestMultipleMdivs(argv[2]);
    ok &= TestLegacyDetection(argv[2]);
    ok &= Expect(
        toolkit.SetOptions(R"({"transpose":"M2","transposeCapo":"5"})"), "fixed target capo options were rejected");
    ok &= Expect(toolkit.LoadFile(argv[1]), "fixed target capo fixture did not load");

    const std::string mei = toolkit.GetMEI();
    ok &= Expect(mei.find("type=\"sounding-chord\"") != std::string::npos, "sounding harmony role disappeared");
    ok &= Expect(mei.find(">G</harm>") != std::string::npos, "sounding F chord was not transposed to G");
    ok &= Expect(mei.find("type=\"guitar-chord-shape\"") != std::string::npos, "guitar harmony role disappeared");
    ok &= Expect(mei.find(">D</harm>") != std::string::npos, "D guitar shape did not remain unchanged at capo V");
    ok &= Expect(mei.find("type=\"guitar-capo capo-5\"") != std::string::npos,
        "target capo was not written as machine-readable MEI");
    ok &= Expect(
        mei.find("chordref=\"#shape-d\"") != std::string::npos, "matching existing D chord diagram was not reused");

    if (!ok) std::cerr << mei << '\n';
    return ok ? 0 : 1;
}
