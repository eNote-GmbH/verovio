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

void VisualOffsetInterface::AlterPosition(std::vector<std::pair<int *, int *>> &points, ClassId classId, int unit) const
{
    // visual offset values
    const int xAdjust = this->HasHo() ? this->GetHo().GetVu() * unit : 0;
    const int yAdjust = this->HasVo() ? this->GetVo().GetVu() * unit : 0;
    // visual offset2 values
    const int xStartAdjust = this->HasStartho() ? this->GetStartho().GetVu() * unit : 0;
    const int xEndAdjust = this->HasEndho() ? this->GetEndho().GetVu() * unit : 0;
    const int yStartAdjust = this->HasStartvo() ? this->GetStartvo().GetVu() * unit : 0;
    const int yEndAdjust = this->HasEndvo() ? this->GetEndvo().GetVu() * unit : 0;

    const int size = static_cast<int>(points.size());
    // If there are no elements to process - return
    if (!size) return;

    // Apply offset values first
    if (xAdjust || yAdjust) {
        for (std::pair<int *, int *> &point : points) {
            *point.first += xAdjust;
            *point.second += yAdjust;
        }
    }

    // If there is only one point - return
    if ((size < 2) && (classId != OCTAVE)) return;
    // If none of the start/end values were set - return
    if (!xStartAdjust && !yStartAdjust && !xEndAdjust && !yEndAdjust) return;

    // Special treatment for Hairpins
    if (classId == HAIRPIN) {
        // Labmda to adjust two points on the hairpin
        const auto adjustHairpin = [&](int firstIndex, int secondIndex) {
            // adjust start
            *points.at(firstIndex).first += xStartAdjust;
            *points.at(firstIndex).second += yStartAdjust;
            // adjust end
            *points.at(secondIndex).first += xEndAdjust;
            *points.at(secondIndex).second += yEndAdjust;
        };
        if (size == 2) {
            adjustHairpin(0, 1);
        }
        else if (size == 3) {
            // check direction of hairpin first
            if (*points.at(0).first < *points.at(1).first) {
                adjustHairpin(0, 1);
                // adjust third point
                *points.at(2).first += xStartAdjust;
                *points.at(2).second += yStartAdjust;
            }
            else {
                adjustHairpin(1, 2);
                // adjust first point
                *points.at(0).first += xEndAdjust;
                *points.at(0).second += yEndAdjust;
            }
        }
    }
    // For other elements apply @start values to first half of the points an @end - to the second
    else {
        for (int i = 0; i < size / 2; ++i) {
            *points.at(i).first += xStartAdjust;
            *points.at(i).second += yStartAdjust;
        }
        for (int i = size / 2; i < size; ++i) {
            *points.at(i).first += xEndAdjust;
            *points.at(i).second += yEndAdjust;
        }
    }
}

} // namespace vrv
