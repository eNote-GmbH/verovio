/////////////////////////////////////////////////////////////////////////////
// Name:        div.cpp
// Author:      Laurent Pugin
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "div.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cassert>

//----------------------------------------------------------------------------

#include "doc.h"
#include "editorial.h"
#include "functor.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Div
//----------------------------------------------------------------------------

static const ClassRegistrar<Div> s_factory("div", DIV);

Div::Div() : TextLayoutElement(DIV), LinkingInterface(), AttLabelled(), AttLang(), AttNNumberLike(), AttWhitespace()
{
    this->RegisterInterface(LinkingInterface::GetAttClasses(), LinkingInterface::IsInterface());
    this->RegisterAttClass(ATT_LABELLED);
    this->RegisterAttClass(ATT_LANG);
    this->RegisterAttClass(ATT_NNUMBERLIKE);
    this->RegisterAttClass(ATT_WHITESPACE);
    this->Reset();
}

Div::~Div() {}

void Div::Reset()
{
    TextLayoutElement::Reset();
    LinkingInterface::Reset();
    this->ResetLabelled();
    this->ResetLang();
    this->ResetNNumberLike();
    this->ResetWhitespace();

    m_drawingInline = false;
    m_drawingXRel = 0;
    m_drawingYRel = 0;
    m_textFlowSource = nullptr;
    m_textFlowFragmentIndex = 0;
    m_textFlowFragmentStart = -1;
    m_textFlowFragmentEnd = -1;
    this->ResetTextFlowLayout();
}

bool Div::IsSupportedChild(ClassId classId)
{
    if ((classId == DIV) || (classId == PB) || Object::IsTextFlowElement(classId)) return true;
    return TextLayoutElement::IsSupportedChild(classId);
}

bool Div::HasTextFlow() const
{
    if (m_textFlowSource) return true;
    for (const Object *child : this->GetChildren()) {
        if (child->Is(DIV) || child->IsTextFlowElement()) return true;
    }
    return false;
}

void Div::ResetTextFlowLayout()
{
    m_textFlowHeight = 0;
    m_textFlowWidth = 0;
    m_textFlowLayouts.clear();
    m_textFlowDocumentLayouts.clear();
}

void Div::SetTextFlowSize(int width, int height)
{
    m_textFlowWidth = width;
    m_textFlowHeight = height;
}

const TextFlowLayoutResult *Div::GetTextFlowLayout(const Object *block) const
{
    for (const TextFlowLayoutResult &result : m_textFlowLayouts) {
        if (result.block == block) return &result;
    }
    return nullptr;
}

const TextFlowLayoutResult &Div::CacheTextFlowLayout(TextFlowLayoutResult result)
{
    m_textFlowLayouts.push_back(std::move(result));
    return m_textFlowLayouts.back();
}

const TextFlowDocumentLayoutResult *Div::GetTextFlowDocumentLayout(int availableWidth) const
{
    if (m_textFlowSource) return m_textFlowSource->GetTextFlowDocumentLayout(availableWidth);
    for (const TextFlowDocumentLayoutResult &result : m_textFlowDocumentLayouts) {
        if (result.availableWidth == availableWidth) return &result;
    }
    return nullptr;
}

const TextFlowDocumentLayoutResult &Div::CacheTextFlowDocumentLayout(TextFlowDocumentLayoutResult result)
{
    if (m_textFlowSource) return m_textFlowSource->CacheTextFlowDocumentLayout(std::move(result));
    m_textFlowDocumentLayouts.push_back(std::move(result));
    return m_textFlowDocumentLayouts.back();
}

void Div::SetTextFlowFragment(Div *source, int index, int startY, int endY)
{
    assert(!source || source != this);
    m_textFlowSource = source;
    m_textFlowFragmentIndex = index;
    m_textFlowFragmentStart = std::max(0, startY);
    m_textFlowFragmentEnd = std::max(m_textFlowFragmentStart, endY);
}

void Div::ResetTextFlowFragment()
{
    m_textFlowSource = nullptr;
    m_textFlowFragmentIndex = 0;
    m_textFlowFragmentStart = -1;
    m_textFlowFragmentEnd = -1;
}

int Div::GetDrawingX() const
{
    const Object *parent = this->GetParent();
    assert(parent);
    if (m_drawingInline) {
        return parent->GetDrawingX() + this->GetDrawingXRel();
    }
    return parent->GetDrawingX();
}

int Div::GetDrawingY() const
{
    const Object *parent = this->GetParent();
    assert(parent);
    if (m_drawingInline) {
        return parent->GetDrawingY() + this->GetDrawingYRel();
    }
    return parent->GetDrawingY();
}

void Div::SetDrawingXRel(int drawingXRel)
{
    m_drawingXRel = drawingXRel;
}

void Div::SetDrawingYRel(int drawingYRel)
{
    m_drawingYRel = drawingYRel;
}

int Div::GetTotalHeight(const Doc *doc) const
{
    assert(doc);

    if (this->HasTextFlowFragment()) return m_textFlowFragmentEnd - m_textFlowFragmentStart;
    if (m_textFlowSource) return m_textFlowSource->GetTextFlowHeight();
    if (this->HasTextFlow()) return m_textFlowHeight;

    int height = this->GetContentHeight();

    return height;
}

int Div::GetTotalWidth(const Doc *doc) const
{
    if (m_textFlowSource) return m_textFlowSource->GetTotalWidth(doc);
    if (this->HasTextFlow()) return (m_textFlowWidth > 0) ? m_textFlowWidth : doc->m_drawingPageContentWidth;
    if (!m_drawingInline) {
        return (doc->m_drawingPageContentWidth);
    }
    else {
        int width = this->GetContentWidth();
        return width;
    }
}

//----------------------------------------------------------------------------
// Functor methods
//----------------------------------------------------------------------------

FunctorCode Div::Accept(Functor &functor)
{
    return functor.VisitDiv(this);
}

FunctorCode Div::Accept(ConstFunctor &functor) const
{
    return functor.VisitDiv(this);
}

FunctorCode Div::AcceptEnd(Functor &functor)
{
    return functor.VisitDivEnd(this);
}

FunctorCode Div::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitDivEnd(this);
}

} // namespace vrv
