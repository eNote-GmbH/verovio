/////////////////////////////////////////////////////////////////////////////
// Name:        chordmember.cpp
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "chordmember.h"

//----------------------------------------------------------------------------

#include "functor.h"

namespace vrv {

//----------------------------------------------------------------------------
// ChordMember
//----------------------------------------------------------------------------

static const ClassRegistrar<ChordMember> s_factory("chordMember", CHORDMEMBER);

ChordMember::ChordMember()
    : Object(CHORDMEMBER), AttAccidentalGes(), AttLabelled(), AttOctave(), AttPitch(), AttStringtab(), AttTyped()
{
    this->RegisterAttClass(ATT_ACCIDENTALGES);
    this->RegisterAttClass(ATT_LABELLED);
    this->RegisterAttClass(ATT_OCTAVE);
    this->RegisterAttClass(ATT_PITCH);
    this->RegisterAttClass(ATT_STRINGTAB);
    this->RegisterAttClass(ATT_TYPED);
    this->Reset();
}

ChordMember::~ChordMember() {}

void ChordMember::Reset()
{
    Object::Reset();
    this->ResetAccidentalGes();
    this->ResetLabelled();
    this->ResetOctave();
    this->ResetPitch();
    this->ResetStringtab();
    this->ResetTyped();
}

bool ChordMember::IsSupportedChild(ClassId classId)
{
    // Nothing for now
    return false;
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

FunctorCode ChordMember::Accept(Functor &functor)
{
    return functor.VisitChordMember(this);
}

FunctorCode ChordMember::Accept(ConstFunctor &functor) const
{
    return functor.VisitChordMember(this);
}

FunctorCode ChordMember::AcceptEnd(Functor &functor)
{
    return functor.VisitChordMemberEnd(this);
}

FunctorCode ChordMember::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitChordMemberEnd(this);
}

} // namespace vrv
