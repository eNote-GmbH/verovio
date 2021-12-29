/////////////////////////////////////////////////////////////////////////////
// Name:        smufl_font.h
// Author:      Andriy Makarchuk
// Created:     2021
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __SMUFL_FONT_H__
#define __SMUFL_FONT_H__

#include <string>

namespace vrv {

class SmuflFont {
public:
    SmuflFont();
    ~SmuflFont();

    bool Load(const std::string &fontName);
private:
    //
};

} // namespace vrv

#endif // __SMUFL_FONT_H__