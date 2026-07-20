/////////////////////////////////////////////////////////////////////////////
// Name:        transposefunctor.cpp
// Author:      David Bauer
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "transposefunctor.h"

//----------------------------------------------------------------------------

#include <cctype>
#include <charconv>
#include <sstream>

//----------------------------------------------------------------------------

#include "barre.h"
#include "chorddef.h"
#include "chordmember.h"
#include "chordtable.h"
#include "dir.h"
#include "doc.h"
#include "harm.h"
#include "layer.h"
#include "mdiv.h"
#include "measure.h"
#include "pagemilestone.h"
#include "rest.h"
#include "score.h"
#include "staff.h"
#include "text.h"
#include "transposition.h"
#include "vrv.h"

//----------------------------------------------------------------------------

namespace vrv {

namespace {

    bool HasTypeToken(const std::string &type, const std::string &token)
    {
        std::istringstream input(type);
        std::string current;
        while (input >> current) {
            if (current == token) return true;
        }
        return false;
    }

    std::string AddTypeToken(const std::string &type, const std::string &token)
    {
        if (HasTypeToken(type, token)) return type;
        return type.empty() ? token : type + " " + token;
    }

    std::u32string FlattenText(const Object *object)
    {
        std::u32string result;
        for (const Object *descendant : object->FindAllDescendantsByType(TEXT)) {
            if (const Text *text = vrv_cast<const Text *>(descendant)) result += text->GetText();
        }
        return result;
    }

    std::optional<int> ParseCapoText(const std::u32string &text)
    {
        std::string ascii;
        for (char32_t value : text) {
            if (value < 128) ascii.push_back(static_cast<char>(value));
        }
        std::transform(
            ascii.begin(), ascii.end(), ascii.begin(), [](unsigned char value) { return std::tolower(value); });
        const size_t capo = ascii.find("capo");
        if (capo == std::string::npos) return std::nullopt;
        size_t position = capo + 4;
        while (position < ascii.size() && std::isspace(static_cast<unsigned char>(ascii[position]))) ++position;
        if (position < ascii.size() && std::isdigit(static_cast<unsigned char>(ascii[position]))) {
            int value = 0;
            while (position < ascii.size() && std::isdigit(static_cast<unsigned char>(ascii[position])))
                value = value * 10 + ascii[position++] - '0';
            if (value >= 0 && value <= 12) return value;
        }
        int value = 0;
        int previous = 0;
        for (; position < ascii.size(); ++position) {
            int current = ascii[position] == 'i' ? 1 : ascii[position] == 'v' ? 5 : ascii[position] == 'x' ? 10 : 0;
            if (!current) break;
            value += current;
            if (previous < current) value -= 2 * previous;
            previous = current;
        }
        if (value >= 1 && value <= 12) return value;
        return std::nullopt;
    }

    std::string HarmAnchor(const Harm *harm)
    {
        if (harm->HasStartid()) return harm->GetStartid();
        if (!harm->HasTstamp()) return {};
        const Measure *measure = vrv_cast<const Measure *>(harm->GetFirstAncestor(MEASURE));
        if (!measure) return {};
        std::ostringstream key;
        key << measure->GetID() << '@' << harm->GetTstamp();
        return key.str();
    }

    bool ParseCapoToken(const std::string &type, int &capo)
    {
        std::istringstream input(type);
        std::string token;
        while (input >> token) {
            if (!token.starts_with("capo-")) continue;
            const std::string number = token.substr(5);
            const auto result = std::from_chars(number.data(), number.data() + number.size(), capo);
            return (result.ec == std::errc()) && (result.ptr == number.data() + number.size()) && (capo >= 0)
                && (capo <= 12);
        }
        return false;
    }

    std::string SetCapoType(const std::string &type, int capo)
    {
        std::istringstream input(type);
        std::vector<std::string> tokens;
        bool hasRole = false;
        std::string token;
        while (input >> token) {
            if (token.starts_with("capo-")) continue;
            if (token == "guitar-capo") hasRole = true;
            tokens.push_back(token);
        }
        if (!hasRole) tokens.push_back("guitar-capo");
        tokens.push_back("capo-" + std::to_string(capo));

        std::string output;
        for (const std::string &value : tokens) {
            if (!output.empty()) output += ' ';
            output += value;
        }
        return output;
    }

    std::string RomanNumeral(int value)
    {
        static const std::array<std::pair<int, const char *>, 8> numerals{ {
            { 10, "X" },
            { 9, "IX" },
            { 5, "V" },
            { 4, "IV" },
            { 3, "III" },
            { 2, "II" },
            { 1, "I" },
            { 0, "0" },
        } };
        for (const auto &[number, text] : numerals) {
            if (value == number) return text;
        }
        if (value == 11) return "XI";
        if (value == 12) return "XII";
        return std::to_string(value);
    }

    void UpdateCapoText(Dir *directive, int capo)
    {
        Text *text = vrv_cast<Text *>(directive->FindDescendantByType(TEXT));
        if (!text) return;
        std::u32string value = text->GetText();
        size_t end = value.size();
        while (end > 0 && value[end - 1] == U' ') --end;
        while (end > 0 && (value[end - 1] == U':' || value[end - 1] == U'.')) --end;
        size_t begin = end;
        while (begin > 0 && value[begin - 1] != U' ') --begin;
        bool roman = begin < end;
        for (size_t index = begin; index < end; ++index)
            roman &= value[index] == U'I' || value[index] == U'V' || value[index] == U'X';
        const std::string replacement = roman ? RomanNumeral(capo) : std::to_string(capo);
        std::u32string replacement32(replacement.begin(), replacement.end());
        value.replace(begin, end - begin, replacement32);
        text->SetText(value);
    }

    bool ConfigureTransposer(Transposer *transposer, Score *score, const std::string &transposition)
    {
        ScoreDef *scoreDef = score->GetScoreDef();
        if (!scoreDef) return false;
        if (transposer->IsValidIntervalName(transposition)) {
            transposer->SetTransposition(transposition);
            return true;
        }
        if (transposer->IsValidKeyTonic(transposition)) {
            KeySig *keySig = vrv_cast<KeySig *>(scoreDef->FindDescendantByType(KEYSIG));
            TransPitch currentKey(0, 0, 0);
            if (keySig && keySig->HasPname())
                currentKey = TransPitch(keySig->GetPname(), keySig->GetAccid(), ACCIDENTAL_WRITTEN_NONE, 0);
            else if (keySig)
                currentKey = transposer->CircleOfFifthsToMajorTonic(keySig->GetFifthsInt());
            transposer->SetTransposition(currentKey, transposition);
            return true;
        }
        if (transposer->IsValidSemitones(transposition)) {
            KeySig *keySig = vrv_cast<KeySig *>(scoreDef->FindDescendantByType(KEYSIG));
            transposer->SetTransposition(keySig ? keySig->GetFifthsInt() : 0, transposition);
            return true;
        }
        return false;
    }

    std::string FingeringID(const CapoFingering &fingering)
    {
        std::string id = "capo-shape";
        for (int fret : fingering.frets) id += fret < 0 ? "-x" : "-" + std::to_string(fret);
        return id;
    }

    std::optional<std::array<int, 6>> ReadFingering(const ChordDef *definition)
    {
        std::array<int, 6> frets{ -1, -1, -1, -1, -1, -1 };
        std::array<bool, 6> seen{};
        for (const Object *object : definition->FindAllDescendantsByType(CHORDMEMBER)) {
            const ChordMember *member = vrv_cast<const ChordMember *>(object);
            if (!member || !member->HasTabCourse()) return std::nullopt;
            const int string = 6 - member->GetTabCourse();
            if (string < 0 || string >= 6 || seen[string]) return std::nullopt;
            seen[string] = true;
            const std::string fingering = member->HasTabFing() ? member->GetTabFing() : "";
            if (member->HasTabFret()) {
                if (member->GetTabFret() < 0 || fingering == "x" || fingering == "X") return std::nullopt;
                frets[string] = member->GetTabFret();
            }
            else if (fingering == "x" || fingering == "X")
                frets[string] = -1;
            else if (fingering == "o" || fingering == "O")
                frets[string] = 0;
            else
                return std::nullopt;
        }
        for (bool value : seen) {
            if (!value) return std::nullopt;
        }
        return frets;
    }

    ChordDef *FindFingering(ScoreDef *scoreDef, const CapoFingering &fingering)
    {
        for (Object *object : scoreDef->FindAllDescendantsByType(CHORDDEF)) {
            ChordDef *definition = vrv_cast<ChordDef *>(object);
            const auto frets = definition ? ReadFingering(definition) : std::nullopt;
            if (frets && *frets == fingering.frets) return definition;
        }
        return NULL;
    }

    ChordDef *CreateFingering(ScoreDef *scoreDef, const CapoFingering &fingering, const std::string &scope)
    {
        ChordTable *table = vrv_cast<ChordTable *>(scoreDef->FindDescendantByType(CHORDTABLE));
        if (!table) {
            table = new ChordTable();
            scoreDef->AddChild(table);
        }
        ChordDef *definition = new ChordDef();
        definition->SetID("capo-" + scope + "-" + FingeringID(fingering));
        definition->SetType("capo-relative-shape");
        std::array<ChordMember *, 6> members{};
        for (int string = 0; string < 6; ++string) {
            ChordMember *member = new ChordMember();
            member->SetID(definition->GetID() + "-s" + std::to_string(string + 1));
            member->SetTabCourse(6 - string);
            if (fingering.frets[string] < 0)
                member->SetTabFing("x");
            else
                member->SetTabFret(fingering.frets[string]);
            definition->AddChild(member);
            members[string] = member;
        }
        if (fingering.barre) {
            Barre *barre = new Barre();
            barre->SetFret(fingering.barre->fret);
            barre->SetStartid("#" + members[fingering.barre->firstString]->GetID());
            barre->SetEndid("#" + members[fingering.barre->lastString]->GetID());
            definition->AddChild(barre);
        }
        table->AddChild(definition);
        return definition;
    }

    Dir *CreateCapoDirective(Harm *anchor, int capo)
    {
        Measure *measure = vrv_cast<Measure *>(anchor->GetFirstAncestor(MEASURE));
        if (!measure) return NULL;
        Dir *directive = new Dir();
        directive->SetType(SetCapoType("", capo));
        if (anchor->HasStartid()) directive->SetStartid(anchor->GetStartid());
        if (anchor->HasStaff()) directive->SetStaff(anchor->GetStaff());
        Text *text = new Text();
        const std::string label = "Capo " + std::to_string(capo);
        text->SetText(std::u32string(label.begin(), label.end()));
        directive->AddChild(text);
        measure->AddChild(directive);
        return directive;
    }

    struct CollectedCapoMdiv {
        std::string id;
        Score *score = NULL;
        std::vector<Harm *> harmonies;
        std::vector<Dir *> directives;
    };

    class CollectCapoMdivsFunctor : public Functor {
    public:
        bool ImplementsEndInterface() const override { return false; }

        FunctorCode VisitMdiv(Mdiv *mdiv) override
        {
            m_groups.push_back({ mdiv->GetID() });
            m_stack.push_back(m_groups.size() - 1);
            return FUNCTOR_CONTINUE;
        }

        FunctorCode VisitScore(Score *score) override
        {
            if (!m_stack.empty()) m_groups[m_stack.back()].score = score;
            return FUNCTOR_CONTINUE;
        }

        FunctorCode VisitHarm(Harm *harm) override
        {
            if (!m_stack.empty()) m_groups[m_stack.back()].harmonies.push_back(harm);
            return FUNCTOR_SIBLINGS;
        }

        FunctorCode VisitDir(Dir *dir) override
        {
            if (!m_stack.empty()) m_groups[m_stack.back()].directives.push_back(dir);
            return FUNCTOR_SIBLINGS;
        }

        FunctorCode VisitPageMilestoneEnd(PageMilestoneEnd *end) override
        {
            if (end->GetStart() && end->GetStart()->Is(MDIV) && !m_stack.empty()) m_stack.pop_back();
            return FUNCTOR_CONTINUE;
        }

        std::vector<CollectedCapoMdiv> m_groups;

    private:
        std::vector<size_t> m_stack;
    };

} // namespace

//----------------------------------------------------------------------------
// TransposeFunctor
//----------------------------------------------------------------------------

TransposeFunctor::TransposeFunctor(Doc *doc, Transposer *transposer) : DocFunctor(doc)
{
    m_transposer = transposer;
}

void TransposeFunctor::PrepareCapoArrangement()
{
    m_capoDocumentPlans.clear();
    if (m_capoMode == "off") return;
    int configuredFixedCapo = -1;
    const auto configuredResult
        = std::from_chars(m_capoMode.data(), m_capoMode.data() + m_capoMode.size(), configuredFixedCapo);
    const bool hasConfiguredFixedCapo = configuredResult.ec == std::errc()
        && configuredResult.ptr == m_capoMode.data() + m_capoMode.size() && configuredFixedCapo >= 0
        && configuredFixedCapo <= 12;
    if (m_capoMode != "auto" && !hasConfiguredFixedCapo) {
        LogWarning("Invalid transposeCapo value '%s'; expected auto, off, or a fret from 0 to 12", m_capoMode.c_str());
        return;
    }

    CollectCapoMdivsFunctor collector;
    m_doc->Process(collector);
    for (const CollectedCapoMdiv &group : collector.m_groups) {
        if (!m_capoMdivFilter.empty() && group.id != m_capoMdivFilter) continue;
        if (!group.score || !group.score->GetScoreDef()) continue;

        Transposer planningTransposer;
        planningTransposer.SetBase600();
        if (!ConfigureTransposer(&planningTransposer, group.score, m_transposition)) continue;
        TransPitch reference(0, 0, 4);
        planningTransposer.Transpose(reference);
        static constexpr std::array<int, 7> naturalPitchClasses{ 0, 2, 4, 5, 7, 9, 11 };
        const int semitones = (reference.m_oct - 4) * 12 + naturalPitchClasses[reference.m_pname] + reference.m_accid;

        Dir *directive = NULL;
        int sourceCapo = -1;
        for (Dir *candidate : group.directives) {
            int parsed = -1;
            if (candidate->HasType() && HasTypeToken(candidate->GetType(), "guitar-capo")
                && ParseCapoToken(candidate->GetType(), parsed)) {
                directive = candidate;
                sourceCapo = parsed;
                break;
            }
        }
        if (!directive) {
            for (Dir *candidate : group.directives) {
                const std::optional<int> parsed = ParseCapoText(FlattenText(candidate));
                if (!parsed) continue;
                if (directive) {
                    directive = NULL;
                    break;
                }
                directive = candidate;
                sourceCapo = *parsed;
            }
        }
        struct Pair {
            Harm *sounding = NULL;
            Harm *guitar = NULL;
            std::optional<size_t> occurrence;
        };
        std::vector<Pair> pairs;
        std::map<std::string, Harm *> soundingByAnchor;
        bool hasExplicitRoles = false;
        for (Harm *harm : group.harmonies) {
            if (!harm->HasType() || !HasTypeToken(harm->GetType(), "sounding-chord")) continue;
            hasExplicitRoles = true;
            const std::string anchor = HarmAnchor(harm);
            if (!anchor.empty()) soundingByAnchor[anchor] = harm;
        }
        if (hasExplicitRoles) {
            for (Harm *guitar : group.harmonies) {
                if (!guitar->HasType() || !HasTypeToken(guitar->GetType(), "guitar-chord-shape")) continue;
                const auto sounding = soundingByAnchor.find(HarmAnchor(guitar));
                if (sounding != soundingByAnchor.end()) pairs.push_back({ sounding->second, guitar });
            }
            if (sourceCapo < 0) {
                int inferredCapo = -1;
                bool consistent = true;
                for (const Pair &pair : pairs) {
                    const auto guitarChord = ParseCapoChord(pair.guitar->GetTextContent());
                    const auto soundingChord = ParseCapoChord(pair.sounding->GetTextContent());
                    if (!guitarChord || !soundingChord || guitarChord->quality != soundingChord->quality) continue;
                    const int distance = (soundingChord->root - guitarChord->root + 12) % 12;
                    if (inferredCapo < 0)
                        inferredCapo = distance;
                    else
                        consistent &= inferredCapo == distance;
                }
                if (!consistent) continue;
                sourceCapo = inferredCapo < 0 ? 0 : inferredCapo;
            }
        }
        else {
            std::map<std::string, std::vector<Harm *>> byAnchor;
            for (Harm *harm : group.harmonies) {
                const std::string anchor = HarmAnchor(harm);
                if (!anchor.empty()) byAnchor[anchor].push_back(harm);
            }
            int inferredCapo = -1;
            bool ambiguous = false;
            for (const auto &[anchor, harmonies] : byAnchor) {
                const int withReference = std::count_if(
                    harmonies.begin(), harmonies.end(), [](const Harm *harm) { return harm->HasChordref(); });
                if (withReference == 0) continue;
                if (harmonies.size() != 2 || withReference != 1) {
                    ambiguous = true;
                    break;
                }
                Harm *guitar = harmonies[0]->HasChordref() ? harmonies[0] : harmonies[1];
                Harm *sounding = harmonies[0]->HasChordref() ? harmonies[1] : harmonies[0];
                const auto guitarChord = ParseCapoChord(guitar->GetTextContent());
                const auto soundingChord = ParseCapoChord(sounding->GetTextContent());
                if (!guitarChord || !soundingChord || guitarChord->quality != soundingChord->quality) {
                    ambiguous = true;
                    break;
                }
                const int distance = (soundingChord->root - guitarChord->root + 12) % 12;
                if ((guitarChord->bass.has_value() != soundingChord->bass.has_value())
                    || (guitarChord->bass && ((*soundingChord->bass - *guitarChord->bass + 12) % 12 != distance))) {
                    ambiguous = true;
                    break;
                }
                if (inferredCapo < 0)
                    inferredCapo = distance;
                else if (inferredCapo != distance) {
                    ambiguous = true;
                    break;
                }
                pairs.push_back({ sounding, guitar });
            }
            if (ambiguous || pairs.empty() || inferredCapo < 0 || (sourceCapo >= 0 && sourceCapo != inferredCapo))
                continue;
            sourceCapo = inferredCapo;
        }
        if (pairs.empty() || sourceCapo < 0) continue;

        std::map<const Harm *, size_t> documentOrder;
        for (size_t index = 0; index < group.harmonies.size(); ++index) documentOrder[group.harmonies[index]] = index;
        std::stable_sort(pairs.begin(), pairs.end(), [&documentOrder](const Pair &left, const Pair &right) {
            return documentOrder[left.guitar] < documentOrder[right.guitar];
        });

        CapoArrangementRequest request;
        request.transpositionSemitones = semitones;
        request.minimumCapo = std::clamp(m_capoMinimum, 0, 12);
        request.maximumCapo = std::clamp(m_capoMaximum, 0, 12);
        if (hasConfiguredFixedCapo) request.fixedCapo = configuredFixedCapo;
        for (Pair &pair : pairs) {
            const std::optional<CapoChord> chord = ParseCapoChord(pair.sounding->GetTextContent());
            pair.occurrence = request.occurrences.size();
            std::optional<std::array<int, 6>> preferredFrets;
            if (pair.guitar->HasChordDef()) preferredFrets = ReadFingering(pair.guitar->GetChordDef());
            request.occurrences.push_back({ chord, preferredFrets });
        }

        CapoArrangementPlan arrangement = PlanCapoArrangement(request);
        if (request.occurrences.empty())
            arrangement.capo = request.fixedCapo.value_or(std::min(request.minimumCapo, request.maximumCapo));
        CapoDocumentPlan documentPlan;
        documentPlan.score = group.score;
        documentPlan.capo = arrangement.capo;
        documentPlan.directive = directive;
        for (const Pair &pair : pairs) {
            const std::optional<CapoFingering> fingering
                = pair.occurrence ? arrangement.fingerings[*pair.occurrence] : std::nullopt;
            documentPlan.harmonies.push_back({ pair.guitar, semitones + sourceCapo - arrangement.capo, fingering });
        }
        m_capoDocumentPlans.push_back(std::move(documentPlan));
    }
}

void TransposeFunctor::ApplyCapoArrangement()
{
    for (const CapoDocumentPlan &documentPlan : m_capoDocumentPlans) {
        if (!documentPlan.score || !documentPlan.score->GetScoreDef()) continue;
        for (const CapoHarmPlan &plan : documentPlan.harmonies) {
            Transposer guitarTransposer;
            guitarTransposer.SetBase600();
            KeySig *keySig = vrv_cast<KeySig *>(documentPlan.score->GetScoreDef()->FindDescendantByType(KEYSIG));
            guitarTransposer.SetTransposition(keySig ? keySig->GetFifthsInt() : 0, plan.semitones);
            unsigned int position = 0;
            TransPitch pitch;
            if (plan.harm->GetRootPitch(pitch, position)) {
                guitarTransposer.Transpose(pitch);
                plan.harm->SetRootPitch(pitch, position);
            }
            if (plan.harm->GetBassPitch(pitch)) {
                guitarTransposer.Transpose(pitch);
                plan.harm->SetBassPitch(pitch);
            }
            plan.harm->SetType(AddTypeToken(plan.harm->GetType(), "guitar-chord-shape"));
            m_capoTransposedHarmIDs.insert(plan.harm->GetID());
            if (!plan.fingering) {
                plan.harm->ResetHarmLog();
                plan.harm->ResetChordDef();
                plan.harm->SetRendgrid(harmVis_RENDGRID_text);
                continue;
            }
            ChordDef *definition = FindFingering(documentPlan.score->GetScoreDef(), *plan.fingering);
            if (!definition)
                definition
                    = CreateFingering(documentPlan.score->GetScoreDef(), *plan.fingering, documentPlan.score->GetID());
            plan.harm->SetChordref("#" + definition->GetID());
            plan.harm->ResetChordDef();
            plan.harm->SetChordDef(definition);
        }
        Dir *directive = documentPlan.directive;
        if (!directive && !documentPlan.harmonies.empty())
            directive = CreateCapoDirective(documentPlan.harmonies.front().harm, documentPlan.capo);
        if (directive) {
            directive->SetType(SetCapoType(directive->GetType(), documentPlan.capo));
            UpdateCapoText(directive, documentPlan.capo);
        }
        for (const CapoHarmPlan &plan : documentPlan.harmonies) {
            const std::string anchor = HarmAnchor(plan.harm);
            for (Object *object : m_doc->FindAllDescendantsByType(HARM)) {
                Harm *sounding = vrv_cast<Harm *>(object);
                if (sounding != plan.harm && HarmAnchor(sounding) == anchor)
                    sounding->SetType(AddTypeToken(sounding->GetType(), "sounding-chord"));
            }
        }
    }
}

FunctorCode TransposeFunctor::VisitHarm(Harm *harm)
{
    if (m_capoTransposedHarmIDs.contains(harm->GetID())) return FUNCTOR_SIBLINGS;

    unsigned int position = 0;
    TransPitch pitch;
    if (harm->GetRootPitch(pitch, position)) {
        m_transposer->Transpose(pitch);
        harm->SetRootPitch(pitch, position);
    }

    // Transpose bass notes (the "/F#" in "G#m7/F#")
    if (harm->GetBassPitch(pitch)) {
        m_transposer->Transpose(pitch);
        harm->SetBassPitch(pitch);
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode TransposeFunctor::VisitKeySig(KeySig *keySig)
{
    // Store current KeySig
    const int staffN = this->GetStaffNForKeySig(keySig);
    m_keySigForStaffN[staffN] = keySig;

    // Transpose
    const int sig = keySig->GetFifthsInt();

    int intervalClass = m_transposer->CircleOfFifthsToIntervalClass(sig);
    intervalClass = m_transposer->Transpose(intervalClass);
    int fifths = m_transposer->IntervalToCircleOfFifths(intervalClass);

    if (fifths == INVALID_INTERVAL_CLASS) {
        keySig->SetSig({ -1, ACCIDENTAL_WRITTEN_NONE });
    }
    else if (fifths < 0) {
        keySig->SetSig({ -fifths, ACCIDENTAL_WRITTEN_f });
    }
    else if (fifths > 0) {
        keySig->SetSig({ fifths, ACCIDENTAL_WRITTEN_s });
    }
    else {
        keySig->SetSig({ -1, ACCIDENTAL_WRITTEN_NONE });
    }

    // Also convert pname and accid attributes
    if (keySig->HasPname()) {
        TransPitch pitch = TransPitch(keySig->GetPname(), keySig->GetAccid(), ACCIDENTAL_WRITTEN_NONE, 4);
        m_transposer->Transpose(pitch);
        keySig->SetPname(pitch.GetPitchName());
        keySig->SetAccid(pitch.GetAccidGesBasic());
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode TransposeFunctor::VisitMdiv(Mdiv *mdiv)
{
    m_keySigForStaffN.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeFunctor::VisitNote(Note *note)
{
    if (!note->HasPname()) return FUNCTOR_SIBLINGS;

    TransPitch pitch = note->GetTransPitch();
    m_transposer->Transpose(pitch);

    const int staffN = note->GetAncestorStaff(RESOLVE_CROSS_STAFF)->GetN();
    const bool hasKeySig = m_keySigForStaffN.contains(staffN) || m_keySigForStaffN.contains(-1);
    note->UpdateFromTransPitch(pitch, hasKeySig);

    return FUNCTOR_SIBLINGS;
}

FunctorCode TransposeFunctor::VisitRest(Rest *rest)
{
    if ((!rest->HasOloc() || !rest->HasPloc()) && !rest->HasLoc()) return FUNCTOR_SIBLINGS;

    // Find whether current layer is top, middle (either one if multiple) or bottom
    Staff *parentStaff = rest->GetAncestorStaff();
    Layer *parentLayer = vrv_cast<Layer *>(rest->GetFirstAncestor(LAYER));
    assert(parentLayer);

    ListOfObjects objects = parentStaff->FindAllDescendantsByType(LAYER, false);
    const int layerCount = (int)objects.size();

    Layer *firstLayer = vrv_cast<Layer *>(objects.front());
    Layer *lastLayer = vrv_cast<Layer *>(objects.back());

    const bool isTopLayer = (firstLayer->GetN() == parentLayer->GetN());
    const bool isBottomLayer = (lastLayer->GetN() == parentLayer->GetN());

    // transpose based on @oloc and @ploc
    if (rest->HasOloc() && rest->HasPloc()) {
        const TransPitch centralLocation(6, 0, 4); // middle location of the staff
        TransPitch restLoc(rest->GetPloc() - PITCHNAME_c, 0, rest->GetOloc());
        m_transposer->Transpose(restLoc);
        const bool isRestOnSpace = static_cast<bool>((restLoc.m_oct * 7 + restLoc.m_pname) % 2);
        // on outer layers move rest on odd locations one line further
        // in middle layers tolerate even locations to not risk collisions
        if (layerCount > 1) {
            if (isTopLayer && isRestOnSpace) {
                ++restLoc;
            }
            else if (isBottomLayer && isRestOnSpace) {
                --restLoc;
            }
            if ((isTopLayer && (restLoc < centralLocation)) || (isBottomLayer && (restLoc > centralLocation))) {
                restLoc = centralLocation;
            }
        }

        rest->UpdateFromTransLoc(restLoc);
    }
    // transpose based on @loc
    else if (rest->HasLoc()) {
        constexpr int centralLocation = 4;
        int transval = m_transposer->GetTranspositionIntervalClass();
        int diatonic;
        int chromatic;
        m_transposer->IntervalToDiatonicChromatic(diatonic, chromatic, transval);
        int transposedLoc = rest->GetLoc() + diatonic;
        // on outer layers move rest on odd locations one line further
        // in middle layers tolerate even locations to not risk collisions
        if (layerCount > 1) {
            if (isTopLayer)
                transposedLoc += abs(transposedLoc % 2);
            else if (isBottomLayer)
                transposedLoc -= abs(transposedLoc % 2);
            if ((isTopLayer && (transposedLoc < centralLocation))
                || (isBottomLayer && (transposedLoc > centralLocation))) {
                transposedLoc = centralLocation;
            }
        }
        rest->SetLoc(transposedLoc);
    }

    return FUNCTOR_SIBLINGS;
}

FunctorCode TransposeFunctor::VisitScore(Score *score)
{
    ScoreDef *scoreDef = score->GetScoreDef();
    assert(scoreDef);

    if (!ConfigureTransposer(m_transposer, score, m_transposition)) {
        LogWarning("Transposition is invalid: %s", m_transposition.c_str());
        return FUNCTOR_STOP;
    }

    // Evaluate functor on scoreDef
    scoreDef->Process(*this);

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeFunctor::VisitStaffDef(StaffDef *staffDef)
{
    if (!this->GetKeySigForStaffDef(staffDef)) {
        KeySig *keySig = new KeySig();
        staffDef->AddChild(keySig);
        LogWarning("Adding auxiliary KeySig for transposition");
    }

    return FUNCTOR_CONTINUE;
}

const KeySig *TransposeFunctor::GetKeySigForStaffDef(const StaffDef *staffDef) const
{
    const KeySig *keySig = vrv_cast<const KeySig *>(staffDef->FindDescendantByType(KEYSIG));
    if (!keySig) {
        const ScoreDef *scoreDef = vrv_cast<const ScoreDef *>(staffDef->GetFirstAncestor(SCOREDEF));
        keySig = vrv_cast<const KeySig *>(scoreDef->FindDescendantByType(KEYSIG, 1));
    }
    return keySig;
}

int TransposeFunctor::GetStaffNForKeySig(const KeySig *keySig) const
{
    int staffN = -1;
    const StaffDef *staffDef = vrv_cast<const StaffDef *>(keySig->GetFirstAncestor(STAFFDEF));
    if (staffDef) {
        staffN = staffDef->GetN();
    }
    else {
        const Staff *staff = keySig->GetAncestorStaff(ANCESTOR_ONLY, false);
        if (staff) staffN = staff->GetN();
    }
    return staffN;
}

//----------------------------------------------------------------------------
// TransposeSelectedMdivFunctor
//----------------------------------------------------------------------------

TransposeSelectedMdivFunctor::TransposeSelectedMdivFunctor(Doc *doc, Transposer *transposer)
    : TransposeFunctor(doc, transposer)
{
}

FunctorCode TransposeSelectedMdivFunctor::VisitMdiv(Mdiv *mdiv)
{
    TransposeFunctor::VisitMdiv(mdiv);

    m_currentMdivIDs.push_back(mdiv->GetID());

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeSelectedMdivFunctor::VisitPageMilestone(PageMilestoneEnd *pageMilestoneEnd)
{
    if (pageMilestoneEnd->GetStart() && pageMilestoneEnd->GetStart()->Is(MDIV)) {
        m_currentMdivIDs.pop_back();
    }
    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeSelectedMdivFunctor::VisitScore(Score *score)
{
    // Check whether we are in the selected mdiv
    if (!m_selectedMdivID.empty()
        && (std::find(m_currentMdivIDs.begin(), m_currentMdivIDs.end(), m_selectedMdivID) == m_currentMdivIDs.end())) {
        return FUNCTOR_CONTINUE;
    }

    return TransposeFunctor::VisitScore(score);
}

FunctorCode TransposeSelectedMdivFunctor::VisitSystem(System *system)
{
    // Check whether we are in the selected mdiv
    if (!m_selectedMdivID.empty()
        && (std::find(m_currentMdivIDs.begin(), m_currentMdivIDs.end(), m_selectedMdivID) == m_currentMdivIDs.end())) {
        return FUNCTOR_SIBLINGS;
    }

    return FUNCTOR_CONTINUE;
}

//----------------------------------------------------------------------------
// TransposeToSoundingPitchFunctor
//----------------------------------------------------------------------------

TransposeToSoundingPitchFunctor::TransposeToSoundingPitchFunctor(Doc *doc, Transposer *transposer)
    : TransposeFunctor(doc, transposer)
{
}

FunctorCode TransposeToSoundingPitchFunctor::VisitMdiv(Mdiv *mdiv)
{
    TransposeFunctor::VisitMdiv(mdiv);

    m_transposeIntervalForStaffN.clear();

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeToSoundingPitchFunctor::VisitScore(Score *score)
{
    // Evaluate functor on scoreDef
    score->GetScoreDef()->Process(*this);

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeToSoundingPitchFunctor::VisitScoreDef(ScoreDef *scoreDef)
{
    // Set the transposition in order to transpose common key signatures
    // (i.e. encoded as ScoreDef attributes or direct KeySig children)
    const std::vector<int> staffNs = scoreDef->GetStaffNs();
    if (staffNs.empty()) {
        int transposeInterval = 0;
        if (!m_transposeIntervalForStaffN.empty()) {
            transposeInterval = m_transposeIntervalForStaffN.begin()->second;
        }
        m_transposer->SetTransposition(transposeInterval);
    }
    else {
        this->VisitStaffDef(scoreDef->GetStaffDef(staffNs.front()));
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeToSoundingPitchFunctor::VisitScoreDefEnd(ScoreDef *scoreDef)
{
    const bool hasScoreDefKeySig = m_keySigForStaffN.contains(-1);
    if (hasScoreDefKeySig) {
        bool showWarning = false;
        // Check if some staves are untransposed
        const int mapEntryCount = static_cast<int>(m_transposeIntervalForStaffN.size());
        if ((mapEntryCount > 0) && (mapEntryCount < (int)scoreDef->GetStaffNs().size())) {
            showWarning = true;
        }
        // Check if there are different transpositions
        auto iter = std::adjacent_find(m_transposeIntervalForStaffN.begin(), m_transposeIntervalForStaffN.end(),
            [](const auto &mapEntry1, const auto &mapEntry2) { return (mapEntry1.second != mapEntry2.second); });
        if (iter != m_transposeIntervalForStaffN.end()) {
            showWarning = true;
        }
        // Display warning
        if (showWarning) {
            LogWarning("Transpose to sounding pitch cannot handle different transpositions for ScoreDef key "
                       "signatures. Please encode KeySig as StaffDef attribute or child.");
        }
    }

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeToSoundingPitchFunctor::VisitStaff(Staff *staff)
{
    this->UpdateTranspositionFromStaffN(staff);

    return FUNCTOR_CONTINUE;
}

FunctorCode TransposeToSoundingPitchFunctor::VisitStaffDef(StaffDef *staffDef)
{
    // Call base method (creates KeySig if missing)
    TransposeFunctor::VisitStaffDef(staffDef);

    const KeySig *keySig = this->GetKeySigForStaffDef(staffDef);

    // Determine and store the transposition interval (based on keySig)
    if (keySig && staffDef->HasTransSemi() && staffDef->HasN()) {
        const int fifths = keySig->GetFifthsInt();
        int semitones = staffDef->GetTransSemi();
        // Factor out octave transpositions
        const int sign = (semitones >= 0) ? +1 : -1;
        semitones = sign * (std::abs(semitones) % 24);
        m_transposer->SetTransposition(fifths, std::to_string(semitones));
        m_transposeIntervalForStaffN[staffDef->GetN()] = m_transposer->GetTranspositionIntervalClass();
        staffDef->ResetTransposition();
    }
    else {
        this->UpdateTranspositionFromStaffN(staffDef);
    }

    return FUNCTOR_CONTINUE;
}

void TransposeToSoundingPitchFunctor::UpdateTranspositionFromStaffN(const AttNInteger *staffN)
{
    int transposeInterval = 0;
    if (staffN->HasN() && m_transposeIntervalForStaffN.contains(staffN->GetN())) {
        transposeInterval = m_transposeIntervalForStaffN.at(staffN->GetN());
    }
    m_transposer->SetTransposition(transposeInterval);
}

} // namespace vrv
