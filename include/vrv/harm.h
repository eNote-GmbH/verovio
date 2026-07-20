/////////////////////////////////////////////////////////////////////////////
// Name:        harm.h
// Author:      Laurent Pugin
// Created:     2016
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_HARM_H__
#define __VRV_HARM_H__

#include "atts_harmony.h"
#include "atts_visual.h"
#include "controlelement.h"
#include "textdirinterface.h"
#include "timeinterface.h"
#include "transposition.h"

namespace vrv {

class ChordDef;
class TextElement;

//----------------------------------------------------------------------------
// Harm (harmony)
//----------------------------------------------------------------------------

/**
 * This class models the MEI <harm> element.
 */
class Harm : public ControlElement,
             public TextListInterface,
             public TextDirInterface,
             public TimeSpanningInterface,
             public AttHarmLog,
             public AttHarmVis,
             public AttLang,
             public AttNNumberLike {
public:
    /**
     * @name Constructors, destructors, and other standard methods
     * Reset method reset all attribute classes
     */
    ///@{
    Harm();
    virtual ~Harm();
    Object *Clone() const override { return new Harm(*this); }
    void Reset() override;
    std::string GetClassName() const override { return "harm"; }
    ///@}

    /**
     * @name Getter to interfaces
     */
    ///@{
    TextDirInterface *GetTextDirInterface() override { return vrv_cast<TextDirInterface *>(this); }
    const TextDirInterface *GetTextDirInterface() const override { return vrv_cast<const TextDirInterface *>(this); }
    TimePointInterface *GetTimePointInterface() override { return vrv_cast<TimePointInterface *>(this); }
    const TimePointInterface *GetTimePointInterface() const override
    {
        return vrv_cast<const TimePointInterface *>(this);
    }
    TimeSpanningInterface *GetTimeSpanningInterface() override { return vrv_cast<TimeSpanningInterface *>(this); }
    const TimeSpanningInterface *GetTimeSpanningInterface() const override
    {
        return vrv_cast<const TimeSpanningInterface *>(this);
    }
    ///@}

    /**
     * Add an element (text, rend. etc.) to a harm.
     * Only supported elements will be actually added to the child list.
     */
    bool IsSupportedChild(ClassId classId) override;

    /** Order harmony rows by drawing group, with lower group IDs closer to the staff. */
    bool IsCloserToStaffThan(const FloatingObject *other, data_STAFFREL drawingPlace) const override;

    /** Resolved target of @chordref. The ChordDef remains owned by the scoreDef. */
    void SetChordDef(ChordDef *chordDef);
    void ResetChordDef() { m_chordDef = NULL; }
    ChordDef *GetChordDef() { return m_chordDef; }
    const ChordDef *GetChordDef() const { return m_chordDef; }
    bool HasChordDef() const { return m_chordDef != NULL; }

    /**
     * Transposition related. The int tracks where we have iterated through the string.
     */
    bool GetRootPitch(TransPitch &pitch, unsigned int &pos) const;
    void SetRootPitch(const TransPitch &pitch, unsigned int endPos);
    bool GetBassPitch(TransPitch &pitch) const;
    void SetBassPitch(const TransPitch &pitch);
    /** Concatenate all text descendants while retaining their separate formatting containers. */
    std::u32string GetTextContent() const;

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

protected:
    //
private:
    //
public:
    //
private:
    ChordDef *m_chordDef;
};

} // namespace vrv

#endif
