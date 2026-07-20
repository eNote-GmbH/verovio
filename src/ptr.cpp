/////////////////////////////////////////////////////////////////////////////
// Name:        ptr.cpp
// Author:      Verovio contributors
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ptr.h"

#include "vrv.h"

namespace vrv {

static const ClassRegistrar<Ptr> s_factory("ptr", PTR);

Ptr::Ptr() : TextElement(PTR), AttPointing()
{
    this->RegisterAttClass(ATT_POINTING);
    this->Reset();
}

void Ptr::Reset()
{
    TextElement::Reset();
    this->ResetPointing();
}

bool Ptr::IsSupportedChild(ClassId)
{
    return false;
}

Object *Ptr::GetTargetObject(Object *root) const
{
    if (!root || !this->HasTarget()) return nullptr;
    const std::string target = this->GetTarget();
    if ((target.size() < 2) || (target.front() != '#')) return nullptr;
    return root->FindDescendantByID(target.substr(1));
}

const Object *Ptr::GetTargetObject(const Object *root) const
{
    if (!root || !this->HasTarget()) return nullptr;
    const std::string target = this->GetTarget();
    if ((target.size() < 2) || (target.front() != '#')) return nullptr;
    return root->FindDescendantByID(target.substr(1));
}

} // namespace vrv
