/////////////////////////////////////////////////////////////////////////////
// Name:        refrain.h
// Author:      Laurent Pugin and others
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_REFRAIN_H__
#define __VRV_REFRAIN_H__

#include "verselike.h"

namespace vrv {

//----------------------------------------------------------------------------
// Refrain
//----------------------------------------------------------------------------

/** Recurring note-attached lyrics, such as a chorus. */
class Refrain : public VerseLike, public AttNNumberLike {
public:
    Refrain();
    virtual ~Refrain();
    Object *Clone() const override { return new Refrain(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "refrain"; }

    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

} // namespace vrv

#endif
