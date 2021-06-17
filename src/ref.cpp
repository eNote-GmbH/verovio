/////////////////////////////////////////////////////////////////////////////
// Name:        ref.cpp
// Author:      Laurent Pugin
// Created:     2018/02/21
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ref.h"

//----------------------------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------------------------

#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Ref
//----------------------------------------------------------------------------

// static ClassRegistrar<Ref> s_factory("ref", REF);

Ref::Ref() : EditorialElement("ref-")
{
    Reset();
}

Ref::~Ref() {}

void Ref::Reset()
{
    EditorialElement::Reset();
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

} // namespace vrv
