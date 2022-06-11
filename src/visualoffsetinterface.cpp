/////////////////////////////////////////////////////////////////////////////
// Name:        visualoffsetinterface.cpp
// Author:      Andriy Makarchuk
// Created:     2022
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "visualoffsetinterface.h"

//----------------------------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------------------------

#include "vrv.h"

namespace vrv {

VisualOffsetInterface::VisualOffsetInterface()
    : Interface(), AttVisualOffsetHo(), AttVisualOffsetVo(), AttVisualOffset2Ho(), AttVisualOffset2Vo()
{
    this->RegisterInterfaceAttClass(ATT_VISUALOFFSETHO);
    this->RegisterInterfaceAttClass(ATT_VISUALOFFSETVO);
    this->RegisterInterfaceAttClass(ATT_VISUALOFFSET2HO);
    this->RegisterInterfaceAttClass(ATT_VISUALOFFSET2VO);
    this->Reset();
}

VisualOffsetInterface::~VisualOffsetInterface() {}

void VisualOffsetInterface::Reset()
{
    this->ResetVisualOffsetHo();
    this->ResetVisualOffsetVo();
    this->ResetVisualOffset2Ho();
    this->ResetVisualOffset2Vo();
}

} // namespace vrv