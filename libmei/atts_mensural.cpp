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

#include "atts_mensural.h"

//----------------------------------------------------------------------------

#include "object.h"

/* #include_block */

namespace vrv {
    
//----------------------------------------------------------------------------
// AttLigatureLog
//----------------------------------------------------------------------------

AttLigatureLog::AttLigatureLog(): Att() {
    ResetLigatureLog();
}

AttLigatureLog::~AttLigatureLog() {

}

void AttLigatureLog::ResetLigatureLog() {
    m_form = "";
}

bool AttLigatureLog::ReadLigatureLog(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("form")) {
        this->SetForm(StrToStr(element.attribute("form").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttLigatureLog::WriteLigatureLog(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasForm()) {
        element.append_attribute("form") = StrToStr(this->GetForm()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttLigatureLog::HasForm( )
{
    return (m_form != "");
}


/* include <attform> */

//----------------------------------------------------------------------------
// AttMensurVis
//----------------------------------------------------------------------------

AttMensurVis::AttMensurVis(): Att() {
    ResetMensurVis();
}

AttMensurVis::~AttMensurVis() {

}

void AttMensurVis::ResetMensurVis() {
    m_form = "";
    m_orient = ORIENTATION_NONE;
}

bool AttMensurVis::ReadMensurVis(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("form")) {
        this->SetForm(StrToStr(element.attribute("form").value()));
        hasAttribute = true;
    }
    if (element.attribute("orient")) {
        this->SetOrient(StrToOrientation(element.attribute("orient").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttMensurVis::WriteMensurVis(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasForm()) {
        element.append_attribute("form") = StrToStr(this->GetForm()).c_str();
        wroteAttribute = true;
    }
    if (this->HasOrient()) {
        element.append_attribute("orient") = OrientationToStr(this->GetOrient()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttMensurVis::HasForm( )
{
    return (m_form != "");
}

bool AttMensurVis::HasOrient( )
{
    return (m_orient != ORIENTATION_NONE);
}


/* include <attorient> */

//----------------------------------------------------------------------------
// AttMensuralLog
//----------------------------------------------------------------------------

AttMensuralLog::AttMensuralLog(): Att() {
    ResetMensuralLog();
}

AttMensuralLog::~AttMensuralLog() {

}

void AttMensuralLog::ResetMensuralLog() {
    m_mensurDot = BOOLEAN_NONE;
    m_mensurSign = MENSURATIONSIGN_NONE;
    m_mensurSlash = 0;
    m_proportNumInt = 0;
    m_proportNumbaseInt = 0;
}

bool AttMensuralLog::ReadMensuralLog(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("mensur.dot")) {
        this->SetMensurDot(StrToBool(element.attribute("mensur.dot").value()));
        hasAttribute = true;
    }
    if (element.attribute("mensur.sign")) {
        this->SetMensurSign(StrToMensurationSign(element.attribute("mensur.sign").value()));
        hasAttribute = true;
    }
    if (element.attribute("mensur.slash")) {
        this->SetMensurSlash(StrToInt(element.attribute("mensur.slash").value()));
        hasAttribute = true;
    }
    if (element.attribute("proport.num")) {
        this->SetProportNum(StrToInt(element.attribute("proport.num").value()));
        hasAttribute = true;
    }
    if (element.attribute("proport.numbase")) {
        this->SetProportNumbase(StrToInt(element.attribute("proport.numbase").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttMensuralLog::WriteMensuralLog(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasMensurDot()) {
        element.append_attribute("mensur.dot") = BoolToStr(this->GetMensurDot()).c_str();
        wroteAttribute = true;
    }
    if (this->HasMensurSign()) {
        element.append_attribute("mensur.sign") = MensurationSignToStr(this->GetMensurSign()).c_str();
        wroteAttribute = true;
    }
    if (this->HasMensurSlash()) {
        element.append_attribute("mensur.slash") = IntToStr(this->GetMensurSlash()).c_str();
        wroteAttribute = true;
    }
    if (this->HasProportNum()) {
        element.append_attribute("proport.num") = IntToStr(this->GetProportNum()).c_str();
        wroteAttribute = true;
    }
    if (this->HasProportNumbase()) {
        element.append_attribute("proport.numbase") = IntToStr(this->GetProportNumbase()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttMensuralLog::HasMensurDot( )
{
    return (m_mensurDot != BOOLEAN_NONE);
}

bool AttMensuralLog::HasMensurSign( )
{
    return (m_mensurSign != MENSURATIONSIGN_NONE);
}

bool AttMensuralLog::HasMensurSlash( )
{
    return (m_mensurSlash != 0);
}

bool AttMensuralLog::HasProportNum( )
{
    return (m_proportNumInt != 0);
}

bool AttMensuralLog::HasProportNumbase( )
{
    return (m_proportNumbaseInt != 0);
}


/* include <attproport.numbase> */

//----------------------------------------------------------------------------
// AttMensuralShared
//----------------------------------------------------------------------------

AttMensuralShared::AttMensuralShared(): Att() {
    ResetMensuralShared();
}

AttMensuralShared::~AttMensuralShared() {

}

void AttMensuralShared::ResetMensuralShared() {
    m_modusmaior = "";
    m_modusminor = "";
    m_prolatio = "";
    m_tempus = "";
}

bool AttMensuralShared::ReadMensuralShared(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("modusmaior")) {
        this->SetModusmaior(StrToStr(element.attribute("modusmaior").value()));
        hasAttribute = true;
    }
    if (element.attribute("modusminor")) {
        this->SetModusminor(StrToStr(element.attribute("modusminor").value()));
        hasAttribute = true;
    }
    if (element.attribute("prolatio")) {
        this->SetProlatio(StrToStr(element.attribute("prolatio").value()));
        hasAttribute = true;
    }
    if (element.attribute("tempus")) {
        this->SetTempus(StrToStr(element.attribute("tempus").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttMensuralShared::WriteMensuralShared(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasModusmaior()) {
        element.append_attribute("modusmaior") = StrToStr(this->GetModusmaior()).c_str();
        wroteAttribute = true;
    }
    if (this->HasModusminor()) {
        element.append_attribute("modusminor") = StrToStr(this->GetModusminor()).c_str();
        wroteAttribute = true;
    }
    if (this->HasProlatio()) {
        element.append_attribute("prolatio") = StrToStr(this->GetProlatio()).c_str();
        wroteAttribute = true;
    }
    if (this->HasTempus()) {
        element.append_attribute("tempus") = StrToStr(this->GetTempus()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttMensuralShared::HasModusmaior( )
{
    return (m_modusmaior != "");
}

bool AttMensuralShared::HasModusminor( )
{
    return (m_modusminor != "");
}

bool AttMensuralShared::HasProlatio( )
{
    return (m_prolatio != "");
}

bool AttMensuralShared::HasTempus( )
{
    return (m_tempus != "");
}


/* include <atttempus> */

//----------------------------------------------------------------------------
// AttMensuralVis
//----------------------------------------------------------------------------

AttMensuralVis::AttMensuralVis(): Att() {
    ResetMensuralVis();
}

AttMensuralVis::~AttMensuralVis() {

}

void AttMensuralVis::ResetMensuralVis() {
    m_mensurColor = "";
    m_mensurForm = "";
    m_mensurLoc = "";
    m_mensurOrient = "";
    m_mensurSize = "";
}

bool AttMensuralVis::ReadMensuralVis(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("mensur.color")) {
        this->SetMensurColor(StrToStr(element.attribute("mensur.color").value()));
        hasAttribute = true;
    }
    if (element.attribute("mensur.form")) {
        this->SetMensurForm(StrToStr(element.attribute("mensur.form").value()));
        hasAttribute = true;
    }
    if (element.attribute("mensur.loc")) {
        this->SetMensurLoc(StrToStr(element.attribute("mensur.loc").value()));
        hasAttribute = true;
    }
    if (element.attribute("mensur.orient")) {
        this->SetMensurOrient(StrToStr(element.attribute("mensur.orient").value()));
        hasAttribute = true;
    }
    if (element.attribute("mensur.size")) {
        this->SetMensurSize(StrToStr(element.attribute("mensur.size").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttMensuralVis::WriteMensuralVis(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasMensurColor()) {
        element.append_attribute("mensur.color") = StrToStr(this->GetMensurColor()).c_str();
        wroteAttribute = true;
    }
    if (this->HasMensurForm()) {
        element.append_attribute("mensur.form") = StrToStr(this->GetMensurForm()).c_str();
        wroteAttribute = true;
    }
    if (this->HasMensurLoc()) {
        element.append_attribute("mensur.loc") = StrToStr(this->GetMensurLoc()).c_str();
        wroteAttribute = true;
    }
    if (this->HasMensurOrient()) {
        element.append_attribute("mensur.orient") = StrToStr(this->GetMensurOrient()).c_str();
        wroteAttribute = true;
    }
    if (this->HasMensurSize()) {
        element.append_attribute("mensur.size") = StrToStr(this->GetMensurSize()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttMensuralVis::HasMensurColor( )
{
    return (m_mensurColor != "");
}

bool AttMensuralVis::HasMensurForm( )
{
    return (m_mensurForm != "");
}

bool AttMensuralVis::HasMensurLoc( )
{
    return (m_mensurLoc != "");
}

bool AttMensuralVis::HasMensurOrient( )
{
    return (m_mensurOrient != "");
}

bool AttMensuralVis::HasMensurSize( )
{
    return (m_mensurSize != "");
}


/* include <attmensur.size> */

//----------------------------------------------------------------------------
// AttNoteLogMensural
//----------------------------------------------------------------------------

AttNoteLogMensural::AttNoteLogMensural(): Att() {
    ResetNoteLogMensural();
}

AttNoteLogMensural::~AttNoteLogMensural() {

}

void AttNoteLogMensural::ResetNoteLogMensural() {
    m_lig = LIGATURE_NONE;
}

bool AttNoteLogMensural::ReadNoteLogMensural(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("lig")) {
        this->SetLig(StrToLigature(element.attribute("lig").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttNoteLogMensural::WriteNoteLogMensural(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasLig()) {
        element.append_attribute("lig") = LigatureToStr(this->GetLig()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttNoteLogMensural::HasLig( )
{
    return (m_lig != LIGATURE_NONE);
}


/* include <attlig> */

//----------------------------------------------------------------------------
// AttRestVisMensural
//----------------------------------------------------------------------------

AttRestVisMensural::AttRestVisMensural(): Att() {
    ResetRestVisMensural();
}

AttRestVisMensural::~AttRestVisMensural() {

}

void AttRestVisMensural::ResetRestVisMensural() {
    m_spacesInt = 0;
}

bool AttRestVisMensural::ReadRestVisMensural(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("spaces")) {
        this->SetSpaces(StrToInt(element.attribute("spaces").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttRestVisMensural::WriteRestVisMensural(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasSpaces()) {
        element.append_attribute("spaces") = IntToStr(this->GetSpaces()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttRestVisMensural::HasSpaces( )
{
    return (m_spacesInt != 0);
}


/* include <attspaces> */

bool Att::SetMensural( Object *element, std::string attrType, std::string attrValue )
{
    if ( (attrType == "form") && dynamic_cast<AttLigatureLog*>(element) ) {
        AttLigatureLog *att = dynamic_cast<AttLigatureLog*>(element);
        att->SetForm(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "form") && dynamic_cast<AttMensurVis*>(element) ) {
        AttMensurVis *att = dynamic_cast<AttMensurVis*>(element);
        att->SetForm(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "orient") && dynamic_cast<AttMensurVis*>(element) ) {
        AttMensurVis *att = dynamic_cast<AttMensurVis*>(element);
        att->SetOrient(att->StrToOrientation(attrValue));
    return true;
    }
    if ( (attrType == "mensurDot") && dynamic_cast<AttMensuralLog*>(element) ) {
        AttMensuralLog *att = dynamic_cast<AttMensuralLog*>(element);
        att->SetMensurDot(att->StrToBool(attrValue));
    return true;
    }
    if ( (attrType == "mensurSign") && dynamic_cast<AttMensuralLog*>(element) ) {
        AttMensuralLog *att = dynamic_cast<AttMensuralLog*>(element);
        att->SetMensurSign(att->StrToMensurationSign(attrValue));
    return true;
    }
    if ( (attrType == "mensurSlash") && dynamic_cast<AttMensuralLog*>(element) ) {
        AttMensuralLog *att = dynamic_cast<AttMensuralLog*>(element);
        att->SetMensurSlash(att->StrToInt(attrValue));
    return true;
    }
    if ( (attrType == "proportNumInt") && dynamic_cast<AttMensuralLog*>(element) ) {
        AttMensuralLog *att = dynamic_cast<AttMensuralLog*>(element);
        att->SetProportNum(att->StrToInt(attrValue));
    return true;
    }
    if ( (attrType == "proportNumbaseInt") && dynamic_cast<AttMensuralLog*>(element) ) {
        AttMensuralLog *att = dynamic_cast<AttMensuralLog*>(element);
        att->SetProportNumbase(att->StrToInt(attrValue));
    return true;
    }
    if ( (attrType == "modusmaior") && dynamic_cast<AttMensuralShared*>(element) ) {
        AttMensuralShared *att = dynamic_cast<AttMensuralShared*>(element);
        att->SetModusmaior(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "modusminor") && dynamic_cast<AttMensuralShared*>(element) ) {
        AttMensuralShared *att = dynamic_cast<AttMensuralShared*>(element);
        att->SetModusminor(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "prolatio") && dynamic_cast<AttMensuralShared*>(element) ) {
        AttMensuralShared *att = dynamic_cast<AttMensuralShared*>(element);
        att->SetProlatio(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "tempus") && dynamic_cast<AttMensuralShared*>(element) ) {
        AttMensuralShared *att = dynamic_cast<AttMensuralShared*>(element);
        att->SetTempus(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "mensurColor") && dynamic_cast<AttMensuralVis*>(element) ) {
        AttMensuralVis *att = dynamic_cast<AttMensuralVis*>(element);
        att->SetMensurColor(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "mensurForm") && dynamic_cast<AttMensuralVis*>(element) ) {
        AttMensuralVis *att = dynamic_cast<AttMensuralVis*>(element);
        att->SetMensurForm(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "mensurLoc") && dynamic_cast<AttMensuralVis*>(element) ) {
        AttMensuralVis *att = dynamic_cast<AttMensuralVis*>(element);
        att->SetMensurLoc(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "mensurOrient") && dynamic_cast<AttMensuralVis*>(element) ) {
        AttMensuralVis *att = dynamic_cast<AttMensuralVis*>(element);
        att->SetMensurOrient(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "mensurSize") && dynamic_cast<AttMensuralVis*>(element) ) {
        AttMensuralVis *att = dynamic_cast<AttMensuralVis*>(element);
        att->SetMensurSize(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "lig") && dynamic_cast<AttNoteLogMensural*>(element) ) {
        AttNoteLogMensural *att = dynamic_cast<AttNoteLogMensural*>(element);
        att->SetLig(att->StrToLigature(attrValue));
    return true;
    }
    if ( (attrType == "spacesInt") && dynamic_cast<AttRestVisMensural*>(element) ) {
        AttRestVisMensural *att = dynamic_cast<AttRestVisMensural*>(element);
        att->SetSpaces(att->StrToInt(attrValue));
    return true;
    }

    return false;
}


} // vrv namespace
    