/////////////////////////////////////////////////////////////////////////////
// Name:        enotetoolkit.h
// Author:      David Bauer
// Created:     25/10/2021
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ENOTETOOLKIT_H__
#define __VRV_ENOTETOOLKIT_H__

//----------------------------------------------------------------------------

#include "toolkit.h"
#include "vrv.h"

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// MeasureRange
//----------------------------------------------------------------------------

struct MeasureRange {
    std::string mdivID;
    int firstN;
    int lastN;
};

//----------------------------------------------------------------------------
// EnoteToolkit
//----------------------------------------------------------------------------

class EnoteToolkit : public Toolkit {
public:
    /**
     *************************************************************************
     * @name Constructors and destructors
     *************************************************************************
     */
    ///@{
    /**
     * Constructor.
     *
     * @param initFont If set to false, resource path is not initialized and SetResourcePath will have to be called
     * explicitly
     */
    EnoteToolkit(bool initFont = true);
    virtual ~EnoteToolkit();

    ///@}

    /**
     *************************************************************************
     * Search methods
     *************************************************************************
     */
    ///@{

    /**
     * Find existing elements
     */
    ///@{
    Measure *FindMeasureByID(const std::string &id);
    Measure *FindMeasureByN(const std::string &n);
    std::vector<Measure *> FindAllMeasures();
    std::vector<Page *> FindAllPages();
    // All page indices are 1-based
    std::optional<int> FindPageIndex(Page *page);
    std::set<int> FindPageIndices(const std::string &id);
    std::set<int> FindPageIndices(const std::list<std::string> &ids);
    Object *FindElement(const std::string &elementID, const std::optional<std::string> &measureID);
    Object *FindElementStartingInMeasure(const std::string &startID, const std::string &measureID);
    ///@}

    /**
     * Extract measure ranges
     */
    /// @param index is 1-based
    std::list<MeasureRange> GetMeasureRangeForPage(int index);

    ///@}

    /**
     *************************************************************************
     * Editor methods
     *************************************************************************
     */
    ///@{

    /**
     * Check for changes
     */
    ///@{
    bool WasEdited(const std::string &elementID, const std::optional<std::string> &measureID);
    void SetEdited(const std::string &elementID, const std::optional<std::string> &measureID, bool wasEdited,
        bool includeDescendants = false);
    void SetEdited(Object *object, bool wasEdited, bool recursive = false);
    ///@}

    /****************************
     *  Editing layer elements  *
     ***************************/

    /**
     * Edit notes (restricted, only pitch changes for now)
     */
    ///@{
    bool HasNote(const std::string &noteID, const std::optional<std::string> &measureID);
    bool EditNote(const std::string &noteID, const std::optional<std::string> &measureID, data_PITCHNAME pitch,
        data_OCTAVE octave);
    bool EditNote(const std::string &noteID, const std::optional<std::string> &measureID, pugi::xml_node xmlNote);
    ///@}

    /**
     * Edit note accidentals
     */
    ///@{
    bool HasNoteAccidental(const std::string &noteID, const std::optional<std::string> &measureID);
    bool AddNoteAccidental(
        const std::string &noteID, const std::optional<std::string> &measureID, data_ACCIDENTAL_WRITTEN type);
    bool AddNoteAccidental(
        const std::string &noteID, const std::optional<std::string> &measureID, pugi::xml_node xmlNoteOrAccid);
    bool EditNoteAccidental(const std::string &noteID, const std::optional<std::string> &measureID,
        data_ACCIDENTAL_WRITTEN type, bool resetAccidGes = true);
    bool RemoveNoteAccidental(const std::string &noteID, const std::optional<std::string> &measureID);
    ///@}

    /**
     * Edit note/chord articulations
     */
    ///@{
    bool HasArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
        const std::optional<std::string> &measureID);
    int GetArticulationCount(const std::string &noteOrChordID, const std::optional<std::string> &measureID);
    bool AddArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
        const std::optional<std::string> &measureID, data_ARTICULATION type);
    bool AddArticulation(
        const std::string &noteOrChordID, const std::optional<std::string> &measureID, pugi::xml_node xmlNoteOrArtic);
    bool EditArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
        const std::optional<std::string> &measureID, data_ARTICULATION type, bool resetPlace = true);
    bool RemoveArticulation(const std::optional<std::string> &articID, const std::string &noteOrChordID,
        const std::optional<std::string> &measureID);
    ///@}

    /******************************
     *  Editing control elements  *
     *****************************/

    /**
     * Edit hairpins
     */
    ///@{
    bool HasHairpin(const std::string &hairpinID, const std::optional<std::string> &measureID);
    bool AddHairpin(const std::optional<std::string> &hairpinID, const std::string &measureID, double tstamp,
        data_MEASUREBEAT tstamp2, const xsdPositiveInteger_List &staffNs, data_STAFFREL place, hairpinLog_FORM form);
    bool AddHairpin(const std::string &measureID, pugi::xml_node xmlHairpin);
    bool EditHairpin(const std::string &hairpinID, const std::optional<std::string> &measureID,
        const std::optional<double> &tstamp, const std::optional<data_MEASUREBEAT> &tstamp2,
        const std::optional<xsdPositiveInteger_List> &staffNs, const std::optional<data_STAFFREL> &place,
        const std::optional<hairpinLog_FORM> &form);
    bool RemoveHairpin(const std::string &hairpinID, const std::optional<std::string> &measureID);
    ///@}

    /**
     * Edit slurs
     */
    ///@{
    bool HasSlur(const std::string &slurID, const std::optional<std::string> &measureID);
    bool AddSlur(const std::optional<std::string> &slurID, const std::string &measureID, const std::string &startID,
        const std::string &endID, const std::optional<curvature_CURVEDIR> &curveDir);
    bool AddSlur(const std::optional<std::string> &slurID, const std::string &measureID, const std::string &startID,
        data_MEASUREBEAT tstamp2, const std::optional<curvature_CURVEDIR> &curveDir);
    bool AddSlur(const std::string &measureID, pugi::xml_node xmlSlur);
    bool EditSlur(
        const std::string &slurID, const std::string &measureID, const std::string &startID, const std::string &endID);
    bool EditSlur(
        const std::string &slurID, const std::string &measureID, const std::string &startID, data_MEASUREBEAT tstamp2);
    bool EditSlur(const std::string &slurID, const std::optional<std::string> &measureID, curvature_CURVEDIR curveDir);
    bool RemoveSlur(const std::string &slurID, const std::optional<std::string> &measureID);
    ///@}

    /**
     * Edit fingering
     */
    ///@{
    bool HasFing(const std::string &fingID, const std::optional<std::string> &measureID);
    bool HasFingOfNote(const std::optional<std::string> &fingID, const std::string &noteID,
        const std::optional<std::string> &measureID);
    bool AddFingToNote(const std::optional<std::string> &fingID, const std::string &noteID,
        const std::optional<std::string> &measureID, const std::optional<data_STAFFREL> &place,
        const std::string &value);
    bool AddFing(const std::string &measureID, pugi::xml_node xmlFing);
    bool EditFingOfNote(const std::optional<std::string> &fingID, const std::string &noteID,
        const std::optional<std::string> &measureID, const std::optional<data_STAFFREL> &place,
        const std::optional<std::string> &value);
    bool RemoveFing(const std::string &fingID, const std::optional<std::string> &measureID);
    bool RemoveFingOfNote(const std::optional<std::string> &fingID, const std::string &noteID,
        const std::optional<std::string> &measureID);
    ///@}

    /**
     * Edit dynamics
     */
    ///@{
    bool HasDynam(const std::string &dynamID, const std::optional<std::string> &measureID);
    bool AddDynam(const std::optional<std::string> &dynamID, const std::string &measureID, double tstamp,
        const xsdPositiveInteger_List &staffNs, data_STAFFREL place, const std::string &value);
    bool AddDynam(const std::string &measureID, pugi::xml_node xmlDynam);
    bool EditDynam(const std::string &dynamID, const std::optional<std::string> &measureID,
        const std::optional<double> &tstamp, const std::optional<xsdPositiveInteger_List> &staffNs,
        const std::optional<data_STAFFREL> &place, const std::optional<std::string> &value);
    bool RemoveDynam(const std::string &dynamID, const std::optional<std::string> &measureID);
    ///@}

    /**
     * Edit pedals
     */
    ///@{
    bool HasPedal(const std::string &pedalID, const std::optional<std::string> &measureID);
    bool AddPedal(const std::optional<std::string> &pedalID, const std::string &measureID, double tstamp,
        const xsdPositiveInteger_List &staffNs, const std::optional<data_STAFFREL> &place,
        const std::optional<int> &vgrp, pedalLog_DIR dir);
    bool AddPedal(const std::string &measureID, pugi::xml_node xmlPedal);
    bool EditPedal(const std::string &pedalID, const std::optional<std::string> &measureID,
        const std::optional<double> &tstamp, const std::optional<xsdPositiveInteger_List> &staffNs,
        const std::optional<data_STAFFREL> &place, const std::optional<int> &vgrp,
        const std::optional<pedalLog_DIR> &dir);
    bool RemovePedal(const std::string &pedalID, const std::optional<std::string> &measureID);
    ///@}

private:
    /**
     * Manipulating the object tree
     */
    ///@{
    bool MoveToMeasure(ControlElement *element, const std::string &measureID);
    bool MoveToMeasure(ControlElement *element, Measure *measure);
    void SetTextChildren(ControlElement *element, const std::list<std::string> &textEntries);
    ///@}

    /**
     * Prepare rerendering
     */
    ///@{
    void UpdateTimePoint(ControlElement *element);
    void UpdateTimeSpanning(ControlElement *element);
    void RemoveTimeSpanning(ControlElement *element);
    ///@}

    // Find all measures in subtree
    std::vector<Measure *> FindAllMeasures(Object *parent);

    // Extract the first integer from a string
    int ExtractNumber(const std::string &text);

public:
    //
private:
    //
};

} // namespace vrv
#endif
