/////////////////////////////////////////////////////////////////////////////
// Name:        ptr.h
// Author:      Verovio contributors
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_PTR_H__
#define __VRV_PTR_H__

#include "atts_shared.h"
#include "textelement.h"

namespace vrv {

/** This class models the MEI <ptr> element. */
class Ptr : public TextElement, public AttPointing {
public:
    Ptr();
    ~Ptr() override = default;

    Object *Clone() const override { return new Ptr(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "ptr"; }
    bool IsSupportedChild(ClassId classId) override;

    /** Resolve a local @target URI against the supplied document tree. */
    Object *GetTargetObject(Object *root) const;
    const Object *GetTargetObject(const Object *root) const;
};

} // namespace vrv

#endif
