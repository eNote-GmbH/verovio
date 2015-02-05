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

#include "atts_tablature.h"

//----------------------------------------------------------------------------

#include "object.h"

/* #include_block */

namespace vrv {
    
//----------------------------------------------------------------------------
// AttNoteGesTablature
//----------------------------------------------------------------------------

AttNoteGesTablature::AttNoteGesTablature(): Att() {
    ResetNoteGesTablature();
}

AttNoteGesTablature::~AttNoteGesTablature() {

}

void AttNoteGesTablature::ResetNoteGesTablature() {
    m_tabFret = "";
    m_tabString = "";
}

bool AttNoteGesTablature::ReadNoteGesTablature(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("tab.fret")) {
        this->SetTabFret(StrToStr(element.attribute("tab.fret").value()));
        hasAttribute = true;
    }
    if (element.attribute("tab.string")) {
        this->SetTabString(StrToStr(element.attribute("tab.string").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttNoteGesTablature::WriteNoteGesTablature(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasTabFret()) {
        element.append_attribute("tab.fret") = StrToStr(this->GetTabFret()).c_str();
        wroteAttribute = true;
    }
    if (this->HasTabString()) {
        element.append_attribute("tab.string") = StrToStr(this->GetTabString()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttNoteGesTablature::HasTabFret( )
{
    return (m_tabFret != "");
}

bool AttNoteGesTablature::HasTabString( )
{
    return (m_tabString != "");
}


/* include <atttab.string> */

//----------------------------------------------------------------------------
// AttStaffDefGesTablature
//----------------------------------------------------------------------------

AttStaffDefGesTablature::AttStaffDefGesTablature(): Att() {
    ResetStaffDefGesTablature();
}

AttStaffDefGesTablature::~AttStaffDefGesTablature() {

}

void AttStaffDefGesTablature::ResetStaffDefGesTablature() {
    m_tabStrings = "";
}

bool AttStaffDefGesTablature::ReadStaffDefGesTablature(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("tab.strings")) {
        this->SetTabStrings(StrToStr(element.attribute("tab.strings").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttStaffDefGesTablature::WriteStaffDefGesTablature(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasTabStrings()) {
        element.append_attribute("tab.strings") = StrToStr(this->GetTabStrings()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttStaffDefGesTablature::HasTabStrings( )
{
    return (m_tabStrings != "");
}


/* include <atttab.strings> */

bool Att::SetTablature( Object *element, std::string attrType, std::string attrValue )
{
    if ( (attrType == "tabFret") && dynamic_cast<AttNoteGesTablature*>(element) ) {
        AttNoteGesTablature *att = dynamic_cast<AttNoteGesTablature*>(element);
        att->SetTabFret(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "tabString") && dynamic_cast<AttNoteGesTablature*>(element) ) {
        AttNoteGesTablature *att = dynamic_cast<AttNoteGesTablature*>(element);
        att->SetTabString(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "tabStrings") && dynamic_cast<AttStaffDefGesTablature*>(element) ) {
        AttStaffDefGesTablature *att = dynamic_cast<AttStaffDefGesTablature*>(element);
        att->SetTabStrings(att->StrToStr(attrValue));
    return true;
    }

    return false;
}


} // vrv namespace
    