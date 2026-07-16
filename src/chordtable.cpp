/////////////////////////////////////////////////////////////////////////////
// Name:        chordtable.cpp
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "chordtable.h"

//----------------------------------------------------------------------------

#include "functor.h"

namespace vrv {

//----------------------------------------------------------------------------
// ChordTable
//----------------------------------------------------------------------------

static const ClassRegistrar<ChordTable> s_factory("chordTable", CHORDTABLE);

ChordTable::ChordTable() : Object(CHORDTABLE), AttLabelled(), AttTyped()
{
    this->RegisterAttClass(ATT_LABELLED);
    this->RegisterAttClass(ATT_TYPED);
    this->Reset();
}

ChordTable::~ChordTable() {}

void ChordTable::Reset()
{
    Object::Reset();
    this->ResetLabelled();
    this->ResetTyped();
}

bool ChordTable::IsSupportedChild(ClassId classId)
{
    return classId == CHORDDEF;
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

FunctorCode ChordTable::Accept(Functor &functor)
{
    return functor.VisitChordTable(this);
}

FunctorCode ChordTable::Accept(ConstFunctor &functor) const
{
    return functor.VisitChordTable(this);
}

FunctorCode ChordTable::AcceptEnd(Functor &functor)
{
    return functor.VisitChordTableEnd(this);
}

FunctorCode ChordTable::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitChordTableEnd(this);
}

} // namespace vrv
