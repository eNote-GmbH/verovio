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
    Object *FindElementInMeasure(const std::string &elementUuid, const std::string &measureUuid);
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
    bool EditNote(
        const std::string &noteUuid, const std::string &measureUuid, data_PITCHNAME pitch, data_OCTAVE octave);
    ///@}

    /**
     * Edit note accidentals
     */
    ///@{
    bool HasNoteAccidental(const std::string &noteUuid, const std::string &measureUuid);
    bool AddNoteAccidental(const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN type);
    bool EditNoteAccidental(const std::string &noteUuid, const std::string &measureUuid, data_ACCIDENTAL_WRITTEN type);
    bool RemoveNoteAccidental(const std::string &noteUuid, const std::string &measureUuid);
    ///@}

    /**
     * Edit note/chord articulations
     */
    ///@{
    bool HasArticulation(const std::string &noteOrChordUuid, const std::string &measureUuid);
    data_ARTICULATION_List GetArticulation(const std::string &noteOrChordUuid, const std::string &measureUuid);
    bool AddArticulation(
        const std::string &noteOrChordUuid, const std::string &measureUuid, const data_ARTICULATION_List &type);
    bool AddArticulation(const std::string &articUuid, const std::string &noteOrChordUuid,
        const std::string &measureUuid, const data_ARTICULATION_List &type);
    bool EditArticulation(
        const std::string &noteOrChordUuid, const std::string &measureUuid, const data_ARTICULATION_List &type);
    bool EditArticulation(const std::string &articUuid, const std::string &noteOrChordUuid,
        const std::string &measureUuid, const data_ARTICULATION_List &type);
    bool RemoveArticulation(const std::string &noteOrChordUuid, const std::string &measureUuid);
    bool RemoveArticulation(
        const std::string &articUuid, const std::string &noteOrChordUuid, const std::string &measureUuid);
    ///@}

    /******************************
     *  Editing control elements  *
     *****************************/

    /**
     * Edit hairpins
     */
    ///@{
    bool AddHairpin(Measure *measure, const std::string &uuid, int staffN, double startTstamp,
        data_MEASUREBEAT endTstamp, hairpinLog_FORM form);
    bool ChangeHairpinForm(const std::string &uuid, hairpinLog_FORM form);
    bool ChangeHairpinLength(const std::string &uuid, double startTstamp, data_MEASUREBEAT endTstamp);
    ///@}

    /**
     * Edit ties
     */
    ///@{
    bool RemoveTie(const std::string &uuid);
    ///@}

    ///@}

private:
    /**
     * Prepare rerendering
     */
    ///@{
    void UpdateTimeSpanning(ControlElement *element);
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
