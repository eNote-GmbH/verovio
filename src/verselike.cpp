/////////////////////////////////////////////////////////////////////////////
// Name:        verselike.cpp
// Author:      Laurent Pugin and others
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "verselike.h"

//----------------------------------------------------------------------------

#include <array>
#include <cassert>

//----------------------------------------------------------------------------

#include "comparison.h"
#include "doc.h"
#include "editorial.h"
#include "syl.h"
#include "volta.h"

namespace vrv {

//----------------------------------------------------------------------------
// VerseLike
//----------------------------------------------------------------------------

VerseLike::VerseLike(ClassId classId)
    : LayerElement(classId)
    , OffsetInterface()
    , AttColor()
    , AttLang()
    , AttPlacementRelStaff()
    , AttTypography()
    , AttVoltaGroupingSym()
{
    this->RegisterInterface(OffsetInterface::GetAttClasses(), OffsetInterface::IsInterface());
    this->RegisterAttClass(ATT_COLOR);
    this->RegisterAttClass(ATT_LANG);
    this->RegisterAttClass(ATT_PLACEMENTRELSTAFF);
    this->RegisterAttClass(ATT_TYPOGRAPHY);
    this->RegisterAttClass(ATT_VOLTAGROUPINGSYM);
}

VerseLike::~VerseLike() {}

void VerseLike::Reset()
{
    LayerElement::Reset();
    OffsetInterface::Reset();
    this->ResetColor();
    this->ResetLang();
    this->ResetPlacementRelStaff();
    this->ResetTypography();
    this->ResetVoltaGroupingSym();
    this->ResetDrawingLyricGroup();
}

bool VerseLike::IsSupportedChild(ClassId classId)
{
    if ((classId == SYL) || (classId == VOLTA)) {
        return true;
    }
    return Object::IsEditorialElement(classId);
}

VerseLike *VerseLike::GetAncestorVerseLike(Object *object, int maxDepth)
{
    return const_cast<VerseLike *>(GetAncestorVerseLike(static_cast<const Object *>(object), maxDepth));
}

const VerseLike *VerseLike::GetAncestorVerseLike(const Object *object, int maxDepth)
{
    const Object *parent = object ? object->GetParent() : NULL;
    while (parent && (maxDepth != 0)) {
        if (parent->IsAnyOf(std::array{ REFRAIN, VERSE })) {
            const VerseLike *verseLike = dynamic_cast<const VerseLike *>(parent);
            assert(verseLike);
            return verseLike;
        }
        parent = parent->GetParent();
        if (maxDepth > 0) --maxDepth;
    }
    return NULL;
}

int VerseLike::GetVoltaCount() const
{
    return (int)this->FindAllDescendantsByType(VOLTA).size();
}

std::pair<int, int> VerseLike::GetVoltaDrawingRange() const
{
    int first = 0;
    int last = 0;
    for (const Object *object : this->FindAllDescendantsByType(VOLTA)) {
        const Volta *volta = vrv_cast<const Volta *>(object);
        assert(volta);
        const int drawingN = volta->GetDrawingVoltaN();
        first = (first == 0) ? drawingN : std::min(first, drawingN);
        last = std::max(last, drawingN);
    }
    return { first, last };
}

bool VerseLike::HasDirectSyl() const
{
    const ListOfConstObjects syls = this->FindAllDescendantsByType(SYL);
    return std::any_of(
        syls.begin(), syls.end(), [](const Object *syl) { return (syl->GetFirstAncestor(VOLTA) == NULL); });
}

int VerseLike::GetLyricLineCount() const
{
    const int lastVoltaTrack = this->GetVoltaDrawingRange().second;
    return std::max(1, lastVoltaTrack + ((lastVoltaTrack && this->HasDrawingDirectSylTrack()) ? 1 : 0));
}

int VerseLike::GetVoltaLineN(const Volta *volta) const
{
    assert(volta);
    return volta->GetDrawingVoltaN() + (this->HasDrawingDirectSylTrack() ? 1 : 0);
}

int VerseLike::AdjustPosition(int &overlap, int freeSpace, const Doc *doc)
{
    assert(doc);

    int nextFreeSpace = 0;
    if (overlap > 0) {
        if (freeSpace > overlap) {
            this->SetDrawingXRel(this->GetDrawingXRel() - overlap);
            overlap = 0;
        }
        else if (freeSpace > 0) {
            this->SetDrawingXRel(this->GetDrawingXRel() - freeSpace);
            overlap -= freeSpace;
        }
    }
    else {
        nextFreeSpace = std::min(-overlap, 3 * doc->GetDrawingUnit(100));
    }
    return nextFreeSpace;
}

bool VerseLikeComparison::operator()(const Object *object)
{
    if (!object->IsAnyOf(std::array{ REFRAIN, VERSE })) return false;
    const VerseLike *verseLike = dynamic_cast<const VerseLike *>(object);
    assert(verseLike);
    return (verseLike->GetDrawingLyricGroupN() == m_groupN);
}

} // namespace vrv
