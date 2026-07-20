/////////////////////////////////////////////////////////////////////////////
// Name:        chordmember.h
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_CHORDMEMBER_H__
#define __VRV_CHORDMEMBER_H__

#include "atts_gestural.h"
#include "atts_shared.h"
#include "atts_stringtab.h"
#include "object.h"

namespace vrv {

//----------------------------------------------------------------------------
// ChordMember
//----------------------------------------------------------------------------

/**
 * This class models the MEI <chordMember> element.
 */
class ChordMember : public Object,
                    public AttAccidentalGes,
                    public AttLabelled,
                    public AttOctave,
                    public AttPitch,
                    public AttStringtab,
                    public AttTyped {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    ChordMember();
    virtual ~ChordMember();
    Object *Clone() const override { return new ChordMember(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "chordMember"; }
    ///@}

    /** Add an element to a chordMember. */
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
