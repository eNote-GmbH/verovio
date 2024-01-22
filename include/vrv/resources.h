/////////////////////////////////////////////////////////////////////////////
// Name:        resources.h
// Author:      David Bauer
// Created:     2022
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_RESOURCES_H__
#define __VRV_RESOURCES_H__

#include <unordered_map>

//----------------------------------------------------------------------------

#include "glyph.h"

namespace vrv {

//----------------------------------------------------------------------------
// Resources
//----------------------------------------------------------------------------

/**
 * This class provides resource values.
 * It manages fonts and glyph tables.
 */

class Resources {
public:
    using StyleAttributes = std::pair<data_FONTWEIGHT, data_FONTSTYLE>;
    using GlyphTable = std::unordered_map<char32_t, Glyph>;
    using GlyphNameTable = std::unordered_map<std::string, char32_t>;
    using GlyphTextMap = std::map<StyleAttributes, GlyphTable>;

    /**
     * @name Constructors, destructors, and other standard methods
     */
    ///@{
    Resources();
    virtual ~Resources() = default;
    ///@}

    /**
     * @name Setters and getters
     */
    ///@{
    static std::string GetDefaultPath() { return s_defaultPath; }
    static void SetDefaultPath(const std::string &path) { s_defaultPath = path; }

    std::string GetPath() const { return m_path; }
    void SetPath(const std::string &path) { m_path = path; }
    ///@}

    /**
     * Font initialization
     */
    ///@{
    /** Init the SMufL music and text fonts */
    bool InitFonts(const std::vector<std::string> &extraFonts, const std::string &defaultFont);
    /** Init the text font (bounding boxes and ASCII only) */
    bool InitTextFont(const std::string &fontName, const StyleAttributes &style);
    /** Select a particular font */
    bool SetCurrentFont(const std::string &fontName, bool allowLoading = false);
    std::string GetCurrentFont() const { return m_currentFontName; }
    bool IsFontLoaded(const std::string &fontName) const { return m_loadedFonts.find(fontName) != m_loadedFonts.end(); }
    ///@}

    /**
     * Retrieving glyphs
     */
    ///@{
    /** Returns the glyph (if exists) for a glyph code in the current SMuFL font */
    const Glyph *GetGlyph(char32_t smuflCode) const;
    /** Returns the glyph (if exists) for a glyph name in the current SMuFL font */
    const Glyph *GetGlyph(const std::string &smuflName) const;
    /** Returns the glyph (if exists) for a glyph name in the current SMuFL font */
    char32_t GetGlyphCode(const std::string &smuflName) const;
    ///@}

    /**
     * Check if the text has any charachter that needs the smufl fallback font
     */
    bool IsSmuflFallbackNeeded(const std::u32string &text) const;

    /**
     * Text fonts
     */
    ///@{
    /** Set current text style*/
    void SelectTextFont(data_FONTWEIGHT fontWeight, data_FONTSTYLE fontStyle) const;
    /** Returns the glyph (if exists) for the text font (bounding box and ASCII only) */
    const Glyph *GetTextGlyph(char32_t code) const;
    /** Returns true if the specified font is loaded and it contains the requested glyph */
    bool FontHasGlyphAvailable(const std::string &fontName, char32_t smuflCode) const;
    ///@}

    /**
     * Static method that converts unicode music code points to SMuFL equivalent.
     * Return the parameter char if nothing can be converted.
     */
    static char32_t GetSmuflGlyphForUnicodeChar(const char32_t unicodeChar);

private:
    class LoadedFont {
    public:
        // LoadedFont() {};
        LoadedFont(
            const std::string &name, const std::string &path, const GlyphTable &glyphTable, bool useFallback = true)
            : m_name(name), m_path(path), m_glyphTable(glyphTable), m_useFallback(useFallback){};
        ~LoadedFont(){};
        const std::string GetName() const { return m_name; };
        const std::string GetPath() const { return m_path; };
        const GlyphTable &GetGlyphTable() const { return m_glyphTable; };
        bool useFallback() const { return m_useFallback; };

    private:
        std::string m_name;
        /** The path to the resources directory (e.g., for the svg/ subdirectory with fonts as XML */
        std::string m_path;
        /** The loaded SMuFL font */
        GlyphTable m_glyphTable;
        /** If the font have a fallback when a glyph is not present **/
        const bool m_useFallback;
    };

    bool LoadFont(const std::string &fontName, bool withFallback = true, bool buildNameTable = false);
    const GlyphTable &GetCurrentGlyphTable() const { return m_loadedFonts.at(m_currentFontName).GetGlyphTable(); };

    std::string m_path;
    std::string m_defaultFontName;
    std::map<std::string, LoadedFont> m_loadedFonts;
    std::string m_currentFontName;
    /** A text font used for bounding box calculations */
    GlyphTextMap m_textFont;
    mutable StyleAttributes m_currentStyle;
    /**
     * A map of glyph name / code
     */
    GlyphNameTable m_glyphNameTable;

    //----------------//
    // Static members //
    //----------------//

    /** The default path to the resources directory (e.g., for the svg/ subdirectory with fonts as XML */
    static thread_local std::string s_defaultPath;

    /** The default font style */
    static const StyleAttributes k_defaultStyle;
};

} // namespace vrv

#endif
