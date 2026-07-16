/////////////////////////////////////////////////////////////////////////////
// Name:        textflowlayout.h
// Author:      Verovio contributors
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_TEXT_FLOW_LAYOUT_H__
#define __VRV_TEXT_FLOW_LAYOUT_H__

#include "devicecontextbase.h"

#include <cstddef>
#include <string>
#include <vector>

namespace vrv {

class DeviceContext;
class Doc;
class Object;
class Syl;

struct TextFlowItemMetrics {
    int width = 0;
    int gapBefore = 0;
    int rowCount = 1;
    bool hardBreak = false;
};

struct TextFlowPlacedItem {
    size_t item = 0;
    int x = 0;
};

struct TextFlowRow {
    std::vector<TextFlowPlacedItem> items;
    int width = 0;
    int rowCount = 1;
};

struct TextFlowSegment {
    Object *object = nullptr;
    std::u32string text;
    bool textOverride = false;
    int width = 0;
};

struct TextFlowStackRow {
    std::vector<TextFlowSegment> segments;
    int width = 0;
    int x = 0;
};

struct TextFlowUnit {
    Object *object = nullptr;
    std::u32string text;
    bool textOverride = false;
    bool prefixHyphen = false;
    Syl *syl = nullptr;
    std::vector<TextFlowStackRow> stackRows;
    TextFlowItemMetrics metrics;
};

struct TextFlowLayoutResult {
    Object *block = nullptr;
    std::vector<TextFlowUnit> units;
    std::vector<TextFlowRow> rows;
    FontInfo font;
    int width = 0;
    int height = 0;
    int lineHeight = 0;
};

/**
 * Complete inline layout for one text-flow block. It owns whitespace
 * normalization, semantic syllable spacing, stack partitioning and alignment
 * metrics, inherited rend measurement, wrapping, and row-height calculation.
 */
class TextFlowLayout {
public:
    TextFlowLayout(Doc *doc, DeviceContext *deviceContext, const FontInfo &baseFont, int availableWidth, int lineHeight,
        int staffSize = 100);

    TextFlowLayoutResult Layout(Object *block) const;

    /** Exposed for small deterministic unit tests of the line breaker. */
    static std::vector<TextFlowRow> Wrap(const std::vector<TextFlowItemMetrics> &items, int availableWidth);

private:
    std::u32string GetText(Object *object) const;
    int MeasureText(const std::u32string &text, const FontInfo &font) const;
    int MeasureObject(Object *object, const FontInfo &inheritedFont, int inheritedPointSize) const;
    FontInfo GetStyledFont(Object *object, const FontInfo &inheritedFont, int inheritedPointSize) const;
    FontInfo GetBlockFont(Object *block) const;
    bool PreservesWhitespace(const Object *object) const;
    Syl *GetSyl(Object *object) const;
    std::vector<TextFlowStackRow> BuildStackRows(Object *stack) const;
    int PositionStackRows(Object *stack, std::vector<TextFlowStackRow> &rows) const;
    int GetRightDigitAnchor(const TextFlowStackRow &row) const;
    std::vector<TextFlowUnit> MakeUnits(Object *block) const;

    Doc *m_doc;
    DeviceContext *m_deviceContext;
    FontInfo m_baseFont;
    mutable FontInfo m_effectiveFont;
    int m_availableWidth;
    int m_lineHeight;
    int m_staffSize;
    mutable int m_spaceWidth;
    mutable int m_hyphenWidth;
};

} // namespace vrv

#endif
