/////////////////////////////////////////////////////////////////////////////
// Name:        div.h
// Author:      Laurent Pugin
// Created:     2023
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_DIV_H__
#define __VRV_DIV_H__

#include "linkinginterface.h"
#include "textflowlayout.h"
#include "textlayoutelement.h"

namespace vrv {

//----------------------------------------------------------------------------
// Div
//----------------------------------------------------------------------------

/**
 * This class represents an MEI Div.
 * The current implementation accepts rend as child of div, which is not valid
 * See https://github.com/music-encoding/music-encoding/issues/1189
 */
class Div : public TextLayoutElement,
            public LinkingInterface,
            public AttLabelled,
            public AttLang,
            public AttNNumberLike,
            public AttWhitespace {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    Div();
    virtual ~Div();
    void Reset() override;
    std::string GetClassName() const override { return "div"; }
    ///@}

    /**
     * @name Get and set the inline drawing flag
     */
    ///@{
    bool GetDrawingInline() const { return m_drawingInline; }
    void SetDrawingInline(bool drawingInline) { m_drawingInline = drawingInline; }
    ///@}

    /**
     * @name Get the X and Y drawing position
     */
    ///@{
    int GetDrawingX() const override;
    int GetDrawingY() const override;
    ///@}

    bool IsSupportedChild(ClassId classId) override;

    LinkingInterface *GetLinkingInterface() override { return vrv_cast<LinkingInterface *>(this); }
    const LinkingInterface *GetLinkingInterface() const override { return vrv_cast<const LinkingInterface *>(this); }

    bool HasTextFlow() const;
    void ResetTextFlowLayout();
    void SetTextFlowSize(int width, int height);
    const TextFlowLayoutResult *GetTextFlowLayout(const Object *block) const;
    const TextFlowLayoutResult &CacheTextFlowLayout(TextFlowLayoutResult result);
    int GetTextFlowHeight() const { return m_textFlowHeight; }
    int GetTextFlowWidth() const { return m_textFlowWidth; }

    /**
     * @name Get and set the X and Y drawing relative positions
     */
    ///@{
    int GetDrawingXRel() const { return m_drawingXRel; }
    virtual void SetDrawingXRel(int drawingXRel);
    void CacheXRel(bool restore = false);
    int GetDrawingYRel() const { return m_drawingYRel; }
    virtual void SetDrawingYRel(int drawingYRel);
    void CacheYRel(bool restore = false);
    ///@}

    /**
     * Overriden to get the appropriate margin
     */
    int GetTotalHeight(const Doc *doc) const override;

    int GetTotalWidth(const Doc *doc) const override;

    //----------//
    // Functors //
    //----------//

    /**
     * Interface for class functor visitation
     */
    ///@{
    FunctorCode Accept(Functor &functor) override;
    FunctorCode Accept(ConstFunctor &functor) const override;
    FunctorCode AcceptEnd(Functor &functor) override;
    FunctorCode AcceptEnd(ConstFunctor &functor) const override;
    ///@}

private:
    //
public:
    //
private:
    /**
     * The Y drawing relative position of the object.
     * It is re-computed everytime the object is drawn and it is not stored in the file.
     */
    int m_drawingYRel;

    /**
     * The X drawing relative position of the object.
     * It is re-computed everytime the object is drawn and it is not stored in the file.
     */
    int m_drawingXRel;

    /**
     * A flag indicating that the div should be displayed inline
     */
    bool m_drawingInline;

    int m_textFlowHeight;
    int m_textFlowWidth;
    std::vector<TextFlowLayoutResult> m_textFlowLayouts;
};

} // namespace vrv

#endif
