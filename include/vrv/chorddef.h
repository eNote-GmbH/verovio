/////////////////////////////////////////////////////////////////////////////
// Name:        chorddef.h
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_CHORDDEF_H__
#define __VRV_CHORDDEF_H__

#include "atts_shared.h"
#include "atts_stringtab.h"
#include "object.h"

namespace vrv {

//----------------------------------------------------------------------------
// ChordDef
//----------------------------------------------------------------------------

/**
 * This class models the MEI <chordDef> element.
 */
class ChordDef : public Object,
                 public AttLabelled,
                 public AttStringtabPosition,
                 public AttStringtabTuning,
                 public AttTyped {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    ChordDef();
    virtual ~ChordDef();
    Object *Clone() const override { return new ChordDef(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "chordDef"; }
    ///@}

    /** @name Methods for adding allowed content. */
    ///@{
    bool IsSupportedChild(ClassId classId) override;
    int GetInsertOrderFor(ClassId classId) const override;
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
    //
};

} // namespace vrv

#endif
