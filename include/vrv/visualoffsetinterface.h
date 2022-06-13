/////////////////////////////////////////////////////////////////////////////
// Name:        visualoffsetinterface.h
// Author:      Andriy Makarchuk
// Created:     2022
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_VISUAL_OFFSET_INTERFACE_H__
#define __VRV_VISUAL_OFFSET_INTERFACE_H__

#include "atts_shared.h"
#include "vrvdef.h"

namespace vrv {

//----------------------------------------------------------------------------
// VisualOffsetInterface
//----------------------------------------------------------------------------

/**
 * This class is an interface for elements having a link
 * It is not an abstract class but should not be instantiated directly.
 */
class VisualOffsetInterface : public Interface,
                              public AttVisualOffsetHo,
                              public AttVisualOffsetVo,
                              public AttVisualOffset2Ho,
                              public AttVisualOffset2Vo {
public:
    /**
     * @name Constructors, destructors, reset methods
     * Reset method reset all attribute classes
     */
    ///@{
    VisualOffsetInterface();
    virtual ~VisualOffsetInterface();
    void Reset() override;
    InterfaceId IsInterface() const override { return INTERFACE_VISUAL_OFFSET; }
    ///@}

    bool HasOffsetValues() const { return this->HasHo() || this->HasVo(); }
};
} // namespace vrv

#endif //__VRV_VISUAL_OFFSET_INTERFACE_H__