/////////////////////////////////////////////////////////////////////////////
// Name:        chordtable.h
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_CHORDTABLE_H__
#define __VRV_CHORDTABLE_H__

#include "atts_shared.h"
#include "object.h"

namespace vrv {

//----------------------------------------------------------------------------
// ChordTable
//----------------------------------------------------------------------------

/**
 * This class models the MEI <chordTable> element.
 */
class ChordTable : public Object, public AttLabelled, public AttTyped {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    ChordTable();
    virtual ~ChordTable();
    /** Keep score-level chord metadata out of derived system score definitions. */
    Object *Clone() const override { return NULL; }
    void Reset() override;
    std::string GetClassName() const override { return "chordTable"; }
    ///@}

    /** Add an element to a chordTable. */
    bool IsSupportedChild(ClassId classId) override;

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
    //
};

} // namespace vrv

#endif
