/////////////////////////////////////////////////////////////////////////////
// Name:        chorddef.cpp
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "chorddef.h"

//----------------------------------------------------------------------------

#include "functor.h"

namespace vrv {

//----------------------------------------------------------------------------
// ChordDef
//----------------------------------------------------------------------------

static const ClassRegistrar<ChordDef> s_factory("chordDef", CHORDDEF);

ChordDef::ChordDef() : Object(CHORDDEF), AttLabelled(), AttStringtabPosition(), AttStringtabTuning(), AttTyped()
{
    this->RegisterAttClass(ATT_LABELLED);
    this->RegisterAttClass(ATT_STRINGTABPOSITION);
    this->RegisterAttClass(ATT_STRINGTABTUNING);
    this->RegisterAttClass(ATT_TYPED);
    this->Reset();
}

ChordDef::~ChordDef() {}

void ChordDef::Reset()
{
    Object::Reset();
    this->ResetLabelled();
    this->ResetStringtabPosition();
    this->ResetStringtabTuning();
    this->ResetTyped();
}

bool ChordDef::IsSupportedChild(ClassId classId)
{
    static const std::vector<ClassId> supported{ CHORDMEMBER, BARRE };
    return std::find(supported.begin(), supported.end(), classId) != supported.end();
}

int ChordDef::GetInsertOrderFor(ClassId classId) const
{
    static const std::vector<ClassId> order{ CHORDMEMBER, BARRE };
    return this->GetInsertOrderForIn(classId, order);
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

FunctorCode ChordDef::Accept(Functor &functor)
{
    return functor.VisitChordDef(this);
}

FunctorCode ChordDef::Accept(ConstFunctor &functor) const
{
    return functor.VisitChordDef(this);
}

FunctorCode ChordDef::AcceptEnd(Functor &functor)
{
    return functor.VisitChordDefEnd(this);
}

FunctorCode ChordDef::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitChordDefEnd(this);
}

} // namespace vrv
