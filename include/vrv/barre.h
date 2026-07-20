/////////////////////////////////////////////////////////////////////////////
// Name:        barre.h
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_BARRE_H__
#define __VRV_BARRE_H__

#include "atts_shared.h"
#include "object.h"

namespace vrv {

//----------------------------------------------------------------------------
// Barre
//----------------------------------------------------------------------------

/**
 * This class models the MEI <barre> element.
 */
class Barre : public Object, public AttLabelled, public AttStartEndId, public AttStartId, public AttTyped {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    Barre();
    virtual ~Barre();
    Object *Clone() const override { return new Barre(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "barre"; }
    ///@}

    /** Add an element to a barre. */
    bool IsSupportedChild(ClassId classId) override;

    /** @name Setter, getter, and presence checker for the element-specific @fret. */
    ///@{
    void SetFret(int fret) { m_fret = fret; }
    int GetFret() const { return m_fret; }
    bool HasFret() const { return m_fret != MEI_UNSET; }
    ///@}

    //----------//
    // Functors //
    //----------//

    /** Interface for class functor visitation. */
    ///@{
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
    ///@}

protected:
    //
private:
    int m_fret;
};

} // namespace vrv

#endif
