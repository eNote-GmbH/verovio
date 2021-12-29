/////////////////////////////////////////////////////////////////////////////
// Name:        smufl_font.cpp
// Author:      Andriy Makarchuk
// Created:     2021
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "smufl_font.h"

//----------------------------------------------------------------------------

#include <fstream>

//----------------------------------------------------------------------------

#include "glyph.h"
#include "jsonxx.h"
#include "vrv.h"

//----------------------------------------------------------------------------

#include "pugixml.hpp"


namespace vrv {

const std::string glyphJson = "glyphnames.json";
const std::string codepoint = "codepoint";

SmuflFont::SmuflFont() {}

SmuflFont::~SmuflFont() {}

bool SmuflFont::Load(const std::string &fontName)
{
    const std::string glyphPath = Resources::GetPath() + "/" + glyphJson;

    // TODO: move to separate method
    jsonxx::Object content;
    // Input is file path
    std::ifstream in(glyphPath.c_str());
    if (!in.is_open()) {
        return false;
    }
    bool result = content.parse(in);
    in.close();

    if (!result) return false;

    // TODO: move to the member variable
    // TODO: move to separate method
    std::map<std::string, Glyph> test_map;
    for (const auto &mapEntry : content.kv_map()) {
        // skip non-objects
        if (!mapEntry.second->is<jsonxx::Object>()) continue;

        const jsonxx::Object &object = mapEntry.second->get<jsonxx::Object>();
        if (object.kv_map().count(codepoint)) {
            const std::string &codestr = object.kv_map().at(codepoint)->string_value_->substr(2);
            // TODO: need to use mapEntry.first, which is actual glyphs name (for the ease of use in further code)
            test_map.insert({ codestr, Glyph(codestr) });
        }
    }

    // TODO: move to separate method
    pugi::xml_document doc;
    const std::string filename = Resources::GetPath() + "/" + fontName + ".xml";
    pugi::xml_parse_result parseResult = doc.load_file(filename.c_str());
    // TODO: add load errors
    pugi::xml_node root = doc.first_child();
    const int unitsPerEm = atoi(root.attribute("units-per-em").value());

    for (pugi::xml_node current = root.child("g"); current; current = current.next_sibling("g")) {
        Glyph *glyph = NULL;
        if (current.attribute("c")) {
            if (!test_map.count(current.attribute("c").value())) {
                //LogWarning("Glyph with code point U+%X not found.", smuflCode);
                continue;
            }
            glyph = &test_map[current.attribute("c").value()];
            glyph->SetUnitsPerEm(unitsPerEm * 10);
            double x = 0.0, y = 0.0, width = 0.0, height = 0.0;
            // Not check for missing values...
            if (current.attribute("x")) x = atof(current.attribute("x").value());
            if (current.attribute("y")) y = atof(current.attribute("y").value());
            if (current.attribute("w")) width = atof(current.attribute("w").value());
            if (current.attribute("h")) height = atof(current.attribute("h").value());
            glyph->SetBoundingBox(x, y, width, height);
            if (current.attribute("h-a-x")) glyph->SetHorizAdvX(atof(current.attribute("h-a-x").value()));
        }

        if (!glyph) continue;

        // load anchors
        pugi::xml_node anchor;
        for (anchor = current.child("a"); anchor; anchor = anchor.next_sibling("a")) {
            if (anchor.attribute("n")) {
                std::string name = std::string(anchor.attribute("n").value());
                // No check for possible x and y missing attributes - not very safe.
                glyph->SetAnchor(name, atof(anchor.attribute("x").value()), atof(anchor.attribute("y").value()));
            }
        }
    }

    int z = 3;

    return false;
}

} // namespace vrv