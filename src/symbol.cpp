/////////////////////////////////////////////////////////////////////////////
// Name:        symbol.cpp
// Author:      Laurent Pugin
// Created:     2022
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "symbol.h"

//----------------------------------------------------------------------------

#include <cassert>

//----------------------------------------------------------------------------

#include "editorial.h"
#include "resources.h"
#include "vrv.h"

namespace vrv {

//----------------------------------------------------------------------------
// Symbol
//----------------------------------------------------------------------------

static const ClassRegistrar<Symbol> s_factory("symbol", SYMBOL);

Symbol::Symbol() : Object(SYMBOL, "symbol-"), AttColor(), AttExtSym()
{
    this->Reset();

    this->RegisterAttClass(ATT_COLOR);
    this->RegisterAttClass(ATT_EXTSYM);
}

Symbol::~Symbol() {}

void Symbol::Reset()
{
    Object::Reset();

    this->ResetColor();
    this->ResetExtSym();

    m_visibility = Visible;
}

bool Symbol::IsSupportedChild(Object *child)
{
    return false;
}

wchar_t Symbol::GetSymbolGlyph() const
{
    const Resources *resources = this->GetDocResources();
    if (!resources) return 0;

    // If there is glyph.num, prioritize it
    if (this->HasGlyphNum()) {
        wchar_t code = this->GetGlyphNum();
        if (NULL != resources->GetGlyph(code)) return code;
    }
    // If there is glyph.name (second priority)
    else if (this->HasGlyphName()) {
        wchar_t code = resources->GetGlyphCode(this->GetGlyphName());
        if (NULL != resources->GetGlyph(code)) return code;
    }

    return 0;
}

} // namespace vrv
