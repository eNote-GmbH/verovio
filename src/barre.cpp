/////////////////////////////////////////////////////////////////////////////
// Name:        barre.cpp
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "barre.h"

//----------------------------------------------------------------------------

#include "functor.h"

namespace vrv {

//----------------------------------------------------------------------------
// Barre
//----------------------------------------------------------------------------

static const ClassRegistrar<Barre> s_factory("barre", BARRE);

Barre::Barre() : Object(BARRE), AttLabelled(), AttStartEndId(), AttStartId(), AttTyped()
{
    this->RegisterAttClass(ATT_LABELLED);
    this->RegisterAttClass(ATT_STARTENDID);
    this->RegisterAttClass(ATT_STARTID);
    this->RegisterAttClass(ATT_TYPED);
    this->Reset();
}

Barre::~Barre() {}

void Barre::Reset()
{
    Object::Reset();
    this->ResetLabelled();
    this->ResetStartEndId();
    this->ResetStartId();
    this->ResetTyped();
    m_fret = MEI_UNSET;
}

bool Barre::IsSupportedChild(ClassId classId)
{
    // Nothing for now
    return false;
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

FunctorCode Barre::Accept(Functor &functor)
{
    return functor.VisitBarre(this);
}

FunctorCode Barre::Accept(ConstFunctor &functor) const
{
    return functor.VisitBarre(this);
}

FunctorCode Barre::AcceptEnd(Functor &functor)
{
    return functor.VisitBarreEnd(this);
}

FunctorCode Barre::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitBarreEnd(this);
}

} // namespace vrv
