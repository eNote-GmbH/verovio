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

#include "atts_pagebased.h"

//----------------------------------------------------------------------------

#include "object.h"

/* #include_block */

namespace vrv {
    
//----------------------------------------------------------------------------
// AttSurface
//----------------------------------------------------------------------------

AttSurface::AttSurface(): Att() {
    ResetSurface();
}

AttSurface::~AttSurface() {

}

void AttSurface::ResetSurface() {
    m_surface = "";
}

bool AttSurface::ReadSurface(  pugi::xml_node element ) {
    bool hasAttribute = false;
    if (element.attribute("surface")) {
        this->SetSurface(StrToStr(element.attribute("surface").value()));
        hasAttribute = true;
    }
    return hasAttribute;
}

bool AttSurface::WriteSurface(  pugi::xml_node element ) {
    bool wroteAttribute = false;
    if (this->HasSurface()) {
        element.append_attribute("surface") = StrToStr(this->GetSurface()).c_str();
        wroteAttribute = true;
    }
    return wroteAttribute;
}

bool AttSurface::HasSurface( )
{
    return (m_surface != "");
}


/* include <attsurface> */

bool Att::SetPagebased( Object *element, std::string attrType, std::string attrValue )
{
    if ( (attrType == "surface") && dynamic_cast<AttSurface*>(element) ) {
        AttSurface *att = dynamic_cast<AttSurface*>(element);
        att->SetSurface(att->StrToStr(attrValue));
    return true;
    }

    return false;
}


} // vrv namespace
    