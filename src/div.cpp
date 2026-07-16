/////////////////////////////////////////////////////////////////////////////
// Name:        div.cpp
// Author:      Laurent Pugin
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "div.h"

//----------------------------------------------------------------------------

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
    this->ResetTextFlowLayout();
}

bool Div::IsSupportedChild(ClassId classId)
{
    if ((classId == DIV) || Object::IsTextFlowElement(classId)) return true;
    return TextLayoutElement::IsSupportedChild(classId);
}

bool Div::HasTextFlow() const
{
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

    if (this->HasTextFlow()) return m_textFlowHeight;

    int height = this->GetContentHeight();

    return height;
}

int Div::GetTotalWidth(const Doc *doc) const
{
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
