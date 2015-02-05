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

#include "atts_edittrans.h"

//----------------------------------------------------------------------------

#include "object.h"

/* #include_block */

namespace vrv {
    
//----------------------------------------------------------------------------
// AttAgentident
//----------------------------------------------------------------------------

AttAgentident::AttAgentident(): Att() {
    ResetAgentident();
}

AttAgentident::~AttAgentident() {

}

void AttAgentident::ResetAgentident() {
    m_agent = "";
}

bool AttAgentident::ReadAgentident(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("agent")) {
        this->SetAgent(StrToStr(element.attribute("agent").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttAgentident::WriteAgentident(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasAgent()) {
        element.append_attribute("agent") = StrToStr(this->GetAgent()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttAgentident::HasAgent( )
{
    return (m_agent != "");
}


/* include <attagent> */

//----------------------------------------------------------------------------
// AttEdit
//----------------------------------------------------------------------------

AttEdit::AttEdit(): Att() {
    ResetEdit();
}

AttEdit::~AttEdit() {

}

void AttEdit::ResetEdit() {
    m_cert = "";
    m_evidence = "";
}

bool AttEdit::ReadEdit(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("cert")) {
        this->SetCert(StrToStr(element.attribute("cert").value()));
        hasAttribute = true;
    }
    if (element.attribute("evidence")) {
        this->SetEvidence(StrToStr(element.attribute("evidence").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttEdit::WriteEdit(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasCert()) {
        element.append_attribute("cert") = StrToStr(this->GetCert()).c_str();
        wroteAttribute = true;
    }
    if (this->HasEvidence()) {
        element.append_attribute("evidence") = StrToStr(this->GetEvidence()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttEdit::HasCert( )
{
    return (m_cert != "");
}

bool AttEdit::HasEvidence( )
{
    return (m_evidence != "");
}


/* include <attevidence> */

//----------------------------------------------------------------------------
// AttExtent
//----------------------------------------------------------------------------

AttExtent::AttExtent(): Att() {
    ResetExtent();
}

AttExtent::~AttExtent() {

}

void AttExtent::ResetExtent() {
    m_extent = "";
}

bool AttExtent::ReadExtent(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("extent")) {
        this->SetExtent(StrToStr(element.attribute("extent").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttExtent::WriteExtent(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasExtent()) {
        element.append_attribute("extent") = StrToStr(this->GetExtent()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttExtent::HasExtent( )
{
    return (m_extent != "");
}


/* include <attextent> */

//----------------------------------------------------------------------------
// AttReasonident
//----------------------------------------------------------------------------

AttReasonident::AttReasonident(): Att() {
    ResetReasonident();
}

AttReasonident::~AttReasonident() {

}

void AttReasonident::ResetReasonident() {
    m_reason = "";
}

bool AttReasonident::ReadReasonident(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("reason")) {
        this->SetReason(StrToStr(element.attribute("reason").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttReasonident::WriteReasonident(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasReason()) {
        element.append_attribute("reason") = StrToStr(this->GetReason()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttReasonident::HasReason( )
{
    return (m_reason != "");
}


/* include <attreason> */

bool Att::SetEdittrans( Object *element, std::string attrType, std::string attrValue )
{
    if ( (attrType == "agent") && dynamic_cast<AttAgentident*>(element) ) {
        AttAgentident *att = dynamic_cast<AttAgentident*>(element);
        att->SetAgent(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "cert") && dynamic_cast<AttEdit*>(element) ) {
        AttEdit *att = dynamic_cast<AttEdit*>(element);
        att->SetCert(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "evidence") && dynamic_cast<AttEdit*>(element) ) {
        AttEdit *att = dynamic_cast<AttEdit*>(element);
        att->SetEvidence(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "extent") && dynamic_cast<AttExtent*>(element) ) {
        AttExtent *att = dynamic_cast<AttExtent*>(element);
        att->SetExtent(att->StrToStr(attrValue));
    return true;
    }
    if ( (attrType == "reason") && dynamic_cast<AttReasonident*>(element) ) {
        AttReasonident *att = dynamic_cast<AttReasonident*>(element);
        att->SetReason(att->StrToStr(attrValue));
    return true;
    }

    return false;
}


} // vrv namespace
    