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
     * Editor methods
     *************************************************************************
     */
    ///@{
    /**
     * Find existing elements
     */
    Measure *FindMeasureByUuid(const std::string &uuid);
    Measure *FindMeasureByN(const std::string &n);

    /**
     * Edit hairpins
     */
    bool AddHairpin(Measure *measure, const std::string &uuid, int staffN, double startTstamp,
        data_MEASUREBEAT endTstamp, hairpinLog_FORM form);
    bool UpdateHairpin(const std::string &uuid, double startTstamp, data_MEASUREBEAT endTstamp, hairpinLog_FORM form);

    /**
     * Edit notes
     */
    bool UpdateNote(const std::string &uuid, data_PITCHNAME pitch, data_OCTAVE octave);

    /**
     * Edit ties
     */
    bool RemoveTie(const std::string &uuid);
    ///@}

private:
    //
public:
    //
private:
    //
};

} // namespace vrv
#endif
