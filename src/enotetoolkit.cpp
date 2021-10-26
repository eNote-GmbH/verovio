/////////////////////////////////////////////////////////////////////////////
// Name:        enotetoolkit.cpp
// Author:      David Bauer
// Created:     25/10/2021
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "enotetoolkit.h"

//----------------------------------------------------------------------------

#include "comparison.h"
#include "hairpin.h"
#include "measure.h"
#include "tie.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// EnoteToolkit
//----------------------------------------------------------------------------

EnoteToolkit::EnoteToolkit(bool initFont) : Toolkit(initFont) {}

EnoteToolkit::~EnoteToolkit() {}

Measure *EnoteToolkit::FindMeasureByUuid(const std::string &uuid)
{
    Object *measure = m_doc.FindDescendantByUuid(uuid);
    return dynamic_cast<Measure *>(measure);
}

Measure *EnoteToolkit::FindMeasureByN(const std::string &n)
{
    AttNNumberLikeComparison comparison(MEASURE, n);
    return vrv_cast<Measure *>(m_doc.FindDescendantByComparison(&comparison));
}

bool EnoteToolkit::AddHairpin(Measure *measure, const std::string &uuid, int staffN, double startTstamp,
    data_MEASUREBEAT endTstamp, hairpinLog_FORM form)
{
    if (measure) {
        Hairpin *hairpin = new Hairpin();
        hairpin->SetUuid(uuid);
        hairpin->SetStaff({ staffN });
        hairpin->SetTstamp(startTstamp);
        hairpin->SetTstamp2(endTstamp);
        hairpin->SetForm(form);

        measure->AddChild(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::UpdateHairpin(
    const std::string &uuid, double startTstamp, data_MEASUREBEAT endTstamp, hairpinLog_FORM form)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByUuid(uuid));
    if (hairpin) {
        hairpin->SetTstamp(startTstamp);
        hairpin->SetTstamp2(endTstamp);
        hairpin->SetForm(form);
        return true;
    }
    return false;
}

bool EnoteToolkit::UpdateNote(const std::string &uuid, data_PITCHNAME pitch, data_OCTAVE octave)
{
    Note *note = dynamic_cast<Note *>(m_doc.FindDescendantByUuid(uuid));
    if (note) {
        note->SetPname(pitch);
        note->SetOct(octave);
        return true;
    }
    return false;
}

bool EnoteToolkit::RemoveTie(const std::string &uuid)
{
    Tie *tie = dynamic_cast<Tie *>(m_doc.FindDescendantByUuid(uuid));
    if (tie) {
        tie->GetParent()->DeleteChild(tie);
        return true;
    }
    return false;
}

} // namespace vrv
