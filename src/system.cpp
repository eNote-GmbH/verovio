/////////////////////////////////////////////////////////////////////////////
// Name:        system.cpp
// Author:      Laurent Pugin
// Created:     2011
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "system.h"

//----------------------------------------------------------------------------

#include <cassert>

//----------------------------------------------------------------------------

#include "beamspan.h"
#include "comparison.h"
#include "dir.h"
#include "doc.h"
#include "dynam.h"
#include "ending.h"
#include "findfunctor.h"
#include "findlayerelementsfunctor.h"
#include "functorparams.h"
#include "layer.h"
#include "measure.h"
#include "page.h"
#include "pages.h"
#include "pedal.h"
#include "section.h"
#include "slur.h"
#include "staff.h"
#include "syl.h"
#include "systemmilestone.h"
#include "tempo.h"
#include "trill.h"
#include "verse.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// System
//----------------------------------------------------------------------------

System::System() : Object(SYSTEM, "system-"), DrawingListInterface(), AttTyped()
{
    this->RegisterAttClass(ATT_TYPED);

    // We set parent to it because we want to access the parent doc from the aligners
    m_systemAligner.SetParent(this);

    // owned pointers need to be set to NULL;
    m_drawingScoreDef = NULL;

    this->Reset();
}

System::~System()
{
    // We need to delete own objects
    this->Reset();
}

void System::Reset()
{
    Object::Reset();
    DrawingListInterface::Reset();
    this->ResetTyped();

    this->ResetDrawingScoreDef();

    m_systemLeftMar = 0;
    m_systemRightMar = 0;
    m_xAbs = VRV_UNSET;
    m_drawingXRel = 0;
    m_yAbs = VRV_UNSET;
    m_drawingYRel = 0;
    m_drawingTotalWidth = 0;
    m_drawingJustifiableWidth = 0;
    m_castOffTotalWidth = 0;
    m_castOffJustifiableWidth = 0;
    m_drawingAbbrLabelsWidth = 0;
    m_drawingIsOptimized = false;
}

bool System::IsSupportedChild(Object *child)
{
    if (child->Is(MEASURE)) {
        assert(dynamic_cast<Measure *>(child));
    }
    else if (child->Is(SCOREDEF)) {
        assert(dynamic_cast<ScoreDef *>(child));
    }
    else if (child->IsSystemElement()) {
        assert(dynamic_cast<SystemElement *>(child));
    }
    else if (child->IsEditorialElement()) {
        assert(dynamic_cast<EditorialElement *>(child));
    }
    else {
        return false;
    }
    return true;
}

int System::GetDrawingX() const
{
    if (m_xAbs != VRV_UNSET) return m_xAbs;

    m_cachedDrawingX = 0;
    return m_drawingXRel;
}

int System::GetDrawingY() const
{
    if (m_yAbs != VRV_UNSET) return m_yAbs;

    m_cachedDrawingY = 0;
    return m_drawingYRel;
}

void System::SetDrawingXRel(int drawingXRel)
{
    this->ResetCachedDrawingX();
    m_drawingXRel = drawingXRel;
}

void System::SetDrawingYRel(int drawingYRel)
{
    this->ResetCachedDrawingY();
    m_drawingYRel = drawingYRel;
}

int System::GetHeight() const
{
    if (m_systemAligner.GetBottomAlignment()) {
        return -m_systemAligner.GetBottomAlignment()->GetYRel();
    }
    return 0;
}

int System::GetMinimumSystemSpacing(const Doc *doc) const
{
    const auto &spacingSystem = doc->GetOptions()->m_spacingSystem;
    if (!spacingSystem.IsSet()) {
        assert(m_drawingScoreDef);
        if (m_drawingScoreDef->HasSpacingSystem()) {
            if (m_drawingScoreDef->GetSpacingSystem().GetType() == MEASUREMENTTYPE_px) {
                return m_drawingScoreDef->GetSpacingSystem().GetPx();
            }
            else {
                return m_drawingScoreDef->GetSpacingSystem().GetVu() * doc->GetDrawingUnit(100);
            }
        }
    }

    return spacingSystem.GetValue() * doc->GetDrawingUnit(100);
}

int System::GetDrawingLabelsWidth() const
{
    return (m_drawingScoreDef) ? m_drawingScoreDef->GetDrawingLabelsWidth() : 0;
}

void System::SetDrawingLabelsWidth(int width)
{
    assert(m_drawingScoreDef);
    m_drawingScoreDef->SetDrawingLabelsWidth(width);
}

void System::SetDrawingAbbrLabelsWidth(int width)
{
    if (m_drawingAbbrLabelsWidth < width) {
        m_drawingAbbrLabelsWidth = width;
    }
}

bool System::SetCurrentFloatingPositioner(
    int staffN, FloatingObject *object, Object *objectX, Object *objectY, char spanningType)
{
    assert(object);

    // If we have only the bottom alignment, then nothing to do (yet)
    if (m_systemAligner.GetChildCount() == 1) return false;
    StaffAlignment *alignment = m_systemAligner.GetStaffAlignmentForStaffN(staffN);
    if (!alignment) {
        LogError("Staff @n='%d' for rendering control event %s %s not found", staffN, object->GetClassName().c_str(),
            object->GetID().c_str());
        return false;
    }
    alignment->SetCurrentFloatingPositioner(object, objectX, objectY, spanningType);
    return true;
}

void System::SetDrawingScoreDef(ScoreDef *drawingScoreDef)
{
    assert(!m_drawingScoreDef); // We should always call ResetDrawingScoreDef before

    m_drawingScoreDef = new ScoreDef();
    *m_drawingScoreDef = *drawingScoreDef;
    m_drawingScoreDef->SetParent(this);
}

void System::ResetDrawingScoreDef()
{
    if (m_drawingScoreDef) {
        delete m_drawingScoreDef;
        m_drawingScoreDef = NULL;
    }
}

bool System::HasMixedDrawingStemDir(const LayerElement *start, const LayerElement *end) const
{
    assert(start);
    assert(end);

    // It is too inefficient to look for chord and notes over the entire system
    // We need first to get a list of measures
    const Object *measureStart = start->GetFirstAncestor(MEASURE);
    assert(measureStart);
    const Object *measureEnd = end->GetFirstAncestor(MEASURE);
    assert(measureEnd);
    ListOfConstObjects measures;

    // start and end are in the same measure, this is the only one we need
    if (measureStart == measureEnd) {
        measures.push_back(measureStart);
    }
    // otherwise look for a measures in between
    else {
        ClassIdComparison isMeasure(MEASURE);
        FindAllBetweenFunctor findAllBetween(&isMeasure, &measures, measureStart, measureEnd);
        this->Process(findAllBetween, 1);
    }

    // Now we can look for chords and note
    ClassIdsComparison matchType({ CHORD, NOTE });
    ListOfConstObjects children;
    for (const Object *measure : measures) {
        const Object *curStart = (measure == measureStart) ? start : measure->GetFirst();
        const Object *curEnd = (measure == measureEnd) ? end : measure->GetLast();
        measure->FindAllDescendantsBetween(&children, &matchType, curStart, curEnd, false);
    }

    const Layer *layerStart = vrv_cast<const Layer *>(start->GetFirstAncestor(LAYER));
    assert(layerStart);
    const Staff *staffStart = vrv_cast<const Staff *>(layerStart->GetFirstAncestor(STAFF));
    assert(staffStart);

    data_STEMDIRECTION stemDir = STEMDIRECTION_NONE;

    for (const Object *child : children) {
        const Layer *layer = vrv_cast<const Layer *>(child->GetFirstAncestor(LAYER));
        assert(layer);
        const Staff *staff = vrv_cast<const Staff *>(child->GetFirstAncestor(STAFF));
        assert(staff);

        // If the slur is spanning over several measures, the children list will include notes and chords
        // from other staves and layers, so we need to skip them.
        // Alternatively we could process by staff / layer, but the current solution might be better
        // if we want to look for slurs starting / ending on different staff / layer
        if ((staff->GetN() != staffStart->GetN()) || (layer->GetN() != layerStart->GetN())) {
            continue;
        }

        const StemmedDrawingInterface *interface = child->GetStemmedDrawingInterface();
        assert(interface);

        // First pass
        if (stemDir == STEMDIRECTION_NONE) {
            stemDir = interface->GetDrawingStemDir();
        }
        else if (stemDir != interface->GetDrawingStemDir()) {
            return true;
        }
    }

    return false;
}

curvature_CURVEDIR System::GetPreferredCurveDirection(
    const LayerElement *start, const LayerElement *end, const Slur *slur) const
{
    FindSpannedLayerElementsFunctor findSpannedLayerElements(slur);
    findSpannedLayerElements.SetMinMaxPos(start->GetDrawingX(), end->GetDrawingX());
    findSpannedLayerElements.SetClassIds({ CHORD, NOTE });

    const Layer *layerStart = vrv_cast<const Layer *>(start->GetFirstAncestor(LAYER));
    assert(layerStart);

    this->Process(findSpannedLayerElements);

    curvature_CURVEDIR preferredDirection = curvature_CURVEDIR_NONE;
    for (auto element : findSpannedLayerElements.GetElements()) {
        const Layer *layer = vrv_cast<const Layer *>((element)->GetFirstAncestor(LAYER));
        assert(layer);
        if (layer == layerStart) continue;

        if (curvature_CURVEDIR_NONE == preferredDirection) {
            if (layer->GetN() > layerStart->GetN()) {
                preferredDirection = curvature_CURVEDIR_above;
            }
            else {
                preferredDirection = curvature_CURVEDIR_below;
            }
        }
        // if there are layers both above and below - discard previous location and return - we'll use default direction
        else if (((curvature_CURVEDIR_above == preferredDirection) && (layer->GetN() < layerStart->GetN()))
            || ((curvature_CURVEDIR_below == preferredDirection) && (layer->GetN() > layerStart->GetN()))) {
            preferredDirection = curvature_CURVEDIR_NONE;
            break;
        }
    }

    return preferredDirection;
}

void System::AddToDrawingListIfNecessary(Object *object)
{
    assert(object);

    if (!object->HasInterface(INTERFACE_TIME_SPANNING)) return;

    if (object->Is(
            { BEAMSPAN, BRACKETSPAN, FIGURE, GLISS, HAIRPIN, LV, OCTAVE, PHRASE, PITCHINFLECTION, SLUR, SYL, TIE })) {
        this->AddToDrawingList(object);
    }
    else if (object->Is(DIR)) {
        Dir *dir = vrv_cast<Dir *>(object);
        assert(dir);
        if (dir->GetEnd() || (dir->GetNextLink() && (dir->GetExtender() == BOOLEAN_true))) {
            this->AddToDrawingList(dir);
        }
    }
    else if (object->Is(DYNAM)) {
        Dynam *dynam = vrv_cast<Dynam *>(object);
        assert(dynam);
        if (dynam->GetEnd() || (dynam->GetNextLink() && (dynam->GetExtender() == BOOLEAN_true))) {
            this->AddToDrawingList(dynam);
        }
    }
    else if (object->Is(PEDAL)) {
        Pedal *pedal = vrv_cast<Pedal *>(object);
        assert(pedal);
        if (pedal->GetEnd()) {
            this->AddToDrawingList(pedal);
        }
    }
    else if (object->Is(TEMPO)) {
        Tempo *tempo = vrv_cast<Tempo *>(object);
        assert(tempo);
        if (tempo->GetEnd() && (tempo->GetExtender() == BOOLEAN_true)) {
            this->AddToDrawingList(tempo);
        }
    }
    else if (object->Is(TRILL)) {
        Trill *trill = vrv_cast<Trill *>(object);
        assert(trill);
        if (trill->GetEnd() && (trill->GetExtender() != BOOLEAN_false)) {
            this->AddToDrawingList(trill);
        }
    }
}

bool System::IsFirstInPage() const
{
    assert(this->GetParent());
    return (this->GetParent()->GetFirst(SYSTEM) == this);
}

bool System::IsLastInPage() const
{
    assert(this->GetParent());
    return (this->GetParent()->GetLast(SYSTEM) == this);
}

bool System::IsFirstOfMdiv() const
{
    assert(this->GetParent());
    const Object *nextSibling = this->GetParent()->GetPrevious(this);
    return (nextSibling && nextSibling->IsPageElement());
}

bool System::IsLastOfMdiv() const
{
    assert(this->GetParent());
    const Object *nextSibling = this->GetParent()->GetNext(this);
    return (nextSibling && nextSibling->IsPageElement());
}

bool System::IsFirstOfSelection() const
{
    const Page *page = vrv_cast<const Page *>(this->GetFirstAncestor(PAGE));
    assert(page);
    return (page->IsFirstOfSelection() && this->IsFirstInPage());
}

bool System::IsLastOfSelection() const
{
    const Page *page = vrv_cast<const Page *>(this->GetFirstAncestor(PAGE));
    assert(page);
    return (page->IsLastOfSelection() && this->IsLastInPage());
}

double System::EstimateJustificationRatio(const Doc *doc) const
{
    assert(doc);

    // We can only estimate if cast off system widths are available
    if ((m_castOffTotalWidth == 0) || (m_castOffJustifiableWidth == 0)) {
        return 1.0;
    }

    const double nonJustifiableWidth
        = m_systemLeftMar + m_systemRightMar + m_castOffTotalWidth - m_castOffJustifiableWidth;
    double estimatedRatio
        = (double)(doc->m_drawingPageContentWidth - nonJustifiableWidth) / ((double)m_castOffJustifiableWidth);

    // Apply dampening and bound compression
    estimatedRatio *= 0.95;
    estimatedRatio = std::max(estimatedRatio, 0.8);

    return estimatedRatio;
}

void System::ConvertToCastOffMensuralSystem(Doc *doc, System *targetSystem)
{
    assert(doc);
    assert(targetSystem);

    // We need to populate processing lists for processing the document by Layer
    InitProcessingListsParams initProcessingListsParams;
    Functor initProcessingLists(&Object::InitProcessingLists);
    this->Process(&initProcessingLists, &initProcessingListsParams);

    // The means no content? Checking just in case
    if (initProcessingListsParams.m_layerTree.child.empty()) return;

    ConvertToCastOffMensuralParams convertToCastOffMensuralParams(
        doc, targetSystem, &initProcessingListsParams.m_layerTree);
    // Store the list of staff N for detecting barLines that are on all systems
    for (auto const &staves : initProcessingListsParams.m_layerTree.child) {
        convertToCastOffMensuralParams.m_staffNs.push_back(staves.first);
    }

    Functor convertToCastOffMensural(&Object::ConvertToCastOffMensural);
    this->Process(&convertToCastOffMensural, &convertToCastOffMensuralParams);
}

void System::ConvertToUnCastOffMensuralSystem()
{
    // We need to populate processing lists for processing the document by Layer
    InitProcessingListsParams initProcessingListsParams;
    Functor initProcessingLists(&Object::InitProcessingLists);
    this->Process(&initProcessingLists, &initProcessingListsParams);

    // The means no content? Checking just in case
    if (initProcessingListsParams.m_layerTree.child.empty()) return;

    ConvertToUnCastOffMensuralParams convertToUnCastOffMensuralParams;

    Filters filters;
    // Now we can process by layer and move their content to (measure) segments
    for (auto const &staves : initProcessingListsParams.m_layerTree.child) {
        for (auto const &layers : staves.second.child) {
            // Create ad comparison object for each type / @n
            AttNIntegerComparison matchStaff(STAFF, staves.first);
            AttNIntegerComparison matchLayer(LAYER, layers.first);
            filters = { &matchStaff, &matchLayer };

            convertToUnCastOffMensuralParams.m_contentMeasure = NULL;
            convertToUnCastOffMensuralParams.m_contentLayer = NULL;

            Functor convertToUnCastOffMensural(&Object::ConvertToUnCastOffMensural);
            this->Process(&convertToUnCastOffMensural, &convertToUnCastOffMensuralParams, NULL, &filters);

            convertToUnCastOffMensuralParams.m_addSegmentsToDelete = false;
        }
    }

    // Detach the contentPage
    for (auto &measure : convertToUnCastOffMensuralParams.m_segmentsToDelete) {
        this->DeleteChild(measure);
    }
}

//----------------------------------------------------------------------------
// System functor methods
//----------------------------------------------------------------------------

FunctorCode System::Accept(MutableFunctor &functor)
{
    return functor.VisitSystem(this);
}

FunctorCode System::Accept(ConstFunctor &functor) const
{
    return functor.VisitSystem(this);
}

FunctorCode System::AcceptEnd(MutableFunctor &functor)
{
    return functor.VisitSystemEnd(this);
}

FunctorCode System::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitSystemEnd(this);
}

int System::ApplyPPUFactor(FunctorParams *functorParams)
{
    ApplyPPUFactorParams *params = vrv_params_cast<ApplyPPUFactorParams *>(functorParams);
    assert(params);

    if (m_xAbs != VRV_UNSET) m_xAbs /= params->m_page->GetPPUFactor();
    if (m_yAbs != VRV_UNSET) m_yAbs /= params->m_page->GetPPUFactor();
    m_systemLeftMar *= params->m_page->GetPPUFactor();
    m_systemRightMar *= params->m_page->GetPPUFactor();

    return FUNCTOR_CONTINUE;
}

int System::JustifyX(FunctorParams *functorParams)
{
    JustifyXParams *params = vrv_params_cast<JustifyXParams *>(functorParams);
    assert(params);

    params->m_measureXRel = 0;
    int margins = m_systemLeftMar + m_systemRightMar;
    int nonJustifiableWidth
        = margins + (m_drawingTotalWidth - m_drawingJustifiableWidth); // m_drawingTotalWidth includes the labels
    params->m_justifiableRatio
        = (double)(params->m_systemFullWidth - nonJustifiableWidth) / ((double)m_drawingJustifiableWidth);

    if (params->m_justifiableRatio < 0.8) {
        // Arbitrary value for avoiding over-compressed justification
        LogWarning("Justification is highly compressed (ratio smaller than 0.8: %lf)", params->m_justifiableRatio);
        LogWarning("\tSystem full width: %d", params->m_systemFullWidth);
        LogWarning("\tNon-justifiable width: %d", nonJustifiableWidth);
        LogWarning("\tDrawing justifiable width: %d", m_drawingJustifiableWidth);
    }

    // Check if we are on the last system of an mdiv.
    // Do not justify it if the non-justified width is less than a specified percent.
    if (this->IsLastOfMdiv() || this->IsLastOfSelection()) {
        double minLastJust = params->m_doc->GetOptions()->m_minLastJustification.GetValue();
        if ((minLastJust > 0) && (params->m_justifiableRatio > (1 / minLastJust))) {
            return FUNCTOR_SIBLINGS;
        }
    }

    return FUNCTOR_CONTINUE;
}

int System::JustifyY(FunctorParams *functorParams)
{
    JustifyYParams *params = vrv_params_cast<JustifyYParams *>(functorParams);
    assert(params);
    if (params->m_justificationSum <= 0.0) return FUNCTOR_STOP;
    if (params->m_spaceToDistribute <= 0) return FUNCTOR_STOP;

    const double systemJustificationFactor = params->m_doc->GetOptions()->m_justificationSystem.GetValue();
    const double shift = systemJustificationFactor / params->m_justificationSum * params->m_spaceToDistribute;

    if (!this->IsFirstInPage()) {
        params->m_cumulatedShift += shift;
    }

    this->SetDrawingYRel(this->GetDrawingY() - params->m_cumulatedShift);

    params->m_relativeShift = 0;
    m_systemAligner.Process(params->m_functor, params);

    return FUNCTOR_SIBLINGS;
}

int System::CastOffPages(FunctorParams *functorParams)
{
    CastOffPagesParams *params = vrv_params_cast<CastOffPagesParams *>(functorParams);
    assert(params);

    int currentShift = params->m_shift;
    // We use params->m_pageHeadHeight to check if we have passed the first page already
    if (params->m_pgHeadHeight != VRV_UNSET) {
        currentShift += params->m_pgHeadHeight + params->m_pgFootHeight;
    }
    else {
        currentShift += params->m_pgHead2Height + params->m_pgFoot2Height;
    }

    const int systemMaxPerPage = params->m_doc->GetOptions()->m_systemMaxPerPage.GetValue();
    const int childCount = params->m_currentPage->GetChildCount();
    if ((systemMaxPerPage && systemMaxPerPage == childCount)
        || (childCount > 0 && (this->m_drawingYRel - this->GetHeight() - currentShift < 0))) {
        // If this is the last system in the list, it doesn't fit the page and it's leftover system (has just one
        // measure) => add the system content to the previous system
        Object *nextSystem = params->m_contentPage->GetNext(this, SYSTEM);
        Object *lastSystem = params->m_currentPage->GetLast(SYSTEM);
        if (!nextSystem && lastSystem && (this == params->m_leftoverSystem)) {
            ArrayOfObjects &children = this->GetChildrenForModification();
            for (Object *child : children) {
                child->MoveItselfTo(lastSystem);
            }
            return FUNCTOR_SIBLINGS;
        }

        params->m_currentPage = new Page();
        // Use VRV_UNSET value as a flag
        params->m_pgHeadHeight = VRV_UNSET;
        assert(params->m_doc->GetPages());
        params->m_doc->GetPages()->AddChild(params->m_currentPage);
        params->m_shift = this->m_drawingYRel - params->m_pageHeight;
    }

    // First add all pending objects
    ArrayOfObjects::iterator iter;
    for (iter = params->m_pendingPageElements.begin(); iter != params->m_pendingPageElements.end(); ++iter) {
        params->m_currentPage->AddChild(*iter);
    }
    params->m_pendingPageElements.clear();

    // Special case where we use the Relinquish method.
    // We want to move the system to the currentPage. However, we cannot use DetachChild
    // from the contentPage because this screws up the iterator. Relinquish gives up
    // the ownership of the system - the contentPage itself will be deleted afterwards.
    System *system = vrv_cast<System *>(params->m_contentPage->Relinquish(this->GetIdx()));
    assert(system);
    params->m_currentPage->AddChild(system);

    return FUNCTOR_SIBLINGS;
}

int System::CastOffSystems(FunctorParams *functorParams)
{
    CastOffSystemsParams *params = vrv_params_cast<CastOffSystemsParams *>(functorParams);
    assert(params);

    // We are starting a new system we need to cast off
    params->m_contentSystem = this;
    // We also need to create a new target system and add it to the page
    System *system = new System();
    params->m_page->AddChild(system);
    params->m_currentSystem = system;

    params->m_shift = -this->GetDrawingLabelsWidth();
    params->m_currentScoreDefWidth
        = params->m_page->m_drawingScoreDef.GetDrawingWidth() + this->GetDrawingAbbrLabelsWidth();

    return FUNCTOR_CONTINUE;
}

int System::CastOffSystemsEnd(FunctorParams *functorParams)
{
    CastOffSystemsParams *params = vrv_params_cast<CastOffSystemsParams *>(functorParams);
    assert(params);

    if (params->m_pendingElements.empty()) return FUNCTOR_CONTINUE;

    // Otherwise add all pendings objects
    ArrayOfObjects::iterator iter;
    for (iter = params->m_pendingElements.begin(); iter != params->m_pendingElements.end(); ++iter) {
        params->m_currentSystem->AddChild(*iter);
    }

    return FUNCTOR_CONTINUE;
}

int System::CastOffEncoding(FunctorParams *functorParams)
{
    CastOffEncodingParams *params = vrv_params_cast<CastOffEncodingParams *>(functorParams);
    assert(params);

    // We are starting a new system we need to cast off
    params->m_contentSystem = this;
    // Create the new system but do not add it to the page yet.
    // It will be added when reaching a pb / sb or at the end of the score in PageMilestoneEnd::CastOffEncoding
    assert(!params->m_currentSystem);
    params->m_currentSystem = new System();

    return FUNCTOR_CONTINUE;
}

int System::CastOffToSelection(FunctorParams *functorParams)
{
    CastOffToSelectionParams *params = vrv_params_cast<CastOffToSelectionParams *>(functorParams);
    assert(params);

    // We are starting a new system we need to cast off
    params->m_contentSystem = this;
    // We also need to create a new target system and add it to the page
    System *system = new System();
    params->m_page->AddChild(system);
    params->m_currentSystem = system;

    return FUNCTOR_CONTINUE;
}

int System::UnCastOff(FunctorParams *functorParams)
{
    UnCastOffParams *params = vrv_params_cast<UnCastOffParams *>(functorParams);
    assert(params);

    // Just move all the content of the system to the continuous one (parameter)
    // Use the MoveChildrenFrom method that moves and relinquishes them
    // See Object::Relinquish
    params->m_currentSystem->MoveChildrenFrom(this);

    return FUNCTOR_CONTINUE;
}

int System::Transpose(FunctorParams *functorParams)
{
    TransposeParams *params = vrv_params_cast<TransposeParams *>(functorParams);
    assert(params);

    // Check whether we are in the selected mdiv
    if (!params->m_selectedMdivID.empty()
        && (std::find(params->m_currentMdivIDs.begin(), params->m_currentMdivIDs.end(), params->m_selectedMdivID)
            == params->m_currentMdivIDs.end())) {
        return FUNCTOR_SIBLINGS;
    }

    return FUNCTOR_CONTINUE;
}

} // namespace vrv
