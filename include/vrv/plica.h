/////////////////////////////////////////////////////////////////////////////
// Name:        plica.h
// Author:      Laurent Pugin
// Created:     2020
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_PLICA_H__
#define __VRV_PLICA_H__

#include "atts_visual.h"
#include "layerelement.h"

namespace vrv {

//----------------------------------------------------------------------------
// Plica
//----------------------------------------------------------------------------

class Plica : public LayerElement, public AttPlicaVis {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    Plica();
    virtual ~Plica();
    Object *Clone() const override { return new Plica(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "Plica"; }

    //----------//
    // Functors //
    //----------//

private:
    //
public:
    //
private:
};

} // namespace vrv

#endif
