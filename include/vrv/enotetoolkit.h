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

private:
    //
public:
    //
private:
    //
};

} // namespace vrv
#endif
