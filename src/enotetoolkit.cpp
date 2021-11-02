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
#include "page.h"
#include "staff.h"
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
    return this->FindAllMeasures(&m_doc);
}

std::vector<Measure *> EnoteToolkit::FindAllMeasures(Object *parent)
{
    ListOfObjects objects;
    ClassIdComparison comparison(MEASURE);
    parent->FindAllDescendantByComparison(&objects, &comparison);

    std::vector<Measure *> measures;
    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(measures),
        [](Object *object) { return vrv_cast<Measure *>(object); });
    return measures;
}

std::vector<Page *> EnoteToolkit::FindAllPages()
{
    ListOfObjects objects;
    ClassIdComparison comparison(PAGE);
    m_doc.FindAllDescendantByComparison(&objects, &comparison);

    std::vector<Page *> pages;
    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(pages),
        [](Object *object) { return vrv_cast<Page *>(object); });
    return pages;
}

std::list<MeasureRange> EnoteToolkit::GetMeasureRangeForPage(int index)
{
    std::vector<Page *> pages = this->FindAllPages();
    if ((index < 1) || (index > pages.size())) {
        vrv::LogWarning("Page not found.");
        return {};
    }
    Page *requestedPage = pages.at(index - 1);

    // Run through all pages and extract the measure ranges
    std::list<MeasureRange> measureRanges;
    std::string mdivUuid;
    int firstN = 0;
    int lastN = 0;
    for (Page *page : pages) {
        ArrayOfObjects *children = page->GetChildrenForModification();
        for (Object *child : *children) {
            if (child->Is(MDIV)) {
                if (firstN > 0) {
                    measureRanges.push_back({ mdivUuid, firstN, lastN });
                    firstN = 0;
                    lastN = 0;
                }
                mdivUuid = child->GetUuid();
            }
            else if (page == requestedPage) {
                std::vector<Measure *> measures = this->FindAllMeasures(child);
                for (Measure *measure : measures) {
                    const int number = this->ExtractNumber(measure->GetN());
                    if (number > 0) {
                        lastN = number;
                        if (firstN == 0) firstN = number;
                    }
                }
            }
        }
        // Update at end of page
        if (firstN > 0) {
            measureRanges.push_back({ mdivUuid, firstN, lastN });
            firstN = 0;
            lastN = 0;
        }
    }
    return measureRanges;
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
        this->UpdateTimeSpanning(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::ChangeHairpinForm(const std::string &uuid, hairpinLog_FORM form)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByUuid(uuid));
    if (hairpin) {
        hairpin->SetForm(form);
        return true;
    }
    return false;
}

bool EnoteToolkit::ChangeHairpinLength(const std::string &uuid, double startTstamp, data_MEASUREBEAT endTstamp)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByUuid(uuid));
    if (hairpin) {
        const xsdPositiveInteger_List staffNs = hairpin->GetStaff();
        hairpin->TimeSpanningInterface::Reset();
        hairpin->SetStaff(staffNs);
        hairpin->SetTstamp(startTstamp);
        hairpin->SetTstamp2(endTstamp);
        this->UpdateTimeSpanning(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::ChangeNotePitch(const std::string &uuid, data_PITCHNAME pitch, data_OCTAVE octave)
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

void EnoteToolkit::UpdateTimeSpanning(ControlElement *element)
{
    TimeSpanningInterface *interface = element->GetTimeSpanningInterface();
    if (interface) {
        // Retrieve all measures
        std::vector<Measure *> measures = this->FindAllMeasures();

        // See Object::PrepareTimestamps
        // Set the first timestamp
        Measure *measure = vrv_cast<Measure *>(element->GetFirstAncestor(MEASURE));
        const auto iterStart = std::find(measures.cbegin(), measures.cend(), measure);
        assert(iterStart != measures.cend());
        const int startIndex = static_cast<int>(iterStart - measures.cbegin());
        TimestampAttr *timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(interface->GetTstamp());
        interface->SetStart(timestampAttr);

        // Set the second timestamp
        const data_MEASUREBEAT endTstamp = interface->GetTstamp2();
        int endIndex = startIndex;
        if (endTstamp.first > 0) {
            if (startIndex + endTstamp.first < measures.size()) {
                const auto iterEnd = std::next(iterStart, endTstamp.first);
                measure = *iterEnd;
                endIndex += endTstamp.first;
            }
            else {
                vrv::LogWarning("Measure of end timestamp not found, shift is ignored.");
            }
        }
        timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(endTstamp.second);
        interface->SetEnd(timestampAttr);

        // See Object::FillStaffCurrentTimeSpanning
        // Check if element must be added or removed to m_timeSpanningElements in the staves
        for (int index = 0; index < measures.size(); ++index) {
            ArrayOfObjects *children = measures.at(index)->GetChildrenForModification();
            for (Object *child : *children) {
                if (!child->Is(STAFF)) continue;
                Staff *staff = vrv_cast<Staff *>(child);
                assert(staff);

                if (!interface->IsOnStaff(staff->GetN())) continue;

                const bool shouldExist = ((index >= startIndex) && (index <= endIndex));
                auto iter
                    = std::find(staff->m_timeSpanningElements.cbegin(), staff->m_timeSpanningElements.cend(), element);
                const bool doesExist = (iter != staff->m_timeSpanningElements.cend());
                if (shouldExist && !doesExist) {
                    staff->m_timeSpanningElements.push_back(element);
                }
                if (!shouldExist && doesExist) {
                    staff->m_timeSpanningElements.erase(iter, iter + 1);
                }
            }
        }
    }
}

int EnoteToolkit::ExtractNumber(const std::string &text)
{
    const size_t index = text.find_first_of("0123456789");
    if (index != std::string::npos) {
        return std::stoi(text.substr(index));
    }
    return 0;
}

} // namespace vrv
