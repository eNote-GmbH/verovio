/////////////////////////////////////////////////////////////////////////////
// Name:        harm.cpp
// Author:      Laurent Pugin
// Created:     2016
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "harm.h"

//----------------------------------------------------------------------------

#include <cassert>

//----------------------------------------------------------------------------

#include "doc.h"
#include "editorial.h"
#include "f.h"
#include "fb.h"
#include "functor.h"
#include "measure.h"
#include "system.h"
#include "text.h"
#include "verticalaligner.h"
#include "vrv.h"

namespace vrv {

namespace {

    std::vector<Text *> GetHarmonyTextNodes(Harm *harm)
    {
        std::vector<Text *> result;
        for (Object *object : harm->FindAllDescendantsByType(TEXT)) {
            if (Text *text = vrv_cast<Text *>(object)) result.push_back(text);
        }
        return result;
    }

    std::vector<const Text *> GetHarmonyTextNodes(const Harm *harm)
    {
        std::vector<const Text *> result;
        for (const Object *object : harm->FindAllDescendantsByType(TEXT)) {
            if (const Text *text = vrv_cast<const Text *>(object)) result.push_back(text);
        }
        return result;
    }

    void ReplaceHarmonyText(Harm *harm, size_t begin, size_t end, const std::u32string &replacement)
    {
        std::vector<Text *> nodes = GetHarmonyTextNodes(harm);
        size_t offset = 0;
        bool inserted = false;
        for (Text *node : nodes) {
            std::u32string text = node->GetText();
            const size_t nodeBegin = offset;
            const size_t nodeEnd = offset + text.size();
            if (end <= nodeBegin || begin >= nodeEnd) {
                offset = nodeEnd;
                continue;
            }
            const size_t localBegin = begin > nodeBegin ? begin - nodeBegin : 0;
            const size_t localEnd = std::min(end, nodeEnd) - nodeBegin;
            const std::u32string value = inserted ? std::u32string() : replacement;
            text.replace(localBegin, localEnd - localBegin, value);
            node->SetText(text);
            inserted = true;
            offset = nodeEnd;
        }
    }

    bool ParseHarmonyPitch(const std::u32string &text, TransPitch &pitch, unsigned int &pos)
    {
        if (text.length() <= pos || text.at(pos) < 'A' || text.at(pos) > 'G') return false;
        int pname = (text.at(pos) - 'C' + 7) % 7;
        int accid = 0;
        for (pos++; pos < text.length(); pos++) {
            if (text.at(pos) == UNICODE_DOUBLE_FLAT)
                accid -= 2;
            else if (text.at(pos) == 'b' || text.at(pos) == UNICODE_FLAT)
                --accid;
            else if (text.at(pos) == '#' || text.at(pos) == UNICODE_SHARP)
                ++accid;
            else if (text.at(pos) == UNICODE_DOUBLE_SHARP)
                accid += 2;
            else
                break;
        }
        pitch = TransPitch(pname, accid, 4);
        return true;
    }

} // namespace

//----------------------------------------------------------------------------
// Harm
//----------------------------------------------------------------------------

static const ClassRegistrar<Harm> s_factory("harm", HARM);

Harm::Harm()
    : ControlElement(HARM)
    , TextListInterface()
    , TextDirInterface()
    , TimeSpanningInterface()
    , AttHarmLog()
    , AttHarmVis()
    , AttLang()
    , AttNNumberLike()
    , AttVerticalGroup()
{
    this->RegisterInterface(TextDirInterface::GetAttClasses(), TextDirInterface::IsInterface());
    this->RegisterInterface(TimeSpanningInterface::GetAttClasses(), TimeSpanningInterface::IsInterface());
    this->RegisterAttClass(ATT_HARMLOG);
    this->RegisterAttClass(ATT_HARMVIS);
    this->RegisterAttClass(ATT_LANG);
    this->RegisterAttClass(ATT_NNUMBERLIKE);
    this->RegisterAttClass(ATT_VERTICALGROUP);

    this->Reset();
}

Harm::~Harm() {}

void Harm::Reset()
{
    ControlElement::Reset();
    TextDirInterface::Reset();
    TimeSpanningInterface::Reset();
    this->ResetHarmLog();
    this->ResetHarmVis();
    this->ResetLang();
    this->ResetNNumberLike();
    this->ResetChordDef();
    this->ResetVerticalGroup();
}

bool Harm::IsSupportedChild(ClassId classId)
{
    static const std::vector<ClassId> supported{ FB, LB, REND, TEXT };

    if (std::find(supported.begin(), supported.end(), classId) != supported.end()) {
        return true;
    }
    else if (Object::IsEditorialElement(classId)) {
        return true;
    }
    else {
        return false;
    }
}

bool Harm::IsCloserToStaffThan(const FloatingObject *other, data_STAFFREL drawingPlace) const
{
    if (!other->Is(HARM)) return false;
    if ((drawingPlace != STAFFREL_above) && (drawingPlace != STAFFREL_below)) return false;

    return (this->GetDrawingGrpId() < other->GetDrawingGrpId());
}

void Harm::SetChordDef(ChordDef *chordDef)
{
    assert(!m_chordDef);
    m_chordDef = chordDef;
}

bool Harm::GetRootPitch(TransPitch &pitch, unsigned int &pos) const
{
    const std::u32string text = this->GetTextContent();
    if (ParseHarmonyPitch(text, pitch, pos)) return true;
    LogWarning("Failed to extract a pitch.");
    return false;
}

void Harm::SetRootPitch(const TransPitch &pitch, unsigned int endPos)
{
    ReplaceHarmonyText(this, 0, endPos, pitch.GetPitchString());
}

bool Harm::GetBassPitch(TransPitch &pitch) const
{
    const std::u32string text = this->GetTextContent();
    if (!text.length()) return false;

    for (unsigned int pos = 0; pos < text.length(); pos++) {
        if (text.at(pos) == U'/') {
            pos++;
            return ParseHarmonyPitch(text, pitch, pos);
        }
    }
    return false;
}

void Harm::SetBassPitch(const TransPitch &pitch)
{
    const std::u32string text = this->GetTextContent();
    const size_t slash = text.find(U'/');
    if (slash == std::u32string::npos) return;
    unsigned int end = static_cast<unsigned int>(slash + 1);
    TransPitch ignored;
    if (!ParseHarmonyPitch(text, ignored, end)) return;
    ReplaceHarmonyText(this, slash + 1, end, pitch.GetPitchString());
}

std::u32string Harm::GetTextContent() const
{
    std::u32string result;
    for (const Text *text : GetHarmonyTextNodes(this)) result += text->GetText();
    return result;
}

//----------------------------------------------------------------------------
// Harm functor methods
//----------------------------------------------------------------------------

FunctorCode Harm::Accept(Functor &functor)
{
    return functor.VisitHarm(this);
}

FunctorCode Harm::Accept(ConstFunctor &functor) const
{
    return functor.VisitHarm(this);
}

FunctorCode Harm::AcceptEnd(Functor &functor)
{
    return functor.VisitHarmEnd(this);
}

FunctorCode Harm::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitHarmEnd(this);
}

} // namespace vrv
