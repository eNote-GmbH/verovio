/////////////////////////////////////////////////////////////////////////////
// Authors:     Laurent Pugin and Rodolfo Zitellini
// Created:     2014
// Copyright (c) Authors and others. All rights reserved.
//
// Code generated using a modified version of libmei
// by Andrew Hankinson, Alastair Porter, and Others
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// NOTE: this file was generated with the Verovio libmei version and
// should not be edited because changes will be lost.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_ATTS_CMN_H__
#define __VRV_ATTS_CMN_H__

#include "att.h"
#include "attdef.h"
#include "pugixml.hpp"

//----------------------------------------------------------------------------

#include <string>

namespace vrv {

//----------------------------------------------------------------------------
// AttArpegLog
//----------------------------------------------------------------------------

class AttArpegLog : public Att {
public:
    AttArpegLog();
    virtual ~AttArpegLog();

    /** Reset the default values for the attribute class **/
    void ResetArpegLog();

    /** Read the values for the attribute class **/
    bool ReadArpegLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteArpegLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetOrder(arpegLog_ORDER order_) { m_order = order_; };
    arpegLog_ORDER GetOrder() const { return m_order; };
    bool HasOrder();
    ///@}

private:
    /** Describes the direction in which an arpeggio is to be performed. **/
    arpegLog_ORDER m_order;

    /* include <attorder> */
};

//----------------------------------------------------------------------------
// AttArpegVis
//----------------------------------------------------------------------------

class AttArpegVis : public Att {
public:
    AttArpegVis();
    virtual ~AttArpegVis();

    /** Reset the default values for the attribute class **/
    void ResetArpegVis();

    /** Read the values for the attribute class **/
    bool ReadArpegVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteArpegVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetArrow(data_BOOLEAN arrow_) { m_arrow = arrow_; };
    data_BOOLEAN GetArrow() const { return m_arrow; };
    bool HasArrow();
    ///@}

private:
    /** Indicates if an arrowhead is to be drawn as part of the arpeggiation symbol. **/
    data_BOOLEAN m_arrow;

    /* include <attarrow> */
};

//----------------------------------------------------------------------------
// AttBTremLog
//----------------------------------------------------------------------------

class AttBTremLog : public Att {
public:
    AttBTremLog();
    virtual ~AttBTremLog();

    /** Reset the default values for the attribute class **/
    void ResetBTremLog();

    /** Read the values for the attribute class **/
    bool ReadBTremLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBTremLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(bTremLog_FORM form_) { m_form = form_; };
    bTremLog_FORM GetForm() const { return m_form; };
    bool HasForm();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    bTremLog_FORM m_form;

    /* include <attform> */
};

//----------------------------------------------------------------------------
// AttBeamed
//----------------------------------------------------------------------------

class AttBeamed : public Att {
public:
    AttBeamed();
    virtual ~AttBeamed();

    /** Reset the default values for the attribute class **/
    void ResetBeamed();

    /** Read the values for the attribute class **/
    bool ReadBeamed(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeamed(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBeam(std::string beam_) { m_beam = beam_; };
    std::string GetBeam() const { return m_beam; };
    bool HasBeam();
    ///@}

private:
    /** Indicates that this event is "under a beam". **/
    std::string m_beam;

    /* include <attbeam> */
};

//----------------------------------------------------------------------------
// AttBeamedwith
//----------------------------------------------------------------------------

class AttBeamedwith : public Att {
public:
    AttBeamedwith();
    virtual ~AttBeamedwith();

    /** Reset the default values for the attribute class **/
    void ResetBeamedwith();

    /** Read the values for the attribute class **/
    bool ReadBeamedwith(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeamedwith(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBeamWith(data_OTHERSTAFF beamWith_) { m_beamWith = beamWith_; };
    data_OTHERSTAFF GetBeamWith() const { return m_beamWith; };
    bool HasBeamWith();
    ///@}

private:
    /**
     * In the case of cross-staff beams, the beam.with attribute is used to indicate
     * which staff the beam is connected to; that is, the staff above or the staff
     * below.
     **/
    data_OTHERSTAFF m_beamWith;

    /* include <attbeam.with> */
};

//----------------------------------------------------------------------------
// AttBeamingLog
//----------------------------------------------------------------------------

class AttBeamingLog : public Att {
public:
    AttBeamingLog();
    virtual ~AttBeamingLog();

    /** Reset the default values for the attribute class **/
    void ResetBeamingLog();

    /** Read the values for the attribute class **/
    bool ReadBeamingLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeamingLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBeamGroup(std::string beamGroup_) { m_beamGroup = beamGroup_; };
    std::string GetBeamGroup() const { return m_beamGroup; };
    bool HasBeamGroup();
    //
    void SetBeamRests(data_BOOLEAN beamRests_) { m_beamRests = beamRests_; };
    data_BOOLEAN GetBeamRests() const { return m_beamRests; };
    bool HasBeamRests();
    ///@}

private:
    /**
     * Provides an example of how automated beaming (including secondary beams) is to
     * be performed.
     **/
    std::string m_beamGroup;
    /**
     * Indicates whether automatically-drawn beams should include rests shorter than a
     * quarter note duration.
     **/
    data_BOOLEAN m_beamRests;

    /* include <attbeam.rests> */
};

//----------------------------------------------------------------------------
// AttBeamrend
//----------------------------------------------------------------------------

class AttBeamrend : public Att {
public:
    AttBeamrend();
    virtual ~AttBeamrend();

    /** Reset the default values for the attribute class **/
    void ResetBeamrend();

    /** Read the values for the attribute class **/
    bool ReadBeamrend(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeamrend(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(beamrend_FORM form_) { m_form = form_; };
    beamrend_FORM GetForm() const { return m_form; };
    bool HasForm();
    //
    void SetSlope(double slope_) { m_slope = slope_; };
    double GetSlope() const { return m_slope; };
    bool HasSlope();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    beamrend_FORM m_form;
    /** Records the slope of the beam. **/
    double m_slope;

    /* include <attslope> */
};

//----------------------------------------------------------------------------
// AttBeamsecondary
//----------------------------------------------------------------------------

class AttBeamsecondary : public Att {
public:
    AttBeamsecondary();
    virtual ~AttBeamsecondary();

    /** Reset the default values for the attribute class **/
    void ResetBeamsecondary();

    /** Read the values for the attribute class **/
    bool ReadBeamsecondary(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeamsecondary(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBreaksec(int breaksec_) { m_breaksec = breaksec_; };
    int GetBreaksec() const { return m_breaksec; };
    bool HasBreaksec();
    ///@}

private:
    /**
     * Presence of this attribute indicates that the secondary beam should be broken
     * following this note/chord.
     * The value of the attribute records the number of beams which should remain
     * unbroken.
     **/
    int m_breaksec;

    /* include <attbreaksec> */
};

//----------------------------------------------------------------------------
// AttBeatRptLog
//----------------------------------------------------------------------------

class AttBeatRptLog : public Att {
public:
    AttBeatRptLog();
    virtual ~AttBeatRptLog();

    /** Reset the default values for the attribute class **/
    void ResetBeatRptLog();

    /** Read the values for the attribute class **/
    bool ReadBeatRptLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeatRptLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBeatDef(data_DURATION beatDef_) { m_beatDef = beatDef_; };
    data_DURATION GetBeatDef() const { return m_beatDef; };
    bool HasBeatDef();
    ///@}

private:
    /** --- **/
    data_DURATION m_beatDef;

    /* include <attbeatDef> */
};

//----------------------------------------------------------------------------
// AttBeatRptVis
//----------------------------------------------------------------------------

class AttBeatRptVis : public Att {
public:
    AttBeatRptVis();
    virtual ~AttBeatRptVis();

    /** Reset the default values for the attribute class **/
    void ResetBeatRptVis();

    /** Read the values for the attribute class **/
    bool ReadBeatRptVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBeatRptVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(data_BEATRPT_REND form_) { m_form = form_; };
    data_BEATRPT_REND GetForm() const { return m_form; };
    bool HasForm();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    data_BEATRPT_REND m_form;

    /* include <attform> */
};

//----------------------------------------------------------------------------
// AttBendGes
//----------------------------------------------------------------------------

class AttBendGes : public Att {
public:
    AttBendGes();
    virtual ~AttBendGes();

    /** Reset the default values for the attribute class **/
    void ResetBendGes();

    /** Read the values for the attribute class **/
    bool ReadBendGes(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteBendGes(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetAmount(std::string amount_) { m_amount = amount_; };
    std::string GetAmount() const { return m_amount; };
    bool HasAmount();
    ///@}

private:
    /**
     * Records the amount of detuning.
     * The decimal values should be rendered as a fraction (or an integer plus a
     * fraction) along with the bend symbol.
     **/
    std::string m_amount;

    /* include <attamount> */
};

//----------------------------------------------------------------------------
// AttCutout
//----------------------------------------------------------------------------

class AttCutout : public Att {
public:
    AttCutout();
    virtual ~AttCutout();

    /** Reset the default values for the attribute class **/
    void ResetCutout();

    /** Read the values for the attribute class **/
    bool ReadCutout(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteCutout(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetCutout(cutout_CUTOUT cutout_) { m_cutout = cutout_; };
    cutout_CUTOUT GetCutout() const { return m_cutout; };
    bool HasCutout();
    ///@}

private:
    /** "Cut-out" style indicated for this measure. **/
    cutout_CUTOUT m_cutout;

    /* include <attcutout> */
};

//----------------------------------------------------------------------------
// AttExpandable
//----------------------------------------------------------------------------

class AttExpandable : public Att {
public:
    AttExpandable();
    virtual ~AttExpandable();

    /** Reset the default values for the attribute class **/
    void ResetExpandable();

    /** Read the values for the attribute class **/
    bool ReadExpandable(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteExpandable(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetExpand(data_BOOLEAN expand_) { m_expand = expand_; };
    data_BOOLEAN GetExpand() const { return m_expand; };
    bool HasExpand();
    ///@}

private:
    /**
     * Indicates whether to render a repeat symbol or the source material to which it
     * refers.
     * A value of 'true' renders the source material, while 'false' displays the repeat
     * symbol.
     **/
    data_BOOLEAN m_expand;

    /* include <attexpand> */
};

//----------------------------------------------------------------------------
// AttFTremLog
//----------------------------------------------------------------------------

class AttFTremLog : public Att {
public:
    AttFTremLog();
    virtual ~AttFTremLog();

    /** Reset the default values for the attribute class **/
    void ResetFTremLog();

    /** Read the values for the attribute class **/
    bool ReadFTremLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteFTremLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(fTremLog_FORM form_) { m_form = form_; };
    fTremLog_FORM GetForm() const { return m_form; };
    bool HasForm();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    fTremLog_FORM m_form;

    /* include <attform> */
};

//----------------------------------------------------------------------------
// AttFermataVis
//----------------------------------------------------------------------------

class AttFermataVis : public Att {
public:
    AttFermataVis();
    virtual ~AttFermataVis();

    /** Reset the default values for the attribute class **/
    void ResetFermataVis();

    /** Read the values for the attribute class **/
    bool ReadFermataVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteFermataVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(fermataVis_FORM form_) { m_form = form_; };
    fermataVis_FORM GetForm() const { return m_form; };
    bool HasForm();
    //
    void SetShape(fermataVis_SHAPE shape_) { m_shape = shape_; };
    fermataVis_SHAPE GetShape() const { return m_shape; };
    bool HasShape();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    fermataVis_FORM m_form;
    /** Describes a clef's shape. **/
    fermataVis_SHAPE m_shape;

    /* include <attshape> */
};

//----------------------------------------------------------------------------
// AttGraced
//----------------------------------------------------------------------------

class AttGraced : public Att {
public:
    AttGraced();
    virtual ~AttGraced();

    /** Reset the default values for the attribute class **/
    void ResetGraced();

    /** Read the values for the attribute class **/
    bool ReadGraced(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteGraced(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetGrace(data_GRACE grace_) { m_grace = grace_; };
    data_GRACE GetGrace() const { return m_grace; };
    bool HasGrace();
    //
    void SetGraceTime(int graceTime_) { m_graceTime = graceTime_; };
    int GetGraceTime() const { return m_graceTime; };
    bool HasGraceTime();
    ///@}

private:
    /**
     * Marks a note or chord as a "grace" (without a definitive written duration) and
     * records from which other note/chord it should "steal" time.
     **/
    data_GRACE m_grace;
    /** Records the amount of time to be "stolen" from a non-grace note/chord. **/
    int m_graceTime;

    /* include <attgrace.time> */
};

//----------------------------------------------------------------------------
// AttHairpinLog
//----------------------------------------------------------------------------

class AttHairpinLog : public Att {
public:
    AttHairpinLog();
    virtual ~AttHairpinLog();

    /** Reset the default values for the attribute class **/
    void ResetHairpinLog();

    /** Read the values for the attribute class **/
    bool ReadHairpinLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteHairpinLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(hairpinLog_FORM form_) { m_form = form_; };
    hairpinLog_FORM GetForm() const { return m_form; };
    bool HasForm();
    //
    void SetNiente(data_BOOLEAN niente_) { m_niente = niente_; };
    data_BOOLEAN GetNiente() const { return m_niente; };
    bool HasNiente();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    hairpinLog_FORM m_form;
    /**
     * Indicates that the hairpin starts from or ends in silence.
     * Often rendered as a small circle attached to the closed end of the hairpin. See
     * Gould, p. 108.
     **/
    data_BOOLEAN m_niente;

    /* include <attniente> */
};

//----------------------------------------------------------------------------
// AttHairpinVis
//----------------------------------------------------------------------------

class AttHairpinVis : public Att {
public:
    AttHairpinVis();
    virtual ~AttHairpinVis();

    /** Reset the default values for the attribute class **/
    void ResetHairpinVis();

    /** Read the values for the attribute class **/
    bool ReadHairpinVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteHairpinVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetOpening(std::string opening_) { m_opening = opening_; };
    std::string GetOpening() const { return m_opening; };
    bool HasOpening();
    ///@}

private:
    /**
     * Specifies the distance between the points of the open end of a hairpin dynamic
     * mark.
     **/
    std::string m_opening;

    /* include <attopening> */
};

//----------------------------------------------------------------------------
// AttHarpPedalLog
//----------------------------------------------------------------------------

class AttHarpPedalLog : public Att {
public:
    AttHarpPedalLog();
    virtual ~AttHarpPedalLog();

    /** Reset the default values for the attribute class **/
    void ResetHarpPedalLog();

    /** Read the values for the attribute class **/
    bool ReadHarpPedalLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteHarpPedalLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetC(harpPedalLog_C c_) { m_c = c_; };
    harpPedalLog_C GetC() const { return m_c; };
    bool HasC();
    //
    void SetD(harpPedalLog_D d_) { m_d = d_; };
    harpPedalLog_D GetD() const { return m_d; };
    bool HasD();
    //
    void SetE(harpPedalLog_E e_) { m_e = e_; };
    harpPedalLog_E GetE() const { return m_e; };
    bool HasE();
    //
    void SetF(harpPedalLog_F f_) { m_f = f_; };
    harpPedalLog_F GetF() const { return m_f; };
    bool HasF();
    //
    void SetG(harpPedalLog_G g_) { m_g = g_; };
    harpPedalLog_G GetG() const { return m_g; };
    bool HasG();
    //
    void SetA(harpPedalLog_A a_) { m_a = a_; };
    harpPedalLog_A GetA() const { return m_a; };
    bool HasA();
    //
    void SetB(harpPedalLog_B b_) { m_b = b_; };
    harpPedalLog_B GetB() const { return m_b; };
    bool HasB();
    ///@}

private:
    /** Indicates the pedal setting for the harp's C strings. **/
    harpPedalLog_C m_c;
    /** Indicates the pedal setting for the harp's D strings. **/
    harpPedalLog_D m_d;
    /** Indicates the pedal setting for the harp's E strings. **/
    harpPedalLog_E m_e;
    /** Indicates the pedal setting for the harp's F strings. **/
    harpPedalLog_F m_f;
    /** Indicates the pedal setting for the harp's G strings. **/
    harpPedalLog_G m_g;
    /** Indicates the pedal setting for the harp's A strings. **/
    harpPedalLog_A m_a;
    /** Indicates the pedal setting for the harp's B strings. **/
    harpPedalLog_B m_b;

    /* include <attb> */
};

//----------------------------------------------------------------------------
// AttLvpresent
//----------------------------------------------------------------------------

class AttLvpresent : public Att {
public:
    AttLvpresent();
    virtual ~AttLvpresent();

    /** Reset the default values for the attribute class **/
    void ResetLvpresent();

    /** Read the values for the attribute class **/
    bool ReadLvpresent(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteLvpresent(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetLv(data_BOOLEAN lv_) { m_lv = lv_; };
    data_BOOLEAN GetLv() const { return m_lv; };
    bool HasLv();
    ///@}

private:
    /**
     * Indicates the attachment of an l.v.
     * (laissez vibrer) sign to this element.
     **/
    data_BOOLEAN m_lv;

    /* include <attlv> */
};

//----------------------------------------------------------------------------
// AttMultiRestVis
//----------------------------------------------------------------------------

class AttMultiRestVis : public Att {
public:
    AttMultiRestVis();
    virtual ~AttMultiRestVis();

    /** Reset the default values for the attribute class **/
    void ResetMultiRestVis();

    /** Read the values for the attribute class **/
    bool ReadMultiRestVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteMultiRestVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBlock(data_BOOLEAN block_) { m_block = block_; };
    data_BOOLEAN GetBlock() const { return m_block; };
    bool HasBlock();
    ///@}

private:
    /**
     * When the block attribute is used, combinations of the 1, 2, and 4 measure rest
     * forms (Read, p.
     * 104) should be rendered instead of the modern form or an alternative symbol.
     **/
    data_BOOLEAN m_block;

    /* include <attblock> */
};

//----------------------------------------------------------------------------
// AttNoteGesCmn
//----------------------------------------------------------------------------

class AttNoteGesCmn : public Att {
public:
    AttNoteGesCmn();
    virtual ~AttNoteGesCmn();

    /** Reset the default values for the attribute class **/
    void ResetNoteGesCmn();

    /** Read the values for the attribute class **/
    bool ReadNoteGesCmn(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteNoteGesCmn(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetGliss(data_GLISSANDO gliss_) { m_gliss = gliss_; };
    data_GLISSANDO GetGliss() const { return m_gliss; };
    bool HasGliss();
    ///@}

private:
    /** Indicates that this element participates in a glissando. **/
    data_GLISSANDO m_gliss;

    /* include <attgliss> */
};

//----------------------------------------------------------------------------
// AttNumbered
//----------------------------------------------------------------------------

class AttNumbered : public Att {
public:
    AttNumbered();
    virtual ~AttNumbered();

    /** Reset the default values for the attribute class **/
    void ResetNumbered();

    /** Read the values for the attribute class **/
    bool ReadNumbered(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteNumbered(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetNum(int num_) { m_num = num_; };
    int GetNum() const { return m_num; };
    bool HasNum();
    ///@}

private:
    /**
     * Along with numbase, describes duration as a ratio.
     * num is the first value in the ratio, while numbase is the second.
     **/
    int m_num;

    /* include <attnum> */
};

//----------------------------------------------------------------------------
// AttNumberplacement
//----------------------------------------------------------------------------

class AttNumberplacement : public Att {
public:
    AttNumberplacement();
    virtual ~AttNumberplacement();

    /** Reset the default values for the attribute class **/
    void ResetNumberplacement();

    /** Read the values for the attribute class **/
    bool ReadNumberplacement(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteNumberplacement(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetNumPlace(data_PLACE numPlace_) { m_numPlace = numPlace_; };
    data_PLACE GetNumPlace() const { return m_numPlace; };
    bool HasNumPlace();
    //
    void SetNumVisible(data_BOOLEAN numVisible_) { m_numVisible = numVisible_; };
    data_BOOLEAN GetNumVisible() const { return m_numVisible; };
    bool HasNumVisible();
    ///@}

private:
    /** States where the tuplet number will be placed in relation to the note heads. **/
    data_PLACE m_numPlace;
    /** Determines if the tuplet number is visible. **/
    data_BOOLEAN m_numVisible;

    /* include <attnum.visible> */
};

//----------------------------------------------------------------------------
// AttOctaveLog
//----------------------------------------------------------------------------

class AttOctaveLog : public Att {
public:
    AttOctaveLog();
    virtual ~AttOctaveLog();

    /** Reset the default values for the attribute class **/
    void ResetOctaveLog();

    /** Read the values for the attribute class **/
    bool ReadOctaveLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteOctaveLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetColl(octaveLog_COLL coll_) { m_coll = coll_; };
    octaveLog_COLL GetColl() const { return m_coll; };
    bool HasColl();
    ///@}

private:
    /**
     * Indicates whether the octave displacement should be performed simultaneously
     * with the written notes, i.e., "coll' ottava".
     * Unlike other octave signs which are indicated by broken lines, coll' ottava
     * typically uses an unbroken line or a series of longer broken lines, ending with
     * a short vertical stroke. See Read, p. 47-48.
     **/
    octaveLog_COLL m_coll;

    /* include <attcoll> */
};

//----------------------------------------------------------------------------
// AttPedalLog
//----------------------------------------------------------------------------

class AttPedalLog : public Att {
public:
    AttPedalLog();
    virtual ~AttPedalLog();

    /** Reset the default values for the attribute class **/
    void ResetPedalLog();

    /** Read the values for the attribute class **/
    bool ReadPedalLog(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WritePedalLog(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetDir(pedalLog_DIR dir_) { m_dir = dir_; };
    pedalLog_DIR GetDir() const { return m_dir; };
    bool HasDir();
    ///@}

private:
    /** Records the position of the piano damper pedal. **/
    pedalLog_DIR m_dir;

    /* include <attdir> */
};

//----------------------------------------------------------------------------
// AttPedalVis
//----------------------------------------------------------------------------

class AttPedalVis : public Att {
public:
    AttPedalVis();
    virtual ~AttPedalVis();

    /** Reset the default values for the attribute class **/
    void ResetPedalVis();

    /** Read the values for the attribute class **/
    bool ReadPedalVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WritePedalVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetForm(pedalVis_FORM form_) { m_form = form_; };
    pedalVis_FORM GetForm() const { return m_form; };
    bool HasForm();
    ///@}

private:
    /** Records the appearance and usually the function of the bar line. **/
    pedalVis_FORM m_form;

    /* include <attform> */
};

//----------------------------------------------------------------------------
// AttPianopedals
//----------------------------------------------------------------------------

class AttPianopedals : public Att {
public:
    AttPianopedals();
    virtual ~AttPianopedals();

    /** Reset the default values for the attribute class **/
    void ResetPianopedals();

    /** Read the values for the attribute class **/
    bool ReadPianopedals(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WritePianopedals(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetPedalStyle(pianopedals_PEDALSTYLE pedalStyle_) { m_pedalStyle = pedalStyle_; };
    pianopedals_PEDALSTYLE GetPedalStyle() const { return m_pedalStyle; };
    bool HasPedalStyle();
    ///@}

private:
    /** Determines whether piano pedal marks should be rendered as lines or as terms. **/
    pianopedals_PEDALSTYLE m_pedalStyle;

    /* include <attpedal.style> */
};

//----------------------------------------------------------------------------
// AttRehearsal
//----------------------------------------------------------------------------

class AttRehearsal : public Att {
public:
    AttRehearsal();
    virtual ~AttRehearsal();

    /** Reset the default values for the attribute class **/
    void ResetRehearsal();

    /** Read the values for the attribute class **/
    bool ReadRehearsal(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteRehearsal(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetRehEnclose(rehearsal_REHENCLOSE rehEnclose_) { m_rehEnclose = rehEnclose_; };
    rehearsal_REHENCLOSE GetRehEnclose() const { return m_rehEnclose; };
    bool HasRehEnclose();
    ///@}

private:
    /** Describes the enclosing shape for rehearsal marks. **/
    rehearsal_REHENCLOSE m_rehEnclose;

    /* include <attreh.enclose> */
};

//----------------------------------------------------------------------------
// AttScoreDefVisCmn
//----------------------------------------------------------------------------

class AttScoreDefVisCmn : public Att {
public:
    AttScoreDefVisCmn();
    virtual ~AttScoreDefVisCmn();

    /** Reset the default values for the attribute class **/
    void ResetScoreDefVisCmn();

    /** Read the values for the attribute class **/
    bool ReadScoreDefVisCmn(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteScoreDefVisCmn(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetGridShow(data_BOOLEAN gridShow_) { m_gridShow = gridShow_; };
    data_BOOLEAN GetGridShow() const { return m_gridShow; };
    bool HasGridShow();
    ///@}

private:
    /** Determines whether to display guitar chord grids. **/
    data_BOOLEAN m_gridShow;

    /* include <attgrid.show> */
};

//----------------------------------------------------------------------------
// AttSlurrend
//----------------------------------------------------------------------------

class AttSlurrend : public Att {
public:
    AttSlurrend();
    virtual ~AttSlurrend();

    /** Reset the default values for the attribute class **/
    void ResetSlurrend();

    /** Read the values for the attribute class **/
    bool ReadSlurrend(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteSlurrend(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetSlurRend(data_CURVERENDITION slurRend_) { m_slurRend = slurRend_; };
    data_CURVERENDITION GetSlurRend() const { return m_slurRend; };
    bool HasSlurRend();
    ///@}

private:
    /** Describes the line style of the slur. **/
    data_CURVERENDITION m_slurRend;

    /* include <attslur.rend> */
};

//----------------------------------------------------------------------------
// AttStemsCmn
//----------------------------------------------------------------------------

class AttStemsCmn : public Att {
public:
    AttStemsCmn();
    virtual ~AttStemsCmn();

    /** Reset the default values for the attribute class **/
    void ResetStemsCmn();

    /** Read the values for the attribute class **/
    bool ReadStemsCmn(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteStemsCmn(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetStemWith(data_OTHERSTAFF stemWith_) { m_stemWith = stemWith_; };
    data_OTHERSTAFF GetStemWith() const { return m_stemWith; };
    bool HasStemWith();
    ///@}

private:
    /**
     * Contains an indication of which staff a note or chord that logically belongs to
     * the current staff should be visually placed on; that is, the one above or the
     * one below.
     **/
    data_OTHERSTAFF m_stemWith;

    /* include <attstem.with> */
};

//----------------------------------------------------------------------------
// AttTierend
//----------------------------------------------------------------------------

class AttTierend : public Att {
public:
    AttTierend();
    virtual ~AttTierend();

    /** Reset the default values for the attribute class **/
    void ResetTierend();

    /** Read the values for the attribute class **/
    bool ReadTierend(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteTierend(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetTieRend(data_CURVERENDITION tieRend_) { m_tieRend = tieRend_; };
    data_CURVERENDITION GetTieRend() const { return m_tieRend; };
    bool HasTieRend();
    ///@}

private:
    /** Describes the line style of the tie. **/
    data_CURVERENDITION m_tieRend;

    /* include <atttie.rend> */
};

//----------------------------------------------------------------------------
// AttTremmeasured
//----------------------------------------------------------------------------

class AttTremmeasured : public Att {
public:
    AttTremmeasured();
    virtual ~AttTremmeasured();

    /** Reset the default values for the attribute class **/
    void ResetTremmeasured();

    /** Read the values for the attribute class **/
    bool ReadTremmeasured(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteTremmeasured(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetMeasperf(data_DURATION measperf_) { m_measperf = measperf_; };
    data_DURATION GetMeasperf() const { return m_measperf; };
    bool HasMeasperf();
    ///@}

private:
    /** The performed duration of an individual note in a measured tremolo. **/
    data_DURATION m_measperf;

    /* include <attmeasperf> */
};

//----------------------------------------------------------------------------
// AttTupletVis
//----------------------------------------------------------------------------

class AttTupletVis : public Att {
public:
    AttTupletVis();
    virtual ~AttTupletVis();

    /** Reset the default values for the attribute class **/
    void ResetTupletVis();

    /** Read the values for the attribute class **/
    bool ReadTupletVis(pugi::xml_node element);

    /** Write the values for the attribute class **/
    bool WriteTupletVis(pugi::xml_node element);

    /**
     * @name Setters, getters and presence checker for class members.
     * The checker returns true if the attribute class is set (e.g., not equal
     * to the default value)
     **/
    ///@{
    void SetBracketPlace(data_PLACE bracketPlace_) { m_bracketPlace = bracketPlace_; };
    data_PLACE GetBracketPlace() const { return m_bracketPlace; };
    bool HasBracketPlace();
    //
    void SetBracketVisible(data_BOOLEAN bracketVisible_) { m_bracketVisible = bracketVisible_; };
    data_BOOLEAN GetBracketVisible() const { return m_bracketVisible; };
    bool HasBracketVisible();
    //
    void SetDurVisible(data_BOOLEAN durVisible_) { m_durVisible = durVisible_; };
    data_BOOLEAN GetDurVisible() const { return m_durVisible; };
    bool HasDurVisible();
    //
    void SetNumFormat(tupletVis_NUMFORMAT numFormat_) { m_numFormat = numFormat_; };
    tupletVis_NUMFORMAT GetNumFormat() const { return m_numFormat; };
    bool HasNumFormat();
    ///@}

private:
    /**
     * Used to state where a tuplet bracket will be placed in relation to the note
     * heads.
     **/
    data_PLACE m_bracketPlace;
    /** States whether a bracket should be rendered with a tuplet. **/
    data_BOOLEAN m_bracketVisible;
    /** Determines if the tuplet duration is visible. **/
    data_BOOLEAN m_durVisible;
    /** Controls how the num:numbase ratio is to be displayed. **/
    tupletVis_NUMFORMAT m_numFormat;

    /* include <attnum.format> */
};

} // vrv namespace

#endif // __VRV_ATTS_CMN_H__
