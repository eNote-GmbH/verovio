/////////////////////////////////////////////////////////////////////////////
// Name:        enotetoolkit.cpp
// Author:      David Bauer
// Created:     25/10/2021
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "enotetoolkit.h"

//----------------------------------------------------------------------------

#include "comparison.h"
#include "fing.h"
#include "hairpin.h"
#include "measure.h"
#include "page.h"
#include "slur.h"
#include "staff.h"
#include "text.h"
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

Object *EnoteToolkit::FindElement(const std::string &elementUuid, const std::optional<std::string> &measureUuid)
{
    if (measureUuid) {
        Measure *measure = this->FindMeasureByUuid(*measureUuid);
        if (!measure) return NULL;
        return measure->FindDescendantByUuid(elementUuid);
    }
    else {
        return m_doc.FindDescendantByUuid(elementUuid);
    }
}

Object *EnoteToolkit::FindElementStartingInMeasure(const std::string &startUuid, const std::string &measureUuid)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (!measure) return NULL;
    LayerElement *start = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
    if (!start) return NULL;

    const ArrayOfObjects &children = measure->GetChildren();
    auto iter = std::find_if(children.begin(), children.end(), [&startUuid](Object *child) {
        TimePointInterface *interface = child->GetTimePointInterface();
        return (interface && (interface->GetStartid() == startUuid));
    });
    return (iter != children.end()) ? *iter : NULL;
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

bool EnoteToolkit::HasNote(const std::string &noteUuid, const std::optional<std::string> &measureUuid)
{
    return (dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid)) != NULL);
}

bool EnoteToolkit::EditNote(
    const std::string &noteUuid, const std::string &measureUuid, data_PITCHNAME pitch, data_OCTAVE octave)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
    if (note) {
        note->SetPname(pitch);
        note->SetOct(octave);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasNoteAccidental(const std::string &noteUuid, const std::optional<std::string> &measureUuid)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
    if (note) {
        return (note->GetChildCount(ACCID) > 0);
    }
    return false;
}

bool EnoteToolkit::AddNoteAccidental(
    const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN accidType)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
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
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
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

bool EnoteToolkit::RemoveNoteAccidental(const std::string &noteUuid, const std::optional<std::string> &measureUuid)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
    if (note) {
        Accid *accid = vrv_cast<Accid *>(note->GetChild(0, ACCID));
        if (accid) {
            note->DeleteChild(accid);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::HasArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
    const std::optional<std::string> &measureUuid)
{
    if (articUuid) {
        Object *parent = this->FindElement(noteOrChordUuid, measureUuid);
        if (parent && parent->Is({ CHORD, NOTE })) {
            return (dynamic_cast<Artic *>(parent->FindDescendantByUuid(*articUuid)) != NULL);
        }
        return false;
    }
    else {
        return (this->GetArticulationCount(noteOrChordUuid, measureUuid) > 0);
    }
}

int EnoteToolkit::GetArticulationCount(
    const std::string &noteOrChordUuid, const std::optional<std::string> &measureUuid)
{
    Object *parent = this->FindElement(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        return parent->GetChildCount(ARTIC);
    }
    return 0;
}

bool EnoteToolkit::AddArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
    const std::string &measureUuid, data_ARTICULATION type)
{
    Object *parent = this->FindElement(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = new Artic();
        artic->SetArtic({ type });
        if (articUuid) artic->SetUuid(*articUuid);
        parent->AddChild(artic);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
    const std::string &measureUuid, data_ARTICULATION type, bool resetPlace)
{
    Object *parent = this->FindElement(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = vrv_cast<Artic *>(parent->GetChild(0, ARTIC));
        if (articUuid) artic = dynamic_cast<Artic *>(parent->FindDescendantByUuid(*articUuid));
        if (artic) {
            artic->SetArtic({ type });
            if (resetPlace) artic->ResetPlacementRelEvent();
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
    const std::optional<std::string> &measureUuid)
{
    Object *parent = this->FindElement(noteOrChordUuid, measureUuid);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = vrv_cast<Artic *>(parent->GetChild(0, ARTIC));
        if (articUuid) artic = dynamic_cast<Artic *>(parent->FindDescendantByUuid(*articUuid));
        if (artic) {
            // At this point parent must not necessarily be the parent of artic
            artic->GetParent()->DeleteChild(artic);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::HasHairpin(const std::string &hairpinUuid, const std::optional<std::string> &measureUuid)
{
    return (dynamic_cast<Hairpin *>(this->FindElement(hairpinUuid, measureUuid)) != NULL);
}

bool EnoteToolkit::AddHairpin(const std::optional<std::string> &hairpinUuid, const std::string &measureUuid,
    double tstamp, data_MEASUREBEAT tstamp2, const xsdPositiveInteger_List &staffNs, data_STAFFREL place,
    hairpinLog_FORM form)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (measure) {
        Hairpin *hairpin = new Hairpin();
        if (hairpinUuid) hairpin->SetUuid(*hairpinUuid);
        hairpin->SetTstamp(tstamp);
        hairpin->SetTstamp2(tstamp2);
        hairpin->SetStaff(staffNs);
        hairpin->SetForm(form);
        measure->AddChild(hairpin);
        this->UpdateTimeSpanning(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditHairpin(const std::string &hairpinUuid, const std::string &measureUuid,
    const std::optional<double> &tstamp, const std::optional<data_MEASUREBEAT> &tstamp2,
    const std::optional<xsdPositiveInteger_List> &staffNs, const std::optional<data_STAFFREL> &place,
    const std::optional<hairpinLog_FORM> &form)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByUuid(hairpinUuid));
    if (hairpin) {
        if (this->MoveToMeasure(hairpin, measureUuid)) {
            const double prevTstamp = hairpin->GetTstamp();
            const data_MEASUREBEAT prevTstamp2 = hairpin->GetTstamp2();
            hairpin->TimeSpanningInterface::Reset();
            hairpin->SetTstamp(tstamp ? *tstamp : prevTstamp);
            hairpin->SetTstamp2(tstamp2 ? *tstamp2 : prevTstamp2);
            if (staffNs) hairpin->SetStaff(*staffNs);
            if (form) hairpin->SetForm(*form);
            this->UpdateTimeSpanning(hairpin);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveHairpin(const std::string &hairpinUuid, const std::optional<std::string> &measureUuid)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(this->FindElement(hairpinUuid, measureUuid));
    if (hairpin) {
        this->RemoveTimeSpanning(hairpin);
        hairpin->GetParent()->DeleteChild(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasSlur(const std::string &slurUuid, const std::optional<std::string> &measureUuid)
{
    return (dynamic_cast<Slur *>(this->FindElement(slurUuid, measureUuid)) != NULL);
}

bool EnoteToolkit::AddSlur(const std::optional<std::string> &slurUuid, const std::string &measureUuid,
    const std::string &startUuid, const std::string &endUuid, const std::optional<curvature_CURVEDIR> &curveDir)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
        LayerElement *endElement = dynamic_cast<LayerElement *>(m_doc.FindDescendantByUuid(endUuid));
        if (startElement && endElement && Object::IsPreOrdered(startElement, endElement)) {
            Slur *slur = new Slur();
            if (slurUuid) slur->SetUuid(*slurUuid);
            slur->SetStartid(startUuid);
            slur->SetEndid(endUuid);
            if (curveDir) slur->SetCurvedir(*curveDir);
            measure->AddChild(slur);
            this->UpdateTimeSpanning(slur);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::AddSlur(const std::optional<std::string> &slurUuid, const std::string &measureUuid,
    const std::string &startUuid, data_MEASUREBEAT tstamp2, const std::optional<curvature_CURVEDIR> &curveDir)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
        if (startElement) {
            Slur *slur = new Slur();
            if (slurUuid) slur->SetUuid(*slurUuid);
            slur->SetStartid(startUuid);
            slur->SetTstamp2(tstamp2);
            if (curveDir) slur->SetCurvedir(*curveDir);
            measure->AddChild(slur);
            this->UpdateTimeSpanning(slur);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::EditSlur(const std::string &slurUuid, const std::string &measureUuid, const std::string &startUuid,
    const std::string &endUuid)
{
    Slur *slur = dynamic_cast<Slur *>(m_doc.FindDescendantByUuid(slurUuid));
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (slur && measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
        LayerElement *endElement = dynamic_cast<LayerElement *>(m_doc.FindDescendantByUuid(endUuid));
        if (startElement && endElement && Object::IsPreOrdered(startElement, endElement)) {
            if (this->MoveToMeasure(slur, measure)) {
                slur->TimeSpanningInterface::Reset();
                slur->SetStartid(startUuid);
                slur->SetEndid(endUuid);
                this->UpdateTimeSpanning(slur);
                return true;
            }
        }
    }
    return false;
}

bool EnoteToolkit::EditSlur(
    const std::string &slurUuid, const std::string &measureUuid, const std::string &startUuid, data_MEASUREBEAT tstamp2)
{
    Slur *slur = dynamic_cast<Slur *>(m_doc.FindDescendantByUuid(slurUuid));
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (slur && measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
        if (startElement) {
            if (this->MoveToMeasure(slur, measure)) {
                slur->TimeSpanningInterface::Reset();
                slur->SetStartid(startUuid);
                slur->SetTstamp2(tstamp2);
                this->UpdateTimeSpanning(slur);
                return true;
            }
        }
    }
    return false;
}

bool EnoteToolkit::EditSlur(
    const std::string &slurUuid, const std::optional<std::string> &measureUuid, curvature_CURVEDIR curveDir)
{
    Slur *slur = dynamic_cast<Slur *>(this->FindElement(slurUuid, measureUuid));
    if (slur) {
        slur->SetCurvedir(curveDir);
        return true;
    }
    return false;
}

bool EnoteToolkit::RemoveSlur(const std::string &slurUuid, const std::optional<std::string> &measureUuid)
{
    Slur *slur = dynamic_cast<Slur *>(this->FindElement(slurUuid, measureUuid));
    if (slur) {
        this->RemoveTimeSpanning(slur);
        slur->GetParent()->DeleteChild(slur);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasFing(const std::string &fingUuid, const std::optional<std::string> &measureUuid)
{
    return (dynamic_cast<Fing *>(this->FindElement(fingUuid, measureUuid)) != NULL);
}

bool EnoteToolkit::HasFingOfNote(const std::optional<std::string> &fingUuid, const std::string &noteUuid,
    const std::optional<std::string> &measureUuid)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
    if (note) {
        Measure *measure = vrv_cast<Measure *>(note->GetFirstAncestor(MEASURE));
        assert(measure);
        Fing *fing = dynamic_cast<Fing *>(fingUuid ? measure->FindDescendantByUuid(*fingUuid)
                                                   : this->FindElementStartingInMeasure(noteUuid, measure->GetUuid()));
        return (fing && (fing->GetStartid() == noteUuid));
    }
    return false;
}

bool EnoteToolkit::AddFingToNote(
    const std::string &noteUuid, const std::string &measureUuid, data_STAFFREL place, const std::string &value)
{
    return this->AddFingToNote("", noteUuid, measureUuid, place, value);
}

bool EnoteToolkit::AddFingToNote(const std::string &fingUuid, const std::string &noteUuid,
    const std::string &measureUuid, data_STAFFREL place, const std::string &value)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (measure) {
        Note *note = dynamic_cast<Note *>(measure->FindDescendantByUuid(noteUuid));
        if (note) {
            Fing *fing = new Fing();
            if (!fingUuid.empty()) fing->SetUuid(fingUuid);
            fing->SetStartid(noteUuid);
            fing->SetPlace(place);
            fing->SetN(value);
            this->SetTextChildren(fing, { value });
            measure->AddChild(fing);
            this->UpdateTimePoint(fing);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::EditFingOfNote(
    const std::string &noteUuid, const std::string &measureUuid, data_STAFFREL place, const std::string &value)
{
    Fing *fing = dynamic_cast<Fing *>(this->FindElementStartingInMeasure(noteUuid, measureUuid));
    if (fing) {
        return this->EditFingOfNote(fing->GetUuid(), noteUuid, measureUuid, place, value);
    }
    return false;
}

bool EnoteToolkit::EditFingOfNote(const std::string &fingUuid, const std::string &noteUuid,
    const std::string &measureUuid, data_STAFFREL place, const std::string &value)
{
    Measure *measure = this->FindMeasureByUuid(measureUuid);
    if (measure) {
        Fing *fing = dynamic_cast<Fing *>(m_doc.FindDescendantByUuid(fingUuid));
        Note *note = dynamic_cast<Note *>(measure->FindDescendantByUuid(noteUuid));
        if (fing && note && (this->MoveToMeasure(fing, measure))) {
            fing->TimePointInterface::Reset();
            fing->SetStartid(noteUuid);
            fing->SetPlace(place);
            fing->SetN(value);
            this->SetTextChildren(fing, { value });
            this->UpdateTimePoint(fing);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveFing(const std::string &fingUuid, const std::optional<std::string> &measureUuid)
{
    Fing *fing = dynamic_cast<Fing *>(this->FindElement(fingUuid, measureUuid));
    if (fing) {
        fing->GetParent()->DeleteChild(fing);
        return true;
    }
    return false;
}

bool EnoteToolkit::RemoveFingOfNote(const std::optional<std::string> &fingUuid, const std::string &noteUuid,
    const std::optional<std::string> &measureUuid)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteUuid, measureUuid));
    if (note) {
        Measure *measure = vrv_cast<Measure *>(note->GetFirstAncestor(MEASURE));
        assert(measure);
        Fing *fing = dynamic_cast<Fing *>(fingUuid ? measure->FindDescendantByUuid(*fingUuid)
                                                   : this->FindElementStartingInMeasure(noteUuid, measure->GetUuid()));
        if (fing && (fing->GetStartid() == noteUuid)) {
            fing->GetParent()->DeleteChild(fing);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::MoveToMeasure(ControlElement *element, const std::string &measureUuid)
{
    Measure *measure = vrv_cast<Measure *>(element->GetFirstAncestor(MEASURE));
    if (measure->GetUuid() == measureUuid) return true;

    // Find the target measure
    measure = this->FindMeasureByUuid(measureUuid);
    if (measure) {
        element->MoveItselfTo(measure);
        return true;
    }
    return false;
}

bool EnoteToolkit::MoveToMeasure(ControlElement *element, Measure *measure)
{
    Measure *parentMeasure = vrv_cast<Measure *>(element->GetFirstAncestor(MEASURE));
    if (parentMeasure == measure) return true;

    if (measure) {
        element->MoveItselfTo(measure);
        parentMeasure->ClearRelinquishedChildren();
        return true;
    }
    return false;
}

void EnoteToolkit::SetTextChildren(ControlElement *element, const std::list<std::string> &textEntries)
{
    // Delete existing text children
    ListOfObjects textChildren;
    const ArrayOfObjects &allChildren = element->GetChildren();
    std::copy_if(allChildren.cbegin(), allChildren.cend(), std::back_inserter(textChildren),
        [](Object *child) { return child->IsTextElement(); });
    std::for_each(textChildren.begin(), textChildren.end(), [element](Object *child) { element->DeleteChild(child); });

    // Add new text children
    std::for_each(textEntries.cbegin(), textEntries.cend(), [element](const std::string &entry) {
        Text *text = new Text();
        text->SetText(UTF8to16(entry));
        element->AddChild(text);
    });
}

void EnoteToolkit::UpdateTimePoint(ControlElement *element)
{
    TimePointInterface *interface = element->GetTimePointInterface();
    if (interface) {
        // Set the start or first timestamp
        Measure *measure = vrv_cast<Measure *>(element->GetFirstAncestor(MEASURE));
        if (interface->HasStartid()) {
            const std::string startUuid = interface->GetStartid();
            LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
            if (startElement) {
                interface->SetStart(startElement);
            }
            else {
                vrv::LogWarning("Start element not found. Please check that the control element is encoded in the "
                                "measure of its start element!");
            }
        }
        else if (interface->HasTstamp()) {
            TimestampAttr *timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(interface->GetTstamp());
            interface->SetStart(timestampAttr);
        }
    }
}

void EnoteToolkit::UpdateTimeSpanning(ControlElement *element)
{
    TimeSpanningInterface *interface = element->GetTimeSpanningInterface();
    if (interface) {
        // Retrieve all measures
        std::vector<Measure *> measures = this->FindAllMeasures();
        const int measureCount = static_cast<int>(measures.size());

        // See Object::PrepareTimestamps
        // Set the start or first timestamp
        Measure *measure = vrv_cast<Measure *>(element->GetFirstAncestor(MEASURE));
        const auto iterStart = std::find(measures.cbegin(), measures.cend(), measure);
        assert(iterStart != measures.cend());
        const int startIndex = static_cast<int>(iterStart - measures.cbegin());
        if (interface->HasStartid()) {
            const std::string startUuid = interface->GetStartid();
            LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByUuid(startUuid));
            if (startElement) {
                interface->SetStart(startElement);
            }
            else {
                vrv::LogWarning("Start element not found. Please check that the control element is encoded in the "
                                "measure of its start element!");
            }
        }
        else if (interface->HasTstamp()) {
            TimestampAttr *timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(interface->GetTstamp());
            interface->SetStart(timestampAttr);
        }

        // Set the end or second timestamp
        int endIndex = startIndex;
        if (interface->HasEndid()) {
            const std::string endUuid = interface->GetEndid();
            LayerElement *endElement = dynamic_cast<LayerElement *>(m_doc.FindDescendantByUuid(endUuid));
            if (endElement) {
                measure = vrv_cast<Measure *>(endElement->GetFirstAncestor(MEASURE));
                const auto iterEnd = std::find(measures.cbegin(), measures.cend(), measure);
                endIndex = static_cast<int>(iterEnd - measures.cbegin());
                interface->SetEnd(endElement);
            }
        }
        else if (interface->HasTstamp2()) {
            const data_MEASUREBEAT endTstamp = interface->GetTstamp2();
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
            TimestampAttr *timestampAttr = measure->m_timestampAligner.GetTimestampAtTime(endTstamp.second);
            interface->SetEnd(timestampAttr);
        }

        // See Object::FillStaffCurrentTimeSpanning
        // Check if element must be added or removed to m_timeSpanningElements in the staves
        for (int index = 0; index < measureCount; ++index) {
            ArrayOfObjects &children = measures.at(index)->GetChildrenForModification();
            for (Object *child : children) {
                if (!child->Is(STAFF)) continue;
                Staff *staff = vrv_cast<Staff *>(child);
                assert(staff);

                const bool shouldExist
                    = ((index >= startIndex) && (index <= endIndex) && interface->IsOnStaff(staff->GetN()));
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

void EnoteToolkit::RemoveTimeSpanning(ControlElement *element)
{
    TimeSpanningInterface *interface = element->GetTimeSpanningInterface();
    if (interface) {
        // Retrieve all measures
        std::vector<Measure *> measures = this->FindAllMeasures();
        const int measureCount = static_cast<int>(measures.size());

        // See Object::FillStaffCurrentTimeSpanning
        // Remove element from m_timeSpanningElements in the staves
        for (int index = 0; index < measureCount; ++index) {
            ArrayOfObjects &children = measures.at(index)->GetChildrenForModification();
            for (Object *child : children) {
                if (!child->Is(STAFF)) continue;
                Staff *staff = vrv_cast<Staff *>(child);
                assert(staff);

                if (!interface->IsOnStaff(staff->GetN())) continue;

                auto iter
                    = std::find(staff->m_timeSpanningElements.cbegin(), staff->m_timeSpanningElements.cend(), element);
                if (iter != staff->m_timeSpanningElements.cend()) {
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
