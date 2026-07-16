/////////////////////////////////////////////////////////////////////////////
// Name:        textflow.cpp
// Author:      Verovio contributors
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "textflow.h"

#include "editorial.h"
#include "functor.h"

namespace vrv {

static const ClassRegistrar<Head> s_headFactory("head", HEAD);
static const ClassRegistrar<Paragraph> s_pFactory("p", P);
static const ClassRegistrar<LineGroup> s_lgFactory("lg", LG);
static const ClassRegistrar<Line> s_lFactory("l", L);
static const ClassRegistrar<Stack> s_stackFactory("stack", STACK);

TextFlowElement::TextFlowElement(ClassId classId)
    : Object(classId)
    , LinkingInterface()
    , AttLabelled()
    , AttLang()
    , AttLayerIdent()
    , AttNNumberLike()
    , AttPlacementRelStaff()
    , AttStaffIdent()
    , AttTyped()
    , AttTypography()
    , AttWhitespace()
    , AttXy()
{
    this->RegisterInterface(LinkingInterface::GetAttClasses(), LinkingInterface::IsInterface());
    this->RegisterAttClass(ATT_LABELLED);
    this->RegisterAttClass(ATT_LANG);
    this->RegisterAttClass(ATT_LAYERIDENT);
    this->RegisterAttClass(ATT_NNUMBERLIKE);
    this->RegisterAttClass(ATT_PLACEMENTRELSTAFF);
    this->RegisterAttClass(ATT_STAFFIDENT);
    this->RegisterAttClass(ATT_TYPED);
    this->RegisterAttClass(ATT_TYPOGRAPHY);
    this->RegisterAttClass(ATT_WHITESPACE);
    this->RegisterAttClass(ATT_XY);
    this->Reset();
}

void TextFlowElement::Reset()
{
    Object::Reset();
    LinkingInterface::Reset();
    this->ResetLabelled();
    this->ResetLang();
    this->ResetLayerIdent();
    this->ResetNNumberLike();
    this->ResetPlacementRelStaff();
    this->ResetStaffIdent();
    this->ResetTyped();
    this->ResetTypography();
    this->ResetWhitespace();
    this->ResetXy();
    m_rhythm.clear();
    m_textFlowDrawingX = 0;
    m_textFlowDrawingY = 0;
}

std::string TextFlowElement::GetClassName() const
{
    return "textFlowElement";
}

bool TextFlowElement::IsSupportedChild(ClassId classId)
{
    if (this->Is(LG)) {
        return (classId == HEAD) || (classId == L) || (classId == LG) || Object::IsEditorialElement(classId);
    }
    return Object::IsTextElement(classId) || (classId == SYL) || Object::IsEditorialElement(classId);
}

FunctorCode TextFlowElement::Accept(Functor &functor)
{
    return functor.VisitObject(this);
}

FunctorCode TextFlowElement::Accept(ConstFunctor &functor) const
{
    return functor.VisitObject(this);
}

FunctorCode TextFlowElement::AcceptEnd(Functor &functor)
{
    return functor.VisitObjectEnd(this);
}

FunctorCode TextFlowElement::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitObjectEnd(this);
}

FunctorCode Head::Accept(Functor &functor)
{
    return functor.VisitHead(this);
}
FunctorCode Head::Accept(ConstFunctor &functor) const
{
    return functor.VisitHead(this);
}
FunctorCode Head::AcceptEnd(Functor &functor)
{
    return functor.VisitHeadEnd(this);
}
FunctorCode Head::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitHeadEnd(this);
}

FunctorCode Paragraph::Accept(Functor &functor)
{
    return functor.VisitParagraph(this);
}
FunctorCode Paragraph::Accept(ConstFunctor &functor) const
{
    return functor.VisitParagraph(this);
}
FunctorCode Paragraph::AcceptEnd(Functor &functor)
{
    return functor.VisitParagraphEnd(this);
}
FunctorCode Paragraph::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitParagraphEnd(this);
}

FunctorCode LineGroup::Accept(Functor &functor)
{
    return functor.VisitLineGroup(this);
}
FunctorCode LineGroup::Accept(ConstFunctor &functor) const
{
    return functor.VisitLineGroup(this);
}
FunctorCode LineGroup::AcceptEnd(Functor &functor)
{
    return functor.VisitLineGroupEnd(this);
}
FunctorCode LineGroup::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitLineGroupEnd(this);
}

FunctorCode Line::Accept(Functor &functor)
{
    return functor.VisitLine(this);
}
FunctorCode Line::Accept(ConstFunctor &functor) const
{
    return functor.VisitLine(this);
}
FunctorCode Line::AcceptEnd(Functor &functor)
{
    return functor.VisitLineEnd(this);
}
FunctorCode Line::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitLineEnd(this);
}

TextFlowSyl::TextFlowSyl() : Syl(), m_textFlowDrawingX(0), m_textFlowDrawingY(0) {}

FunctorCode TextFlowSyl::Accept(Functor &functor)
{
    return functor.VisitTextFlowSyl(this);
}

FunctorCode TextFlowSyl::Accept(ConstFunctor &functor) const
{
    return functor.VisitTextFlowSyl(this);
}

FunctorCode TextFlowSyl::AcceptEnd(Functor &functor)
{
    return functor.VisitTextFlowSylEnd(this);
}

FunctorCode TextFlowSyl::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitTextFlowSylEnd(this);
}

Stack::Stack() : TextElement(STACK), LinkingInterface(), AttLang(), AttNNumberLike(), AttWhitespace()
{
    this->RegisterInterface(LinkingInterface::GetAttClasses(), LinkingInterface::IsInterface());
    this->RegisterAttClass(ATT_LANG);
    this->RegisterAttClass(ATT_NNUMBERLIKE);
    this->RegisterAttClass(ATT_WHITESPACE);
    this->Reset();
}

void Stack::Reset()
{
    TextElement::Reset();
    LinkingInterface::Reset();
    this->ResetLang();
    this->ResetNNumberLike();
    this->ResetWhitespace();
    m_delimiter.clear();
    m_alignment = TextFlowStackAlignment::Left;
    m_hasAlignment = false;
    m_textFlowDrawingX = 0;
    m_textFlowDrawingY = 0;
}

std::string Stack::GetAlignment() const
{
    switch (m_alignment) {
        case TextFlowStackAlignment::Left: return "left";
        case TextFlowStackAlignment::Right: return "right";
        case TextFlowStackAlignment::Center: return "center";
        case TextFlowStackAlignment::RightDigit: return "rightdigit";
    }
    return "left";
}

bool Stack::SetAlignment(const std::string &alignment)
{
    if (alignment == "left")
        m_alignment = TextFlowStackAlignment::Left;
    else if (alignment == "right")
        m_alignment = TextFlowStackAlignment::Right;
    else if (alignment == "center")
        m_alignment = TextFlowStackAlignment::Center;
    else if (alignment == "rightdigit")
        m_alignment = TextFlowStackAlignment::RightDigit;
    else
        return false;
    m_hasAlignment = true;
    return true;
}

bool Stack::IsSupportedChild(ClassId classId)
{
    return Object::IsTextElement(classId) || (classId == SYL) || Object::IsEditorialElement(classId);
}

FunctorCode Stack::Accept(Functor &functor)
{
    return functor.VisitStack(this);
}
FunctorCode Stack::Accept(ConstFunctor &functor) const
{
    return functor.VisitStack(this);
}
FunctorCode Stack::AcceptEnd(Functor &functor)
{
    return functor.VisitStackEnd(this);
}
FunctorCode Stack::AcceptEnd(ConstFunctor &functor) const
{
    return functor.VisitStackEnd(this);
}

} // namespace vrv
