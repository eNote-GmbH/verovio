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
class Harm;
class Object;
class Stack;
class Syl;
class Table;
class TableCaption;
class TableCell;

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
    bool joinsPrevious = false;
    Syl *syl = nullptr;
    Stack *stack = nullptr;
    int lyricX = 0;
    int lyricWidth = 0;
    std::vector<TextFlowStackRow> stackRows;
    TextFlowItemMetrics metrics;
};

struct TextFlowConnector {
    Syl *syl = nullptr;
    size_t row = 0;
    int x = 0;
};

struct TextFlowLayoutResult {
    Object *block = nullptr;
    std::vector<TextFlowUnit> units;
    std::vector<TextFlowRow> rows;
    std::vector<TextFlowConnector> connectors;
    FontInfo font;
    int width = 0;
    int height = 0;
    int lineHeight = 0;
};

struct TextFlowTableCellLayout {
    TableCell *cell = nullptr;
    int row = 0;
    int column = 0;
    int colspan = 1;
    int rowspan = 1;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int preferredHeight = 0;
};

struct TextFlowTableLayoutResult {
    Table *table = nullptr;
    TableCaption *caption = nullptr;
    std::vector<TextFlowTableCellLayout> cells;
    std::vector<int> rowHeights;
    int columns = 0;
    int gutter = 0;
    int captionHeight = 0;
    int gridY = 0;
    int width = 0;
    int height = 0;
};

enum class TextFlowLayoutNodeKind { Flow, Phrase, Table, Row, Cell, Figure };

struct TextFlowLayoutNode {
    Object *object = nullptr;
    TextFlowLayoutNodeKind kind = TextFlowLayoutNodeKind::Flow;
    TextFlowLayoutResult phrase;
    std::vector<TextFlowLayoutNode> children;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int span = 1;
    bool bold = false;
};

struct TextFlowBreakUnit {
    int y = 0;
    int height = 0;
    int keepDepth = 0;
};

struct TextFlowPageSlice {
    int startY = 0;
    int endY = 0;
    bool startsNewPage = false;
    bool forcedSplit = false;
    bool overflow = false;
};

struct TextFlowDocumentLayoutResult {
    Object *root = nullptr;
    TextFlowLayoutNode layout;
    int availableWidth = 0;
    int width = 0;
    int height = 0;
    std::vector<TextFlowBreakUnit> breakUnits;
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
    TextFlowDocumentLayoutResult LayoutFlow(Object *root) const;
    TextFlowTableLayoutResult LayoutTable(Table *table) const;
    void ResolveTableHeights(TextFlowTableLayoutResult &table) const;

    /** Exposed for small deterministic unit tests of the line breaker. */
    static std::vector<TextFlowRow> Wrap(const std::vector<TextFlowItemMetrics> &items, int availableWidth);

    /** Split vertical text-flow units into page-sized slices. */
    static std::vector<TextFlowPageSlice> Paginate(
        const std::vector<TextFlowBreakUnit> &units, int firstPageHeight, int fullPageHeight);

private:
    std::u32string GetText(Object *object) const;
    int MeasureText(const std::u32string &text, const FontInfo &font) const;
    int MeasureObject(Object *object, const FontInfo &inheritedFont, int inheritedPointSize) const;
    FontInfo GetStyledFont(Object *object, const FontInfo &inheritedFont, int inheritedPointSize) const;
    FontInfo GetBlockFont(Object *block) const;
    bool PreservesWhitespace(const Object *object) const;
    Harm *GetHarm(Object *object) const;
    Syl *GetSyl(Object *object) const;
    Stack *GetStack(Object *object) const;
    std::vector<TextFlowStackRow> BuildStackRows(Object *stack) const;
    int PositionStackRows(Object *stack, std::vector<TextFlowStackRow> &rows) const;
    int GetRightDigitAnchor(const TextFlowStackRow &row) const;
    std::vector<TextFlowUnit> MakeUnits(Object *block) const;
    std::vector<TextFlowUnit> MakeUnits(Object *block, const std::vector<Object *> &children) const;
    TextFlowLayoutResult LayoutInline(Object *block, const std::vector<Object *> &children) const;
    TextFlowLayoutNode LayoutFlowNode(Object *object, int width, bool bold) const;
    void CollectBreakUnits(const TextFlowLayoutNode &node, int parentY, std::vector<TextFlowBreakUnit> &units) const;
    std::vector<TextFlowRow> WrapUnits(const std::vector<TextFlowUnit> &units) const;
    std::vector<TextFlowConnector> PositionConnectors(
        const std::vector<TextFlowUnit> &units, std::vector<TextFlowRow> &rows) const;

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
