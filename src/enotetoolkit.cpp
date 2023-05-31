/////////////////////////////////////////////////////////////////////////////
// Name:        enotetoolkit.cpp
// Author:      David Bauer
// Created:     25/10/2021
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "enotetoolkit.h"

//----------------------------------------------------------------------------

#include "comparison.h"
#include "dynam.h"
#include "fing.h"
#include "hairpin.h"
#include "iomei.h"
#include "measure.h"
#include "page.h"
#include "pages.h"
#include "pedal.h"
#include "slur.h"
#include "staff.h"
#include "text.h"
#include "timestamp.h"

//----------------------------------------------------------------------------

namespace vrv {

const std::string UserContentType = "UserContent";

//----------------------------------------------------------------------------
// EnoteToolkit
//----------------------------------------------------------------------------

EnoteToolkit::EnoteToolkit(bool initFont) : Toolkit(initFont) {}

EnoteToolkit::~EnoteToolkit() {}

void EnoteToolkit::CancelLayout()
{
    m_doc.SetAbortMode(true);
}

void EnoteToolkit::ContinueLayout()
{
    m_doc.SetAbortMode(false);
}

Measure *EnoteToolkit::FindMeasureByID(const std::string &id)
{
    std::vector<Measure *> measures = this->FindAllMeasures(&m_doc);
    IDComparison comparison(MEASURE, id);
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

std::optional<int> EnoteToolkit::FindPageIndex(Page *page)
{
    Pages *pages = m_doc.GetPages();
    if (pages) {
        const int index = pages->GetChildIndex(page);
        if (index >= 0) return index + 1;
    }
    return std::nullopt;
}

std::set<int> EnoteToolkit::FindPageIndices(const std::string &id)
{
    std::set<int> indices;
    Object *element = this->FindElement(id, std::nullopt);
    if (element) {
        // Determine the first and last page of the element
        Page *firstPage = vrv_cast<Page *>(element->GetFirstAncestor(PAGE));
        Page *lastPage = firstPage;
        TimeSpanningInterface *interface = element->GetTimeSpanningInterface();
        if (interface) {
            Object *start = interface->GetStart();
            Object *end = interface->GetEnd();
            if (start && end) {
                firstPage = vrv_cast<Page *>(start->GetFirstAncestor(PAGE));
                lastPage = vrv_cast<Page *>(end->GetFirstAncestor(PAGE));
            }
        }
        // Fill the page indices
        const auto firstIndex = this->FindPageIndex(firstPage);
        const auto lastIndex = this->FindPageIndex(lastPage);
        if (firstIndex && lastIndex) {
            for (int index = *firstIndex; index <= *lastIndex; ++index) {
                indices.insert(index);
            }
        }
    }
    return indices;
}

std::set<int> EnoteToolkit::FindPageIndices(const std::list<std::string> &ids)
{
    std::set<int> indices;
    for (const auto &id : ids) {
        const std::set<int> elementIndices = this->FindPageIndices(id);
        indices.insert(elementIndices.begin(), elementIndices.end());
    }
    return indices;
}

Object *EnoteToolkit::FindElement(const std::string &elementID, const std::optional<std::string> &measureID)
{
    if (measureID) {
        Measure *measure = this->FindMeasureByID(*measureID);
        if (!measure) return NULL;
        return measure->FindDescendantByID(elementID);
    }
    else {
        return m_doc.FindDescendantByID(elementID);
    }
}

Object *EnoteToolkit::FindElementStartingInMeasure(const std::string &startID, const std::string &measureID)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (!measure) return NULL;
    LayerElement *start = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
    if (!start) return NULL;

    const ArrayOfObjects &children = measure->GetChildren();
    auto iter = std::find_if(children.begin(), children.end(), [&startID](Object *child) {
        TimePointInterface *interface = child->GetTimePointInterface();
        return (interface && (interface->GetStartid() == startID));
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
    std::string mdivID;
    int firstN = 0;
    int lastN = 0;
    for (Page *page : pages) {
        ArrayOfObjects &children = page->GetChildrenForModification();
        for (Object *child : children) {
            if (child->Is(MDIV)) {
                if (firstN > 0) {
                    measureRanges.push_back({ mdivID, firstN, lastN });
                    firstN = 0;
                    lastN = 0;
                }
                mdivID = child->GetID();
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
            measureRanges.push_back({ mdivID, firstN, lastN });
            firstN = 0;
            lastN = 0;
        }
    }
    return measureRanges;
}

bool EnoteToolkit::WasEdited(const std::string &elementID, const std::optional<std::string> &measureID)
{
    Object *element = this->FindElement(elementID, measureID);
    if (element && element->HasAttClass(ATT_TYPED)) {
        AttTyped *att = dynamic_cast<AttTyped *>(element);
        assert(att);
        return (att->GetType() == UserContentType);
    }
    return false;
}

void EnoteToolkit::SetEdited(
    const std::string &elementID, const std::optional<std::string> &measureID, bool wasEdited, bool includeDescendants)
{
    Object *element = this->FindElement(elementID, measureID);
    if (element) {
        this->SetEdited(element, wasEdited, includeDescendants);
    }
}

void EnoteToolkit::SetEdited(Object *object, bool wasEdited, bool recursive)
{
    if (object->HasAttClass(ATT_TYPED)) {
        AttTyped *att = dynamic_cast<AttTyped *>(object);
        assert(att);
        att->SetType(wasEdited ? UserContentType : "");
    }
    if (recursive) {
        for (Object *child : object->GetChildren()) {
            this->SetEdited(child, wasEdited, true);
        }
    }
}

bool EnoteToolkit::HasNote(const std::string &noteID, const std::optional<std::string> &measureID)
{
    return (dynamic_cast<Note *>(this->FindElement(noteID, measureID)) != NULL);
}

bool EnoteToolkit::EditNote(
    const std::string &noteID, const std::optional<std::string> &measureID, data_PITCHNAME pitch, data_OCTAVE octave)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        note->SetPname(pitch);
        note->SetOct(octave);
        note->SetType(UserContentType);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditNote(
    const std::string &noteID, const std::optional<std::string> &measureID, pugi::xml_node xmlNote)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        note->ReadPitch(xmlNote);
        note->ReadOctave(xmlNote);
        note->SetType(UserContentType);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasNoteAccidental(const std::string &noteID, const std::optional<std::string> &measureID)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        return (note->GetChildCount(ACCID) > 0);
    }
    return false;
}

bool EnoteToolkit::AddNoteAccidental(
    const std::string &noteID, const std::optional<std::string> &measureID, data_ACCIDENTAL_WRITTEN accidType)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Accid *accid = new Accid();
        accid->SetAccid(accidType);
        accid->SetType(UserContentType);
        note->AddChild(accid);
        return true;
    }
    return false;
}

bool EnoteToolkit::AddNoteAccidental(
    const std::string &noteID, const std::optional<std::string> &measureID, pugi::xml_node xmlNoteOrAccid)
{
    Accid tempAccid;
    tempAccid.ReadAccidental(xmlNoteOrAccid);
    if (tempAccid.HasAccid()) {
        return this->AddNoteAccidental(noteID, measureID, tempAccid.GetAccid());
    }
    return false;
}

bool EnoteToolkit::EditNoteAccidental(const std::string &noteID, const std::optional<std::string> &measureID,
    data_ACCIDENTAL_WRITTEN type, bool resetAccidGes)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Accid *accid = vrv_cast<Accid *>(note->GetChild(0, ACCID));
        if (accid) {
            accid->SetAccid(type);
            if (resetAccidGes) accid->ResetAccidentalGes();
            accid->SetType(UserContentType);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveNoteAccidental(const std::string &noteID, const std::optional<std::string> &measureID)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Accid *accid = vrv_cast<Accid *>(note->GetChild(0, ACCID));
        if (accid) {
            note->DeleteChild(accid);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::HasArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
    const std::optional<std::string> &measureID)
{
    if (articID) {
        Object *parent = this->FindElement(noteOrChordID, measureID);
        if (parent && parent->Is({ CHORD, NOTE })) {
            return (dynamic_cast<Artic *>(parent->FindDescendantByID(*articID)) != NULL);
        }
        return false;
    }
    else {
        return (this->GetArticulationCount(noteOrChordID, measureID) > 0);
    }
}

int EnoteToolkit::GetArticulationCount(const std::string &noteOrChordID, const std::optional<std::string> &measureID)
{
    Object *parent = this->FindElement(noteOrChordID, measureID);
    if (parent && parent->Is({ CHORD, NOTE })) {
        return parent->GetChildCount(ARTIC);
    }
    return 0;
}

bool EnoteToolkit::AddArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
    const std::optional<std::string> &measureID, data_ARTICULATION type)
{
    Object *parent = this->FindElement(noteOrChordID, measureID);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = new Artic();
        artic->SetArtic({ type });
        if (articID) artic->SetID(*articID);
        artic->SetType(UserContentType);
        parent->AddChild(artic);
        return true;
    }
    return false;
}

bool EnoteToolkit::AddArticulation(
    const std::string &noteOrChordID, const std::optional<std::string> &measureID, pugi::xml_node xmlNoteOrArtic)
{
    Artic tempArtic;
    tempArtic.ReadArticulation(xmlNoteOrArtic);
    const data_ARTICULATION_List artics = tempArtic.GetArtic();
    if (artics.empty()) return false;
    return std::all_of(artics.begin(), artics.end(), [&](data_ARTICULATION articType) {
        return this->AddArticulation(std::nullopt, noteOrChordID, measureID, articType);
    });
}

bool EnoteToolkit::EditArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
    const std::optional<std::string> &measureID, data_ARTICULATION type, bool resetPlace)
{
    Object *parent = this->FindElement(noteOrChordID, measureID);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = vrv_cast<Artic *>(parent->GetChild(0, ARTIC));
        if (articID) artic = dynamic_cast<Artic *>(parent->FindDescendantByID(*articID));
        if (artic) {
            artic->SetArtic({ type });
            if (resetPlace) artic->ResetPlacementRelEvent();
            artic->SetType(UserContentType);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
    const std::optional<std::string> &measureID)
{
    Object *parent = this->FindElement(noteOrChordID, measureID);
    if (parent && parent->Is({ CHORD, NOTE })) {
        Artic *artic = vrv_cast<Artic *>(parent->GetChild(0, ARTIC));
        if (articID) artic = dynamic_cast<Artic *>(parent->FindDescendantByID(*articID));
        if (artic) {
            // At this point parent must not necessarily be the parent of artic
            artic->GetParent()->DeleteChild(artic);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::HasHairpin(const std::string &hairpinID, const std::optional<std::string> &measureID)
{
    return (dynamic_cast<Hairpin *>(this->FindElement(hairpinID, measureID)) != NULL);
}

bool EnoteToolkit::AddHairpin(const std::optional<std::string> &hairpinID, const std::string &measureID, double tstamp,
    data_MEASUREBEAT tstamp2, const xsdPositiveInteger_List &staffNs, data_STAFFREL place, hairpinLog_FORM form)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        Hairpin *hairpin = new Hairpin();
        if (hairpinID) hairpin->SetID(*hairpinID);
        hairpin->SetTstamp(tstamp);
        hairpin->SetTstamp2(tstamp2);
        hairpin->SetStaff(staffNs);
        hairpin->SetForm(form);
        hairpin->SetType(UserContentType);
        measure->AddChild(hairpin);
        this->UpdateTimeSpanning(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::AddHairpin(const std::string &measureID, pugi::xml_node xmlHairpin)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        MEIInput input(&m_doc);
        input.ReadHairpin(measure, xmlHairpin);
        Hairpin *hairpin = vrv_cast<Hairpin *>(measure->GetLast(HAIRPIN));
        if (hairpin) this->UpdateTimeSpanning(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditHairpin(const std::string &hairpinID, const std::optional<std::string> &measureID,
    const std::optional<double> &tstamp, const std::optional<data_MEASUREBEAT> &tstamp2,
    const std::optional<xsdPositiveInteger_List> &staffNs, const std::optional<data_STAFFREL> &place,
    const std::optional<hairpinLog_FORM> &form)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(m_doc.FindDescendantByID(hairpinID));
    if (hairpin) {
        const bool ok = measureID ? this->MoveToMeasure(hairpin, *measureID) : true;
        if (ok) {
            const xsdPositiveInteger_List prevStaffNs = hairpin->GetStaff();
            const double prevTstamp = hairpin->GetTstamp();
            const data_MEASUREBEAT prevTstamp2 = hairpin->GetTstamp2();
            hairpin->TimeSpanningInterface::Reset();
            hairpin->SetTstamp(tstamp ? *tstamp : prevTstamp);
            hairpin->SetTstamp2(tstamp2 ? *tstamp2 : prevTstamp2);
            hairpin->SetStaff(staffNs ? *staffNs : prevStaffNs);
            if (form) hairpin->SetForm(*form);
            hairpin->SetType(UserContentType);
            this->UpdateTimeSpanning(hairpin);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveHairpin(const std::string &hairpinID, const std::optional<std::string> &measureID)
{
    Hairpin *hairpin = dynamic_cast<Hairpin *>(this->FindElement(hairpinID, measureID));
    if (hairpin) {
        this->RemoveTimeSpanning(hairpin);
        hairpin->GetParent()->DeleteChild(hairpin);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasSlur(const std::string &slurID, const std::optional<std::string> &measureID)
{
    return (dynamic_cast<Slur *>(this->FindElement(slurID, measureID)) != NULL);
}

bool EnoteToolkit::AddSlur(const std::optional<std::string> &slurID, const std::string &measureID,
    const std::string &startID, const std::string &endID, const std::optional<curvature_CURVEDIR> &curveDir)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
        LayerElement *endElement = dynamic_cast<LayerElement *>(m_doc.FindDescendantByID(endID));
        if (startElement && endElement && Object::IsPreOrdered(startElement, endElement)) {
            Slur *slur = new Slur();
            if (slurID) slur->SetID(*slurID);
            slur->SetStartid(startID);
            slur->SetEndid(endID);
            if (curveDir) slur->SetCurvedir(*curveDir);
            slur->SetType(UserContentType);
            measure->AddChild(slur);
            this->UpdateTimeSpanning(slur);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::AddSlur(const std::optional<std::string> &slurID, const std::string &measureID,
    const std::string &startID, data_MEASUREBEAT tstamp2, const std::optional<curvature_CURVEDIR> &curveDir)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
        if (startElement) {
            Slur *slur = new Slur();
            if (slurID) slur->SetID(*slurID);
            slur->SetStartid(startID);
            slur->SetTstamp2(tstamp2);
            if (curveDir) slur->SetCurvedir(*curveDir);
            slur->SetType(UserContentType);
            measure->AddChild(slur);
            this->UpdateTimeSpanning(slur);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::AddSlur(const std::string &measureID, pugi::xml_node xmlSlur)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        MEIInput input(&m_doc);
        input.ReadSlur(measure, xmlSlur);
        Slur *slur = vrv_cast<Slur *>(measure->GetLast(SLUR));
        if (slur) this->UpdateTimeSpanning(slur);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditSlur(
    const std::string &slurID, const std::string &measureID, const std::string &startID, const std::string &endID)
{
    Slur *slur = dynamic_cast<Slur *>(m_doc.FindDescendantByID(slurID));
    Measure *measure = this->FindMeasureByID(measureID);
    if (slur && measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
        LayerElement *endElement = dynamic_cast<LayerElement *>(m_doc.FindDescendantByID(endID));
        if (startElement && endElement && Object::IsPreOrdered(startElement, endElement)) {
            if (this->MoveToMeasure(slur, measure)) {
                slur->TimeSpanningInterface::Reset();
                slur->SetStartid(startID);
                slur->SetEndid(endID);
                slur->SetType(UserContentType);
                this->UpdateTimeSpanning(slur);
                return true;
            }
        }
    }
    return false;
}

bool EnoteToolkit::EditSlur(
    const std::string &slurID, const std::string &measureID, const std::string &startID, data_MEASUREBEAT tstamp2)
{
    Slur *slur = dynamic_cast<Slur *>(m_doc.FindDescendantByID(slurID));
    Measure *measure = this->FindMeasureByID(measureID);
    if (slur && measure) {
        LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
        if (startElement) {
            if (this->MoveToMeasure(slur, measure)) {
                slur->TimeSpanningInterface::Reset();
                slur->SetStartid(startID);
                slur->SetTstamp2(tstamp2);
                slur->SetType(UserContentType);
                this->UpdateTimeSpanning(slur);
                return true;
            }
        }
    }
    return false;
}

bool EnoteToolkit::EditSlur(
    const std::string &slurID, const std::optional<std::string> &measureID, curvature_CURVEDIR curveDir)
{
    Slur *slur = dynamic_cast<Slur *>(this->FindElement(slurID, measureID));
    if (slur) {
        slur->SetCurvedir(curveDir);
        slur->SetType(UserContentType);
        return true;
    }
    return false;
}

bool EnoteToolkit::RemoveSlur(const std::string &slurID, const std::optional<std::string> &measureID)
{
    Slur *slur = dynamic_cast<Slur *>(this->FindElement(slurID, measureID));
    if (slur) {
        this->RemoveTimeSpanning(slur);
        slur->GetParent()->DeleteChild(slur);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasFing(const std::string &fingID, const std::optional<std::string> &measureID)
{
    return (dynamic_cast<Fing *>(this->FindElement(fingID, measureID)) != NULL);
}

bool EnoteToolkit::HasFingOfNote(
    const std::optional<std::string> &fingID, const std::string &noteID, const std::optional<std::string> &measureID)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Measure *measure = vrv_cast<Measure *>(note->GetFirstAncestor(MEASURE));
        assert(measure);
        Fing *fing = dynamic_cast<Fing *>(fingID ? measure->FindDescendantByID(*fingID)
                                                 : this->FindElementStartingInMeasure(noteID, measure->GetID()));
        return (fing && (fing->GetStartid() == noteID));
    }
    return false;
}

bool EnoteToolkit::AddFingToNote(const std::optional<std::string> &fingID, const std::string &noteID,
    const std::optional<std::string> &measureID, const std::optional<data_STAFFREL> &place, const std::string &value)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Measure *measure = vrv_cast<Measure *>(note->GetFirstAncestor(MEASURE));
        assert(measure);
        Fing *fing = new Fing();
        if (fingID) fing->SetID(*fingID);
        fing->SetStartid(noteID);
        if (place) fing->SetPlace(*place);
        fing->SetN(value);
        this->SetTextChildren(fing, { value });
        fing->SetType(UserContentType);
        measure->AddChild(fing);
        this->UpdateTimePoint(fing);
        return true;
    }
    return false;
}

bool EnoteToolkit::AddFing(const std::string &measureID, pugi::xml_node xmlFing)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        MEIInput input(&m_doc);
        input.ReadFing(measure, xmlFing);
        Fing *fing = vrv_cast<Fing *>(measure->GetLast(FING));
        if (fing) this->UpdateTimePoint(fing);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditFingOfNote(const std::optional<std::string> &fingID, const std::string &noteID,
    const std::optional<std::string> &measureID, const std::optional<data_STAFFREL> &place,
    const std::optional<std::string> &value)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Measure *measure = vrv_cast<Measure *>(note->GetFirstAncestor(MEASURE));
        assert(measure);
        Fing *fing = dynamic_cast<Fing *>(fingID ? measure->FindDescendantByID(*fingID)
                                                 : this->FindElementStartingInMeasure(noteID, measure->GetID()));
        if (fing && (fing->GetStartid() == noteID)) {
            if (place) fing->SetPlace(*place);
            if (value) {
                fing->SetN(*value);
                this->SetTextChildren(fing, { *value });
            }
            fing->SetType(UserContentType);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveFing(const std::string &fingID, const std::optional<std::string> &measureID)
{
    Fing *fing = dynamic_cast<Fing *>(this->FindElement(fingID, measureID));
    if (fing) {
        fing->GetParent()->DeleteChild(fing);
        return true;
    }
    return false;
}

bool EnoteToolkit::RemoveFingOfNote(
    const std::optional<std::string> &fingID, const std::string &noteID, const std::optional<std::string> &measureID)
{
    Note *note = dynamic_cast<Note *>(this->FindElement(noteID, measureID));
    if (note) {
        Measure *measure = vrv_cast<Measure *>(note->GetFirstAncestor(MEASURE));
        assert(measure);
        Fing *fing = dynamic_cast<Fing *>(fingID ? measure->FindDescendantByID(*fingID)
                                                 : this->FindElementStartingInMeasure(noteID, measure->GetID()));
        if (fing && (fing->GetStartid() == noteID)) {
            fing->GetParent()->DeleteChild(fing);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::HasDynam(const std::string &dynamID, const std::optional<std::string> &measureID)
{
    return (dynamic_cast<Dynam *>(this->FindElement(dynamID, measureID)) != NULL);
}

bool EnoteToolkit::AddDynam(const std::optional<std::string> &dynamID, const std::string &measureID, double tstamp,
    const xsdPositiveInteger_List &staffNs, data_STAFFREL place, const std::string &value)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        Dynam *dynam = new Dynam();
        if (dynamID) dynam->SetID(*dynamID);
        dynam->SetTstamp(tstamp);
        dynam->SetStaff(staffNs);
        dynam->SetPlace(place);
        this->SetTextChildren(dynam, { value });
        dynam->SetType(UserContentType);
        measure->AddChild(dynam);
        this->UpdateTimeSpanning(dynam);
        return true;
    }
    return false;
}

bool EnoteToolkit::AddDynam(const std::string &measureID, pugi::xml_node xmlDynam)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        MEIInput input(&m_doc);
        input.ReadDynam(measure, xmlDynam);
        Dynam *dynam = vrv_cast<Dynam *>(measure->GetLast(DYNAM));
        if (dynam) this->UpdateTimeSpanning(dynam);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditDynam(const std::string &dynamID, const std::optional<std::string> &measureID,
    const std::optional<double> &tstamp, const std::optional<xsdPositiveInteger_List> &staffNs,
    const std::optional<data_STAFFREL> &place, const std::optional<std::string> &value)
{
    Dynam *dynam = dynamic_cast<Dynam *>(m_doc.FindDescendantByID(dynamID));
    if (dynam) {
        const bool ok = measureID ? this->MoveToMeasure(dynam, *measureID) : true;
        if (ok) {
            const xsdPositiveInteger_List prevStaffNs = dynam->GetStaff();
            const double prevTstamp = dynam->GetTstamp();
            const data_MEASUREBEAT prevTstamp2 = dynam->GetTstamp2();
            dynam->TimeSpanningInterface::Reset();
            dynam->SetTstamp(tstamp ? *tstamp : prevTstamp);
            dynam->SetTstamp2(prevTstamp2);
            dynam->SetStaff(staffNs ? *staffNs : prevStaffNs);
            if (place) dynam->SetPlace(*place);
            if (value) this->SetTextChildren(dynam, { *value });
            dynam->SetType(UserContentType);
            this->UpdateTimeSpanning(dynam);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemoveDynam(const std::string &dynamID, const std::optional<std::string> &measureID)
{
    Dynam *dynam = dynamic_cast<Dynam *>(this->FindElement(dynamID, measureID));
    if (dynam) {
        this->RemoveTimeSpanning(dynam);
        dynam->GetParent()->DeleteChild(dynam);
        return true;
    }
    return false;
}

bool EnoteToolkit::HasPedal(const std::string &pedalID, const std::optional<std::string> &measureID)
{
    return (dynamic_cast<Pedal *>(this->FindElement(pedalID, measureID)) != NULL);
}

bool EnoteToolkit::AddPedal(const std::optional<std::string> &pedalID, const std::string &measureID, double tstamp,
    const xsdPositiveInteger_List &staffNs, const std::optional<data_STAFFREL> &place, const std::optional<int> &vgrp,
    pedalLog_DIR dir)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        Pedal *pedal = new Pedal();
        if (pedalID) pedal->SetID(*pedalID);
        pedal->SetTstamp(tstamp);
        pedal->SetStaff(staffNs);
        if (place) pedal->SetPlace(*place);
        if (vgrp) pedal->SetVgrp(*vgrp);
        pedal->SetDir(dir);
        pedal->SetType(UserContentType);
        measure->AddChild(pedal);
        this->UpdateTimeSpanning(pedal);
        return true;
    }
    return false;
}

bool EnoteToolkit::AddPedal(const std::string &measureID, pugi::xml_node xmlPedal)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        MEIInput input(&m_doc);
        input.ReadPedal(measure, xmlPedal);
        Pedal *pedal = vrv_cast<Pedal *>(measure->GetLast(PEDAL));
        if (pedal) this->UpdateTimeSpanning(pedal);
        return true;
    }
    return false;
}

bool EnoteToolkit::EditPedal(const std::string &pedalID, const std::optional<std::string> &measureID,
    const std::optional<double> &tstamp, const std::optional<xsdPositiveInteger_List> &staffNs,
    const std::optional<data_STAFFREL> &place, const std::optional<int> &vgrp, const std::optional<pedalLog_DIR> &dir)
{
    Pedal *pedal = dynamic_cast<Pedal *>(m_doc.FindDescendantByID(pedalID));
    if (pedal) {
        const bool ok = measureID ? this->MoveToMeasure(pedal, *measureID) : true;
        if (ok) {
            const xsdPositiveInteger_List prevStaffNs = pedal->GetStaff();
            const double prevTstamp = pedal->GetTstamp();
            const data_MEASUREBEAT prevTstamp2 = pedal->GetTstamp2();
            pedal->TimeSpanningInterface::Reset();
            pedal->SetTstamp(tstamp ? *tstamp : prevTstamp);
            pedal->SetTstamp2(prevTstamp2);
            pedal->SetStaff(staffNs ? *staffNs : prevStaffNs);
            if (place) pedal->SetPlace(*place);
            if (vgrp) pedal->SetVgrp(*vgrp);
            if (dir) pedal->SetDir(*dir);
            pedal->SetType(UserContentType);
            this->UpdateTimeSpanning(pedal);
            return true;
        }
    }
    return false;
}

bool EnoteToolkit::RemovePedal(const std::string &pedalID, const std::optional<std::string> &measureID)
{
    Pedal *pedal = dynamic_cast<Pedal *>(this->FindElement(pedalID, measureID));
    if (pedal) {
        this->RemoveTimeSpanning(pedal);
        pedal->GetParent()->DeleteChild(pedal);
        return true;
    }
    return false;
}

bool EnoteToolkit::MoveToMeasure(ControlElement *element, const std::string &measureID)
{
    Measure *measure = this->FindMeasureByID(measureID);
    if (measure) {
        return this->MoveToMeasure(element, measure);
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
        text->SetText(UTF8to32(entry));
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
            const std::string startID = ExtractIDFragment(interface->GetStartid());
            LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
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
            const std::string startID = ExtractIDFragment(interface->GetStartid());
            LayerElement *startElement = dynamic_cast<LayerElement *>(measure->FindDescendantByID(startID));
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
            const std::string endID = ExtractIDFragment(interface->GetEndid());
            LayerElement *endElement = dynamic_cast<LayerElement *>(m_doc.FindDescendantByID(endID));
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
