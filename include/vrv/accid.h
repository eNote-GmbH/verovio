/////////////////////////////////////////////////////////////////////////////
// Name:        accid.h
// Author:      Laurent Pugin
// Created:     201X
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ACCID_H__
#define __VRV_ACCID_H__

#include "atts_externalsymbols.h"
#include "atts_gestural.h"
#include "layerelement.h"
#include "positioninterface.h"
#include "visualoffsetinterface.h"

namespace vrv {

class AlignmentReference;

//----------------------------------------------------------------------------
// Accid
//----------------------------------------------------------------------------

/**
 * This class models the MEI <accid> element.
 */
class Accid : public LayerElement,
              public PositionInterface,
              public VisualOffsetInterface,
              public AttAccidental,
              public AttAccidentalGestural,
              public AttAccidLog,
              public AttColor,
              public AttEnclosingChars,
              public AttExtSym,
              public AttPlacementOnStaff,
              public AttPlacementRelEvent {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method resets all attribute classes
     */
    ///@{
    Accid();
    virtual ~Accid();
    Object *Clone() const override { return new Accid(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "Accid"; }
    ///@}

    /** Override the method since it is align to the staff */
    bool IsRelativeToStaff() const override { return (this->HasLoc() || (this->HasOloc() && this->HasPloc())); }

    /**
     * @name Getter to interfaces
     */
    ///@{
    PositionInterface *GetPositionInterface() override { return vrv_cast<PositionInterface *>(this); }
    const PositionInterface *GetPositionInterface() const override { return vrv_cast<const PositionInterface *>(this); }
    const VisualOffsetInterface *GetVisualOffsetInterface() const override
    {
        return vrv_cast<const VisualOffsetInterface *>(this);
    }
    ///@}

    /** Override the method since alignment is required */
    bool HasToBeAligned() const override { return true; }

    /**
     * @name Set and get drawing octave flag
     */
    ///@{
    void SetDrawingOctave(bool isDrawingOctave) { m_isDrawingOctave = isDrawingOctave; }
    bool GetDrawingOctave() const { return m_isDrawingOctave; }
    void SetDrawingOctaveAccid(Accid *drawingOctave) { m_drawingOctave = drawingOctave; }
    Accid *GetDrawingOctaveAccid() { return m_drawingOctave; }
    const Accid *GetDrawingOctaveAccid() const { return m_drawingOctave; }
    ///@}

    /**
     * @name Set and get drawing unison accid
     */
    ///@{
    void SetDrawingUnisonAccid(Accid *drawingUnison) { m_drawingUnison = drawingUnison; }
    Accid *GetDrawingUnisonAccid() { return m_drawingUnison; }
    const Accid *GetDrawingUnisonAccid() const { return m_drawingUnison; }
    ///@}

    /**
     * Retrieve SMuFL string for the accidental.
     * This will include brackets
     */
    std::wstring GetSymbolStr(const data_NOTATIONTYPE notationType) const;

    /**
     * Adjust X position of accid in relation to other element
     */
    void AdjustX(LayerElement *element, const Doc *doc, int staffSize, std::vector<Accid *> &leftAccids,
        std::vector<Accid *> &adjustedAccids);

    /**
     * Adjust accid position if it's placed above/below staff so that it does not overlap with ledger lines
     */
    void AdjustToLedgerLines(const Doc *doc, LayerElement *element, int staffSize);

    /**
     * @name Set and get same layer alignment
     */
    ///@{
    void IsAlignedWithSameLayer(bool alignWithSameLayer) { m_alignedWithSameLayer = alignWithSameLayer; }
    bool IsAlignedWithSameLayer() const { return m_alignedWithSameLayer; }
    ///@}

    //----------------//
    // Static methods //
    //----------------//

    /**
     * @name Method used for drawing accidentals on ornaments
     */
    static wchar_t GetAccidGlyph(data_ACCIDENTAL_WRITTEN);

    //----------//
    // Functors //
    //----------//

    /**
     * See Object::ResetData
     */
    int ResetData(FunctorParams *functorParams) override;

    /**
     * See Object::ResetHorizontalAlignment
     */
    int ResetHorizontalAlignment(FunctorParams *functorParams) override;

private:
    //
public:
    //
private:
    Accid *m_drawingOctave;
    Accid *m_drawingUnison;
    bool m_isDrawingOctave;
    bool m_alignedWithSameLayer;
};

//----------------------------------------------------------------------------
// AccidSpaceSort
//----------------------------------------------------------------------------

/**
 * Sort Object by drawing Y value or by layerN
 */
class AccidSpaceSort {

public:
    AccidSpaceSort() {}

    bool operator()(const Accid *first, const Accid *second) const
    {
        if (first->GetDrawingY() < second->GetDrawingY()) {
            return true;
        }
        else if (first->GetDrawingY() > second->GetDrawingY()) {
            return false;
        }
        else {
            // with unissons, natural should always be the last accidental (assuming there is a natural)
            if ((first->GetAccid() == ACCIDENTAL_WRITTEN_n) || (second->GetAccid() == ACCIDENTAL_WRITTEN_n)) {
                return (first->GetAccid() != ACCIDENTAL_WRITTEN_n);
            }
            return first->GetDrawingY() < second->GetDrawingY();
        }
    }
};

} // namespace vrv

#endif
