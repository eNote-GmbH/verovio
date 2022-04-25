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

//----------------------------------------------------------------------------

#include <optional>

//----------------------------------------------------------------------------

namespace vrv {

//----------------------------------------------------------------------------
// MeasureRange
//----------------------------------------------------------------------------

struct MeasureRange {
    std::string mdivUuid;
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
    Measure *FindMeasureByUuid(const std::string &uuid);
    Measure *FindMeasureByN(const std::string &n);
    std::vector<Measure *> FindAllMeasures();
    std::vector<Page *> FindAllPages();
    Object *FindElement(const std::string &elementUuid, const std::optional<std::string> &measureUuid);
    Object *FindElementStartingInMeasure(const std::string &startUuid, const std::string &measureUuid);
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

    /****************************
     *  Editing layer elements  *
     ***************************/

    /**
     * Edit notes (restricted, only pitch changes for now)
     */
    ///@{
    bool HasNote(const std::string &noteUuid, const std::optional<std::string> &measureUuid);
    bool EditNote(
        const std::string &noteUuid, const std::string &measureUuid, data_PITCHNAME pitch, data_OCTAVE octave);
    ///@}

    /**
     * Edit note accidentals
     */
    ///@{
    bool HasNoteAccidental(const std::string &noteUuid, const std::optional<std::string> &measureUuid);
    bool AddNoteAccidental(const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN type);
    bool EditNoteAccidental(const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN type,
        bool resetAccidGes = true);
    bool RemoveNoteAccidental(const std::string &noteUuid, const std::optional<std::string> &measureUuid);
    ///@}

    /**
     * Edit note/chord articulations
     */
    ///@{
    bool HasArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
        const std::optional<std::string> &measureUuid);
    int GetArticulationCount(const std::string &noteOrChordUuid, const std::optional<std::string> &measureUuid);
    bool AddArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
        const std::string &measureUuid, data_ARTICULATION type);
    bool EditArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
        const std::string &measureUuid, data_ARTICULATION type, bool resetPlace = true);
    bool RemoveArticulation(const std::optional<std::string> &articUuid, const std::string &noteOrChordUuid,
        const std::optional<std::string> &measureUuid);
    ///@}

    /******************************
     *  Editing control elements  *
     *****************************/

    /**
     * Edit hairpins
     */
    ///@{
    bool HasHairpin(const std::string &hairpinUuid, const std::optional<std::string> &measureUuid);
    bool AddHairpin(const std::optional<std::string> &hairpinUuid, const std::string &measureUuid, double tstamp,
        data_MEASUREBEAT tstamp2, const xsdPositiveInteger_List &staffNs, data_STAFFREL place, hairpinLog_FORM form);
    bool EditHairpin(const std::string &hairpinUuid, const std::string &measureUuid,
        const std::optional<double> &tstamp, const std::optional<data_MEASUREBEAT> &tstamp2,
        const std::optional<xsdPositiveInteger_List> &staffNs, const std::optional<data_STAFFREL> &place,
        const std::optional<hairpinLog_FORM> &form);
    bool RemoveHairpin(const std::string &hairpinUuid, const std::optional<std::string> &measureUuid);
    ///@}

    /**
     * Edit slurs
     */
    ///@{
    bool HasSlur(const std::string &slurUuid, const std::optional<std::string> &measureUuid);
    bool AddSlur(const std::string &measureUuid, const std::string &startUuid, const std::string &endUuid,
        curvature_CURVEDIR curveDir);
    bool AddSlur(const std::string &measureUuid, const std::string &startUuid, data_MEASUREBEAT tstamp2,
        curvature_CURVEDIR curveDir);
    bool AddSlur(const std::string &slurUuid, const std::string &measureUuid, const std::string &startUuid,
        const std::string &endUuid, curvature_CURVEDIR curveDir);
    bool AddSlur(const std::string &slurUuid, const std::string &measureUuid, const std::string &startUuid,
        data_MEASUREBEAT tstamp2, curvature_CURVEDIR curveDir);
    bool EditSlur(const std::string &slurUuid, const std::string &measureUuid, const std::string &startUuid,
        const std::string &endUuid);
    bool EditSlur(const std::string &slurUuid, const std::string &measureUuid, const std::string &startUuid,
        data_MEASUREBEAT tstamp2);
    bool RemoveSlur(const std::string &slurUuid, const std::optional<std::string> &measureUuid);
    ///@}

    /**
     * Edit fingering
     */
    ///@{
    bool HasFing(const std::string &fingUuid, const std::optional<std::string> &measureUuid);
    bool HasFingOfNote(const std::optional<std::string> &fingUuid, const std::string &noteUuid,
        const std::optional<std::string> &measureUuid);
    bool AddFingToNote(
        const std::string &noteUuid, const std::string &measureUuid, data_STAFFREL place, const std::string &value);
    bool AddFingToNote(const std::string &fingUuid, const std::string &noteUuid, const std::string &measureUuid,
        data_STAFFREL place, const std::string &value);
    bool EditFingOfNote(
        const std::string &noteUuid, const std::string &measureUuid, data_STAFFREL place, const std::string &value);
    bool EditFingOfNote(const std::string &fingUuid, const std::string &noteUuid, const std::string &measureUuid,
        data_STAFFREL place, const std::string &value);
    bool RemoveFing(const std::string &fingUuid, const std::optional<std::string> &measureUuid);
    bool RemoveFingOfNote(const std::optional<std::string> &fingUuid, const std::string &noteUuid,
        const std::optional<std::string> &measureUuid);
    ///@}

    ///@}

private:
    /**
     * Manipulating the object tree
     */
    ///@{
    bool MoveToMeasure(ControlElement *element, const std::string &measureUuid);
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
