/////////////////////////////////////////////////////////////////////////////
// Name:        textflow.h
// Author:      Verovio contributors
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_TEXT_FLOW_H__
#define __VRV_TEXT_FLOW_H__

#include "atts_figtable.h"
#include "atts_shared.h"
#include "facsimileinterface.h"
#include "linkinginterface.h"
#include "syl.h"
#include "textelement.h"

namespace vrv {

enum class TextFlowStackAlignment { Left, Right, Center, RightDigit };

/**
 * Shared implementation for the MEI block-level text elements <head>, <p>,
 * <lg>, and <l>. The ClassId retains the semantic element name while this
 * class keeps their common import, export, and traversal behavior local.
 */
class TextFlowElement : public Object,
                        public LinkingInterface,
                        public AttLabelled,
                        public AttLang,
                        public AttLayerIdent,
                        public AttNNumberLike,
                        public AttPlacementRelStaff,
                        public AttStaffIdent,
                        public AttTyped,
                        public AttTypography,
                        public AttWhitespace,
                        public AttXy {
public:
    explicit TextFlowElement(ClassId classId);
    ~TextFlowElement() override = default;

    Object *Clone() const override { return new TextFlowElement(*this); }
    void Reset() override;
    std::string GetClassName() const override;
    bool IsSupportedChild(ClassId classId) override;
    int GetDrawingX() const override { return m_textFlowDrawingX; }
    int GetDrawingY() const override { return m_textFlowDrawingY; }
    void SetTextFlowDrawingX(int x) { m_textFlowDrawingX = x; }
    void SetTextFlowDrawingY(int y) { m_textFlowDrawingY = y; }

    LinkingInterface *GetLinkingInterface() override { return vrv_cast<LinkingInterface *>(this); }
    const LinkingInterface *GetLinkingInterface() const override { return vrv_cast<const LinkingInterface *>(this); }

    const std::string &GetRhythm() const { return m_rhythm; }
    void SetRhythm(const std::string &rhythm) { m_rhythm = rhythm; }
    bool HasRhythm() const { return !m_rhythm.empty(); }

    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;

private:
    std::string m_rhythm;
    int m_textFlowDrawingX;
    int m_textFlowDrawingY;
};

class Head : public TextFlowElement {
public:
    Head() : TextFlowElement(HEAD) {}
    Object *Clone() const override { return new Head(*this); }
    std::string GetClassName() const override { return "head"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class Paragraph : public TextFlowElement {
public:
    Paragraph() : TextFlowElement(P) {}
    Object *Clone() const override { return new Paragraph(*this); }
    std::string GetClassName() const override { return "p"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class LineGroup : public TextFlowElement {
public:
    LineGroup() : TextFlowElement(LG) {}
    Object *Clone() const override { return new LineGroup(*this); }
    std::string GetClassName() const override { return "lg"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class Line : public TextFlowElement {
public:
    Line() : TextFlowElement(L) {}
    Object *Clone() const override { return new Line(*this); }
    std::string GetClassName() const override { return "l"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

/** Shared MEI attributes and drawing state for table, caption, tr, td, and th. */
class TextFlowTableElement : public Object,
                             public LinkingInterface,
                             public FacsimileInterface,
                             public AttClassed,
                             public AttLabelled,
                             public AttLang,
                             public AttNNumberLike,
                             public AttResponsibility,
                             public AttTyped,
                             public AttXy {
public:
    explicit TextFlowTableElement(ClassId classId);
    ~TextFlowTableElement() override = default;

    Object *Clone() const override { return new TextFlowTableElement(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "textFlowTableElement"; }
    bool IsSupportedChild(ClassId classId) override;
    int GetDrawingX() const override { return m_drawingX; }
    int GetDrawingY() const override { return m_drawingY; }
    void SetTextFlowDrawingX(int x) { m_drawingX = x; }
    void SetTextFlowDrawingY(int y) { m_drawingY = y; }

    LinkingInterface *GetLinkingInterface() override { return vrv_cast<LinkingInterface *>(this); }
    const LinkingInterface *GetLinkingInterface() const override { return vrv_cast<const LinkingInterface *>(this); }
    FacsimileInterface *GetFacsimileInterface() override { return vrv_cast<FacsimileInterface *>(this); }
    const FacsimileInterface *GetFacsimileInterface() const override
    {
        return vrv_cast<const FacsimileInterface *>(this);
    }

    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;

private:
    int m_drawingX;
    int m_drawingY;
};

class Table : public TextFlowTableElement {
public:
    Table() : TextFlowTableElement(TABLE) {}
    Object *Clone() const override { return new Table(*this); }
    std::string GetClassName() const override { return "table"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class TableCaption : public TextFlowTableElement {
public:
    TableCaption() : TextFlowTableElement(CAPTION) {}
    Object *Clone() const override { return new TableCaption(*this); }
    std::string GetClassName() const override { return "caption"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class TableRow : public TextFlowTableElement {
public:
    TableRow() : TextFlowTableElement(TR) {}
    Object *Clone() const override { return new TableRow(*this); }
    std::string GetClassName() const override { return "tr"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class TableCell : public TextFlowTableElement, public AttTabular {
public:
    explicit TableCell(ClassId classId);
    void Reset() override;
};

class Td : public TableCell {
public:
    Td() : TableCell(TD) {}
    Object *Clone() const override { return new Td(*this); }
    std::string GetClassName() const override { return "td"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

class Th : public TableCell {
public:
    Th() : TableCell(TH) {}
    Object *Clone() const override { return new Th(*this); }
    std::string GetClassName() const override { return "th"; }
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
};

/**
 * A syllable used as inline text rather than as a note-attached lyric. It
 * retains Syl's MEI attributes and public type while avoiding layer-layout
 * visitors and providing text-flow drawing coordinates for bounding boxes.
 */
class TextFlowSyl : public Syl {
public:
    TextFlowSyl();
    ~TextFlowSyl() override = default;

    Object *Clone() const override { return new TextFlowSyl(*this); }
    int GetDrawingX() const override { return m_textFlowDrawingX; }
    int GetDrawingY() const override { return m_textFlowDrawingY; }
    void SetTextFlowDrawingX(int x) { m_textFlowDrawingX = x; }
    void SetTextFlowDrawingY(int y) { m_textFlowDrawingY = y; }

    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;

private:
    int m_textFlowDrawingX;
    int m_textFlowDrawingY;
};

/** MEI <stack>: inline, single-column stacked text. */
class Stack : public TextElement, public LinkingInterface, public AttLang, public AttNNumberLike, public AttWhitespace {
public:
    Stack();
    ~Stack() override = default;

    Object *Clone() const override { return new Stack(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "stack"; }
    bool IsSupportedChild(ClassId classId) override;
    int GetDrawingX() const override { return m_textFlowDrawingX; }
    int GetDrawingY() const override { return m_textFlowDrawingY; }
    void SetTextFlowDrawingX(int x) { m_textFlowDrawingX = x; }
    void SetTextFlowDrawingY(int y) { m_textFlowDrawingY = y; }

    LinkingInterface *GetLinkingInterface() override { return vrv_cast<LinkingInterface *>(this); }
    const LinkingInterface *GetLinkingInterface() const override { return vrv_cast<const LinkingInterface *>(this); }

    const std::string &GetDelimiter() const { return m_delimiter; }
    void SetDelimiter(const std::string &delimiter) { m_delimiter = delimiter; }
    bool HasDelimiter() const { return !m_delimiter.empty(); }

    std::string GetAlignment() const;
    bool SetAlignment(const std::string &alignment);
    TextFlowStackAlignment GetTextFlowAlignment() const { return m_alignment; }
    bool HasAlignment() const { return m_hasAlignment; }

    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;

private:
    std::string m_delimiter;
    TextFlowStackAlignment m_alignment;
    bool m_hasAlignment;
    int m_textFlowDrawingX;
    int m_textFlowDrawingY;
};

} // namespace vrv

#endif
