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
#include "timestamp.h"

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

std::vector<Measure *> EnoteToolkit::FindAllMeasures()
{
    ListOfObjects objects;
    ClassIdComparison comparison(MEASURE);
    m_doc.FindAllDescendantByComparison(&objects, &comparison);

    std::vector<Measure *> measures;
    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(measures),
        [](Object *object) { return vrv_cast<Measure *>(object); });
    return measures;
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
        this->UpdateTimeStamps(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::UpdateHairpin(const std::string &uuid, hairpinLog_FORM form)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByUuid(uuid));
    if (hairpin) {
        hairpin->SetForm(form);
        return true;
    }
    return false;
}

bool EnoteToolkit::UpdateHairpin(const std::string &uuid, double startTstamp, data_MEASUREBEAT endTstamp)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByUuid(uuid));
    if (hairpin) {
        const xsdPositiveInteger_List staffNs = hairpin->GetStaff();
        hairpin->TimeSpanningInterface::Reset();
        hairpin->SetStaff(staffNs);
        hairpin->SetTstamp(startTstamp);
        hairpin->SetTstamp2(endTstamp);
        this->UpdateTimeStamps(hairpin);
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

void EnoteToolkit::UpdateTimeStamps(ControlElement *element)
{
    TimeSpanningInterface *interface = element->GetTimeSpanningInterface();
    if (interface) {
        // Set the first timestamp
        Measure *measure = vrv_cast<Measure *>(element->GetFirstAncestor(MEASURE));
        TimestampAttr *timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(interface->GetTstamp());
        interface->SetStart(timestampAttr);

        // Set the second timestamp
        const data_MEASUREBEAT endTstamp = interface->GetTstamp2();
        if (endTstamp.first > 0) {
            std::vector<Measure *> measures = this->FindAllMeasures();
            auto iter = std::find(measures.cbegin(), measures.cend(), measure);
            if (iter != measures.end()) {
                const size_t index = iter - measures.cbegin() + endTstamp.first;
                if ((index >= 0) && (index < measures.size())) {
                    iter = std::next(iter, endTstamp.first);
                    measure = *iter;
                }
                else {
                    vrv::LogWarning("Measure of end timestamp not found, shift is ignored.");
                }
            }
        }
        timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(endTstamp.second);
        interface->SetEnd(timestampAttr);
    }
}

} // namespace vrv
