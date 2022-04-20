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
    std::vector<Measure *> measures = this->FindAllMeasures(&m_doc);
    UuidComparison comparison(MEASURE, uuid);
    auto iter = std::find_if(measures.cbegin(), measures.cend(), comparison);
    return (iter != measures.cend()) ? *iter : NULL;
}

Measure *EnoteToolkit::FindMeasureByN(const std::string &n)
{
    std::vector<Measure *> measures = this->FindAllMeasures(&m_doc);
    AttNNumberLikeComparison comparison(MEASURE, n);
    auto iter = std::find_if(measures.cbegin(), measures.cend(), comparison);
    return (iter != measures.cend()) ? *iter : NULL;
}

std::vector<Measure *> EnoteToolkit::FindAllMeasures()
{
    return this->FindAllMeasures(&m_doc);
}

std::vector<Measure *> EnoteToolkit::FindAllMeasures(Object *parent)
{
    ListOfObjects objects = parent->FindAllDescendantsByType(MEASURE, false);
    std::vector<Measure *> measures;
    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(measures),
        [](Object *object) { return vrv_cast<Measure *>(object); });
    return measures;
}

std::vector<Page *> EnoteToolkit::FindAllPages()
{
    ListOfObjects objects = m_doc.FindAllDescendantsByType(PAGE, false);
    std::vector<Page *> pages;
    std::transform(objects.cbegin(), objects.cend(), std::back_inserter(pages),
        [](Object *object) { return vrv_cast<Page *>(object); });
    return pages;
}

Object *EnoteToolkit::FindElementInMeasure(const std::string &elementUuid, const std::string &measureUuid)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (!measure) return NULL;
    return measure->FindDescendantByUuid(elementUuid);
}

std::list<MeasureRange> EnoteToolkit::GetMeasureRangeForPage(int index)
{
    std::vector<Page *> pages = this->FindAllPages();
    const int pageCount = static_cast<int>(pages.size());
    if ((index < 1) || (index > pageCount)) {
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
        ArrayOfObjects &children = page->GetChildrenForModification();
        for (Object *child : children) {
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

bool EnoteToolkit::EditNote(
    const std::string &noteUuid, const std::string &measureUuid, data_PITCHNAME pitch, data_OCTAVE octave)
{
    Note *note = dynamic_cast<Note *>(this->FindElementInMeasure(noteUuid, measureUuid));
    if (note) {
        note->SetPname(pitch);
        note->SetOct(octave);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasNoteAccidental(const std::string &noteUuid, const std::string &measureUuid)
{
    Note *note = dynamic_cast<Note *>(this->FindElementInMeasure(noteUuid, measureUuid));
    if (note) {
        return (note->GetChildCount(ACCID) > 0);
    }
    return false;
}

bool EnoteToolkit::AddNoteAccidental(
    const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN accidType)
{
    Note *note = dynamic_cast<Note *>(this->FindElementInMeasure(noteUuid, measureUuid));
    if (note) {
        Accid *accid = new Accid();
        accid->SetAccid(accidType);
        note->AddChild(accid);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditNoteAccidental(
    const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN type, bool resetAccidGes)
{
    Note *note = dynamic_cast<Note *>(this->FindElementInMeasure(noteUuid, measureUuid));
    if (note) {
        Accid *accid = vrv_cast<Accid *>(note->GetChild(0, ACCID));
        if (accid) {
            accid->SetAccid(type);
            if (resetAccidGes) accid->ResetAccidentalGestural();
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveNoteAccidental(const std::string &noteUuid, const std::string &measureUuid)
{
    Note *note = dynamic_cast<Note *>(this->FindElementInMeasure(noteUuid, measureUuid));
    if (note) {
        Accid *accid = vrv_cast<Accid *>(note->GetChild(0, ACCID));
        if (accid) {
            note->DeleteChild(accid);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::HasArticulation(const std::string &noteOrChordUuid, const std::string &measureUuid)
{
    return (this->GetArticulationCount(noteOrChordUuid, measureUuid) > 0);
}

bool EnoteToolkit::HasArticulation(
    const std::string &articUuid, const std::string &noteOrChordUuid, const std::string &measureUuid)
{
    Object *parent = this->FindElementInMeasure(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        return (dynamic_cast<Artic *>(parent->FindDescendantByUuid(articUuid)) != NULL);
    }
    return false;
}

int EnoteToolkit::GetArticulationCount(const std::string &noteOrChordUuid, const std::string &measureUuid)
{
    Object *parent = this->FindElementInMeasure(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        return parent->GetChildCount(ARTIC);
    }
    return 0;
}

bool EnoteToolkit::AddArticulation(
    const std::string &noteOrChordUuid, const std::string &measureUuid, data_ARTICULATION type)
{
    return this->AddArticulation("", noteOrChordUuid, measureUuid, type);
}

bool EnoteToolkit::AddArticulation(const std::string &articUuid, const std::string &noteOrChordUuid,
    const std::string &measureUuid, data_ARTICULATION type)
{
    Object *parent = this->FindElementInMeasure(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = new Artic();
        artic->SetArtic({ type });
        if (!articUuid.empty()) artic->SetUuid(articUuid);
        parent->AddChild(artic);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditArticulation(
    const std::string &noteOrChordUuid, const std::string &measureUuid, data_ARTICULATION type)
{
    return this->EditArticulation("", noteOrChordUuid, measureUuid, type);
}

bool EnoteToolkit::EditArticulation(const std::string &articUuid, const std::string &noteOrChordUuid,
    const std::string &measureUuid, data_ARTICULATION type)
{
    Object *parent = this->FindElementInMeasure(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = vrv_cast<Artic *>(parent->GetChild(0, ARTIC));
        if (!articUuid.empty()) artic = dynamic_cast<Artic *>(parent->FindDescendantByUuid(articUuid));
        if (artic) {
            artic->SetArtic({ type });
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveArticulation(const std::string &noteOrChordUuid, const std::string &measureUuid)
{
    return this->RemoveArticulation("", noteOrChordUuid, measureUuid);
}

bool EnoteToolkit::RemoveArticulation(
    const std::string &articUuid, const std::string &noteOrChordUuid, const std::string &measureUuid)
{
    Object *parent = this->FindElementInMeasure(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = vrv_cast<Artic *>(parent->GetChild(0, ARTIC));
        if (!articUuid.empty()) artic = dynamic_cast<Artic *>(parent->FindDescendantByUuid(articUuid));
        if (artic) {
            // At this point parent must not necessarily be the parent of artic
            artic->GetParent()->DeleteChild(artic);
            return true;
        }
    }
    return false;
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
        const int measureCount = static_cast<int>(measures.size());

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
            if (startIndex + endTstamp.first < measureCount) {
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
        for (int index = 0; index < measureCount; ++index) {
            ArrayOfObjects &children = measures.at(index)->GetChildrenForModification();
            for (Object *child : children) {
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
    return -1;
}

} // namespace vrv
