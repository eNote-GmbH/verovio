/////////////////////////////////////////////////////////////////////////////
// Name:        textflowlayout.cpp
// Author:      Verovio contributors
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "textflowlayout.h"

#include <algorithm>
#include <cmath>
#include <functional>

#include "devicecontext.h"
#include "div.h"
#include "doc.h"
#include "editorial.h"
#include "fig.h"
#include "harm.h"
#include "ptr.h"
#include "rend.h"
#include "svg.h"
#include "syl.h"
#include "text.h"
#include "textflow.h"
#include "vrv.h"

namespace vrv {

TextFlowLayout::TextFlowLayout(
    Doc *doc, DeviceContext *deviceContext, const FontInfo &baseFont, int availableWidth, int lineHeight, int staffSize)
    : m_doc(doc)
    , m_deviceContext(deviceContext)
    , m_baseFont(baseFont)
    , m_effectiveFont(baseFont)
    , m_availableWidth(availableWidth)
    , m_lineHeight(lineHeight)
    , m_staffSize(staffSize)
    , m_spaceWidth(1)
    , m_hyphenWidth(1)
{
    m_spaceWidth = std::max(1, this->MeasureText(U" ", m_baseFont));
    m_hyphenWidth = std::max(1, this->MeasureText(U"-", m_baseFont));
}

std::u32string TextFlowLayout::GetText(Object *object) const
{
    if (object->Is(TEXT)) return vrv_cast<Text *>(object)->GetText();
    if (Harm *harm = this->GetHarm(object)) return harm->GetTextContent();
    std::u32string value;
    for (Object *child : object->GetChildren()) value += this->GetText(child);
    return value;
}

int TextFlowLayout::MeasureText(const std::u32string &text, const FontInfo &font) const
{
    FontInfo measuredFont = font;
    m_deviceContext->SetFont(&measuredFont);
    TextExtend extent;
    m_deviceContext->GetTextExtent(text, &extent, true);
    m_deviceContext->ResetFont();
    return extent.m_width;
}

FontInfo TextFlowLayout::GetStyledFont(Object *object, const FontInfo &inheritedFont, int inheritedPointSize) const
{
    FontInfo font = inheritedFont;
    AttTypography *typography = dynamic_cast<AttTypography *>(object);
    if (!typography) return font;

    if (typography->HasFontname())
        font.SetFaceName(typography->GetFontname().c_str());
    else if (typography->HasFontfam())
        font.SetFaceName(typography->GetFontfam().c_str());

    if (typography->HasFontsize()) {
        data_FONTSIZE *fontSize = typography->GetFontsizeAlternate();
        if (fontSize->GetType() == FONTSIZE_fontSizeNumeric) {
            if (fontSize->GetFontSizeNumericType() == FONTSIZENUMERIC_vu) {
                font.SetPointSize(
                    static_cast<int>(std::lround(fontSize->GetFontSizeNumeric() * m_doc->GetDrawingUnit(m_staffSize))));
            }
            else {
                constexpr double millimetersPerInch = 25.4;
                constexpr double pointsPerInch = 72.0;
                constexpr double drawingUnitsPerMillimeter = 10.0 * DEFINITION_FACTOR;
                font.SetPointSize(static_cast<int>(std::lround(
                    fontSize->GetFontSizeNumeric() * millimetersPerInch * drawingUnitsPerMillimeter / pointsPerInch)));
            }
        }
        else if (fontSize->GetType() == FONTSIZE_term) {
            font.SetPointSize(inheritedPointSize * fontSize->GetPercentForTerm() / 100);
        }
        else if (fontSize->GetType() == FONTSIZE_percent) {
            font.SetPointSize(inheritedPointSize * fontSize->GetPercent() / 100);
        }
    }
    if (typography->HasFontstyle()) font.SetStyle(typography->GetFontstyle());
    if (typography->HasFontweight()) font.SetWeight(typography->GetFontweight());
    if (typography->HasLetterspacing()) {
        font.SetLetterSpacing(typography->GetLetterspacing() * m_doc->GetDrawingUnit(m_staffSize));
    }
    Rend *rend = dynamic_cast<Rend *>(object);
    if (rend && ((rend->GetRend() == TEXTRENDITION_sup) || (rend->GetRend() == TEXTRENDITION_sub))) {
        font.SetSupSubScript(true);
        font.SetPointSize(font.GetPointSize() * SUPER_SCRIPT_FACTOR);
    }
    return font;
}

FontInfo TextFlowLayout::GetBlockFont(Object *block) const
{
    std::vector<Object *> ancestors;
    for (Object *current = block; current; current = current->GetParent()) ancestors.push_back(current);
    std::reverse(ancestors.begin(), ancestors.end());

    FontInfo font = m_baseFont;
    int pointSize = font.GetPointSize();
    for (Object *object : ancestors) {
        font = this->GetStyledFont(object, font, pointSize);
        if (font.GetPointSize() > 0) pointSize = font.GetPointSize();
    }
    return font;
}

int TextFlowLayout::MeasureObject(Object *object, const FontInfo &inheritedFont, int inheritedPointSize) const
{
    if (object->Is(TEXT)) return this->MeasureText(vrv_cast<Text *>(object)->GetText(), inheritedFont);

    if (Harm *harm = this->GetHarm(object)) {
        int width = 0;
        for (Object *child : harm->GetChildren())
            width += this->MeasureObject(child, inheritedFont, inheritedPointSize);
        return width;
    }

    FontInfo font = this->GetStyledFont(object, inheritedFont, inheritedPointSize);
    const int pointSize = (font.GetPointSize() > 0) ? font.GetPointSize() : inheritedPointSize;
    int width = 0;
    for (Object *child : object->GetChildren()) width += this->MeasureObject(child, font, pointSize);
    return width;
}

bool TextFlowLayout::PreservesWhitespace(const Object *object) const
{
    for (const Object *current = object; current; current = current->GetParent()) {
        const AttWhitespace *whitespace = dynamic_cast<const AttWhitespace *>(current);
        if (whitespace && whitespace->HasSpace()) return whitespace->GetSpace() == "preserve";
    }
    return false;
}

Harm *TextFlowLayout::GetHarm(Object *object) const
{
    if (!object || !object->Is(PTR)) return nullptr;
    Ptr *ptr = vrv_cast<Ptr *>(object);
    return dynamic_cast<Harm *>(ptr->GetTargetObject(m_doc));
}

Syl *TextFlowLayout::GetSyl(Object *object) const
{
    if (object->Is(SYL)) return vrv_cast<Syl *>(object);
    if (object->Is(STACK)) return vrv_cast<Syl *>(object->FindDescendantByType(SYL));
    return nullptr;
}

Stack *TextFlowLayout::GetStack(Object *object) const
{
    if (object->Is(STACK)) return vrv_cast<Stack *>(object);
    if (object->Is(SYL)) return vrv_cast<Stack *>(object->FindDescendantByType(STACK));
    return nullptr;
}

std::vector<TextFlowStackRow> TextFlowLayout::BuildStackRows(Object *object) const
{
    Stack *stack = vrv_cast<Stack *>(object);
    std::vector<TextFlowStackRow> rows(1);
    const std::u32string delimiter = UTF8to32(stack->GetDelimiter());
    for (Object *child : stack->GetChildren()) {
        if (child->Is(TEXT) && !delimiter.empty()) {
            std::u32string remaining = vrv_cast<Text *>(child)->GetText();
            size_t position = 0;
            while ((position = remaining.find(delimiter)) != std::u32string::npos) {
                if (position > 0) {
                    const std::u32string text = remaining.substr(0, position);
                    rows.back().segments.push_back({ child, text, true, this->MeasureText(text, m_effectiveFont) });
                }
                rows.emplace_back();
                remaining.erase(0, position + delimiter.size());
            }
            if (!remaining.empty()) {
                rows.back().segments.push_back(
                    { child, remaining, true, this->MeasureText(remaining, m_effectiveFont) });
            }
        }
        else {
            rows.back().segments.push_back(
                { child, U"", false, this->MeasureObject(child, m_effectiveFont, m_effectiveFont.GetPointSize()) });
        }
    }
    return rows;
}

int TextFlowLayout::GetRightDigitAnchor(const TextFlowStackRow &row) const
{
    int precedingWidth = 0;
    int anchor = -1;
    for (const TextFlowSegment &segment : row.segments) {
        const std::u32string text = segment.textOverride ? segment.text : this->GetText(segment.object);
        const size_t digit = text.find_last_of(U"0123456789");
        if (digit != std::u32string::npos) {
            if (digit + 1 == text.size()) {
                anchor = precedingWidth + segment.width;
            }
            else {
                FontInfo font = segment.textOverride
                    ? m_effectiveFont
                    : this->GetStyledFont(segment.object, m_effectiveFont, m_effectiveFont.GetPointSize());
                anchor = precedingWidth + this->MeasureText(text.substr(0, digit + 1), font);
            }
        }
        precedingWidth += segment.width;
    }
    return (anchor >= 0) ? anchor : row.width;
}

int TextFlowLayout::PositionStackRows(Object *object, std::vector<TextFlowStackRow> &rows) const
{
    Stack *stack = vrv_cast<Stack *>(object);
    int cellWidth = 0;
    for (TextFlowStackRow &row : rows) {
        for (const TextFlowSegment &segment : row.segments) row.width += segment.width;
        cellWidth = std::max(cellWidth, row.width);
    }

    if (stack->GetTextFlowAlignment() == TextFlowStackAlignment::RightDigit) {
        std::vector<int> anchors;
        int targetAnchor = 0;
        for (const TextFlowStackRow &row : rows) {
            anchors.push_back(this->GetRightDigitAnchor(row));
            targetAnchor = std::max(targetAnchor, anchors.back());
        }
        cellWidth = 0;
        for (size_t i = 0; i < rows.size(); ++i) {
            rows[i].x = targetAnchor - anchors[i];
            cellWidth = std::max(cellWidth, rows[i].x + rows[i].width);
        }
        return cellWidth;
    }

    for (TextFlowStackRow &row : rows) {
        switch (stack->GetTextFlowAlignment()) {
            case TextFlowStackAlignment::Left: row.x = 0; break;
            case TextFlowStackAlignment::Right: row.x = cellWidth - row.width; break;
            case TextFlowStackAlignment::Center: row.x = (cellWidth - row.width) / 2; break;
            case TextFlowStackAlignment::RightDigit: break;
        }
    }
    return cellWidth;
}

std::vector<TextFlowUnit> TextFlowLayout::MakeUnits(Object *block) const
{
    return this->MakeUnits(block, block->GetChildren());
}

std::vector<TextFlowUnit> TextFlowLayout::MakeUnits(Object *block, const std::vector<Object *> &rootChildren) const
{
    std::vector<TextFlowUnit> units;
    Syl *previousSyl = nullptr;
    bool pendingSpace = false;

    const auto addSemanticUnit = [&](Object *object) {
        TextFlowUnit unit;
        unit.object = object;
        unit.syl = this->GetSyl(object);
        unit.stack = this->GetStack(object);
        if (unit.stack) {
            unit.stackRows = this->BuildStackRows(unit.stack);
            unit.metrics.width = this->PositionStackRows(unit.stack, unit.stackRows);
            unit.metrics.rowCount = std::max(1, static_cast<int>(unit.stackRows.size()));
            for (const TextFlowStackRow &row : unit.stackRows) {
                int segmentX = row.x;
                for (const TextFlowSegment &segment : row.segments) {
                    if (unit.syl
                        && ((segment.object == unit.syl) || (segment.object->FindDescendantByType(SYL) == unit.syl))) {
                        unit.lyricX = segmentX;
                        unit.lyricWidth
                            = this->MeasureObject(unit.syl, m_effectiveFont, m_effectiveFont.GetPointSize());
                    }
                    segmentX += segment.width;
                }
            }
            if (unit.syl == object && !unit.stackRows.empty()) {
                const TextFlowStackRow &lyricRow = unit.stackRows.back();
                unit.lyricX = lyricRow.x;
                unit.lyricWidth = lyricRow.width;
            }
        }
        else {
            unit.metrics.width = this->MeasureObject(object, m_effectiveFont, m_effectiveFont.GetPointSize());
            if (unit.syl) unit.lyricWidth = unit.metrics.width;
        }

        if (previousSyl && unit.syl) {
            unit.joinsPrevious = (previousSyl->GetCon() == sylLog_CON_d)
                || (previousSyl->GetWordpos() == sylLog_WORDPOS_i) || (previousSyl->GetWordpos() == sylLog_WORDPOS_m);
            unit.metrics.gapBefore = unit.joinsPrevious ? 0 : m_spaceWidth;
        }
        else if (pendingSpace) {
            unit.metrics.gapBefore = m_spaceWidth;
        }
        units.push_back(std::move(unit));
        previousSyl = units.back().syl;
        pendingSpace = false;
    };

    const auto addTextUnit = [&](Object *textObject, const std::u32string &text, int gapBefore) {
        TextFlowUnit unit;
        unit.object = textObject;
        unit.text = text;
        unit.textOverride = true;
        unit.metrics.width = this->MeasureText(text, m_effectiveFont);
        unit.metrics.gapBefore = gapBefore;
        units.push_back(std::move(unit));
    };

    std::function<void(Object *)> addChildren;
    addChildren = [&](Object *parent) {
        const std::vector<Object *> &children = (parent == block) ? rootChildren : parent->GetChildren();
        for (Object *child : children) {
            if (child->Is(LB)) {
                TextFlowUnit unit;
                unit.object = child;
                unit.metrics.hardBreak = true;
                units.push_back(std::move(unit));
                previousSyl = nullptr;
                pendingSpace = false;
            }
            else if (child->Is(TEXT)) {
                std::u32string value = vrv_cast<Text *>(child)->GetText();
                if (this->PreservesWhitespace(child)) {
                    std::replace(value.begin(), value.end(), U'\t', U' ');
                    size_t begin = 0;
                    while (begin <= value.size()) {
                        const size_t end = value.find(U'\n', begin);
                        const std::u32string line = value.substr(begin, end - begin);
                        if (!line.empty()) addTextUnit(child, line, 0);
                        if (end == std::u32string::npos) break;
                        TextFlowUnit unit;
                        unit.object = child;
                        unit.metrics.hardBreak = true;
                        units.push_back(std::move(unit));
                        begin = end + 1;
                    }
                    pendingSpace = false;
                    previousSyl = nullptr;
                    continue;
                }

                std::u32string word;
                for (char32_t character : value) {
                    const bool whitespace
                        = (character == U' ') || (character == U'\t') || (character == U'\n') || (character == U'\r');
                    if (whitespace) {
                        if (!word.empty()) {
                            addTextUnit(child, word, pendingSpace ? m_spaceWidth : 0);
                            word.clear();
                        }
                        pendingSpace = true;
                        previousSyl = nullptr;
                    }
                    else {
                        word.push_back(character);
                    }
                }
                if (!word.empty()) {
                    addTextUnit(child, word, pendingSpace ? m_spaceWidth : 0);
                    pendingSpace = false;
                    previousSyl = nullptr;
                }
            }
            else if (child->Is(SYL) || child->Is(STACK) || child->IsTextElement()) {
                addSemanticUnit(child);
            }
            else if (child->IsEditorialElement()) {
                addChildren(child);
            }
        }
    };
    addChildren(block);
    return units;
}

TextFlowLayoutResult TextFlowLayout::Layout(Object *block) const
{
    TextFlowLayoutResult result;
    result.block = block;
    m_effectiveFont = this->GetBlockFont(block);
    m_spaceWidth = std::max(1, this->MeasureText(U" ", m_effectiveFont));
    m_hyphenWidth = std::max(1, this->MeasureText(U"-", m_effectiveFont));
    result.font = m_effectiveFont;
    const int styledLineHeight = m_doc->GetTextLineHeight(&m_effectiveFont, false);
    result.lineHeight = (styledLineHeight > 0) ? styledLineHeight : std::max(1, m_lineHeight);
    result.units = this->MakeUnits(block);

    result.rows = this->WrapUnits(result.units);
    result.connectors = this->PositionConnectors(result.units, result.rows);
    for (const TextFlowRow &row : result.rows) {
        result.width = std::max(result.width, row.width);
        result.height += std::max(1, row.rowCount) * result.lineHeight;
    }
    return result;
}

TextFlowLayoutResult TextFlowLayout::LayoutInline(Object *block, const std::vector<Object *> &children) const
{
    TextFlowLayoutResult result;
    result.block = block;
    m_effectiveFont = this->GetBlockFont(block);
    m_spaceWidth = std::max(1, this->MeasureText(U" ", m_effectiveFont));
    m_hyphenWidth = std::max(1, this->MeasureText(U"-", m_effectiveFont));
    result.font = m_effectiveFont;
    const int styledLineHeight = m_doc->GetTextLineHeight(&m_effectiveFont, false);
    result.lineHeight = (styledLineHeight > 0) ? styledLineHeight : std::max(1, m_lineHeight);
    result.units = this->MakeUnits(block, children);
    result.rows = this->WrapUnits(result.units);
    result.connectors = this->PositionConnectors(result.units, result.rows);
    for (const TextFlowRow &row : result.rows) {
        result.width = std::max(result.width, row.width);
        result.height += std::max(1, row.rowCount) * result.lineHeight;
    }
    return result;
}

TextFlowLayoutNode TextFlowLayout::LayoutFlowNode(Object *object, int width, bool bold) const
{
    TextFlowLayoutNode node;
    node.object = object;
    node.width = width;
    node.bold = bold;

    FontInfo baseFont = m_baseFont;
    if (bold) baseFont.SetWeight(FONTWEIGHT_bold);
    TextFlowLayout nestedLayout(m_doc, m_deviceContext, baseFont, width, m_lineHeight, m_staffSize);

    if (object->Is(TABLE)) {
        node.kind = TextFlowLayoutNodeKind::Table;
        Table *table = vrv_cast<Table *>(object);
        TextFlowTableLayoutResult tableLayout = nestedLayout.LayoutTable(table);
        if (tableLayout.caption) {
            TextFlowLayoutNode caption = nestedLayout.LayoutFlowNode(tableLayout.caption, width, bold);
            tableLayout.captionHeight = caption.height;
            node.children.push_back(std::move(caption));
        }

        std::vector<TextFlowLayoutNode> cellContents;
        cellContents.reserve(tableLayout.cells.size());
        for (TextFlowTableCellLayout &cell : tableLayout.cells) {
            TextFlowLayoutNode content = nestedLayout.LayoutFlowNode(cell.cell, cell.width, bold || cell.cell->Is(TH));
            cell.preferredHeight = content.height;
            cellContents.push_back(std::move(content));
        }
        nestedLayout.ResolveTableHeights(tableLayout);

        std::vector<TableRow *> rows;
        for (Object *child : table->GetChildren()) {
            if (!child) continue;
            if (child->Is(TR)) rows.push_back(vrv_cast<TableRow *>(child));
        }
        int rowY = tableLayout.gridY;
        for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
            TextFlowLayoutNode rowNode;
            rowNode.object = rows[rowIndex];
            rowNode.kind = TextFlowLayoutNodeKind::Row;
            rowNode.y = rowY;
            rowNode.width = width;
            rowNode.height = tableLayout.rowHeights[rowIndex];
            for (size_t cellIndex = 0; cellIndex < tableLayout.cells.size(); ++cellIndex) {
                const TextFlowTableCellLayout &cell = tableLayout.cells[cellIndex];
                if (cell.row != static_cast<int>(rowIndex)) continue;
                TextFlowLayoutNode cellNode = std::move(cellContents[cellIndex]);
                cellNode.kind = TextFlowLayoutNodeKind::Cell;
                cellNode.x = cell.x;
                cellNode.y = cell.y - rowY;
                cellNode.width = cell.width;
                cellNode.height = cell.height;
                rowNode.children.push_back(std::move(cellNode));
            }
            node.children.push_back(std::move(rowNode));
            rowY += tableLayout.rowHeights[rowIndex] + tableLayout.gutter;
        }
        node.height = tableLayout.height;
        return node;
    }

    if (object->Is(FIG)) {
        node.kind = TextFlowLayoutNodeKind::Figure;
        if (Svg *svg = vrv_cast<Svg *>(object->FindDescendantByType(SVG))) {
            node.width = svg->GetWidth();
            node.height = svg->GetHeight();
        }
        return node;
    }
    const bool phrase = object->IsAnyOf(std::array{ HEAD, P, L, REND, CAPTION });
    if (phrase) {
        std::vector<Object *> inlineChildren;
        int cursorY = 0;
        bool interrupted = false;
        const auto flush = [&]() {
            if (inlineChildren.empty()) return;
            TextFlowLayoutNode fragment;
            fragment.kind = TextFlowLayoutNodeKind::Phrase;
            fragment.phrase = nestedLayout.LayoutInline(object, inlineChildren);
            fragment.y = cursorY;
            fragment.width = fragment.phrase.width;
            fragment.height = fragment.phrase.height;
            cursorY += fragment.height;
            node.children.push_back(std::move(fragment));
            inlineChildren.clear();
        };
        for (Object *child : object->GetChildren()) {
            if (!child) continue;
            if (child->Is(TABLE)) {
                interrupted = true;
                flush();
                TextFlowLayoutNode tableNode = nestedLayout.LayoutFlowNode(child, width, bold);
                tableNode.y = cursorY;
                cursorY += tableNode.height;
                node.children.push_back(std::move(tableNode));
            }
            else {
                inlineChildren.push_back(child);
            }
        }
        flush();
        if (!interrupted) {
            node.kind = TextFlowLayoutNodeKind::Phrase;
            node.phrase = nestedLayout.Layout(object);
            node.children.clear();
            node.width = node.phrase.width;
            node.height = node.phrase.height;
        }
        else {
            node.kind = TextFlowLayoutNodeKind::Flow;
            node.height = std::max(m_lineHeight, cursorY);
        }
        return node;
    }

    node.kind = TextFlowLayoutNodeKind::Flow;
    int cursorY = 0;
    std::vector<Object *> inlineChildren;
    const auto flush = [&]() {
        if (!inlineChildren.empty()) {
            TextFlowLayoutNode fragment;
            fragment.kind = TextFlowLayoutNodeKind::Phrase;
            fragment.phrase = nestedLayout.LayoutInline(object, inlineChildren);
            fragment.y = cursorY;
            fragment.width = fragment.phrase.width;
            fragment.height = fragment.phrase.height;
            cursorY += fragment.height;
            node.children.push_back(std::move(fragment));
            inlineChildren.clear();
        }
    };
    for (Object *child : object->GetChildren()) {
        if (!child) continue;
        if (!child->IsAnyOf(std::array{ DIV, HEAD, P, LG, L, TABLE, FIG })) {
            inlineChildren.push_back(child);
            continue;
        }
        flush();
        TextFlowLayoutNode childNode = nestedLayout.LayoutFlowNode(child, width, bold);
        childNode.y = cursorY;
        cursorY += childNode.height;
        node.children.push_back(std::move(childNode));
    }
    flush();
    node.height = std::max(m_lineHeight, cursorY);
    for (const TextFlowLayoutNode &child : node.children) node.width = std::max(node.width, child.x + child.width);
    return node;
}

TextFlowDocumentLayoutResult TextFlowLayout::LayoutFlow(Object *root) const
{
    TextFlowDocumentLayoutResult result;
    result.root = root;
    result.availableWidth = m_availableWidth;
    result.layout = this->LayoutFlowNode(root, m_availableWidth, false);
    result.width = result.layout.width;
    result.height = result.layout.height;
    return result;
}

TextFlowTableLayoutResult TextFlowLayout::LayoutTable(Table *table) const
{
    TextFlowTableLayoutResult result;
    result.table = table;
    result.width = std::max(0, m_availableWidth);
    result.gutter = std::max(1, m_baseFont.GetPointSize());

    std::vector<TableRow *> rows;
    for (Object *child : table->GetChildren()) {
        if (child->Is(CAPTION) && !result.caption) result.caption = vrv_cast<TableCaption *>(child);
        if (child->Is(TR)) rows.push_back(vrv_cast<TableRow *>(child));
    }
    std::vector<std::vector<bool>> occupied(rows.size());
    for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        int column = 0;
        for (Object *child : rows[rowIndex]->GetChildren()) {
            TableCell *cell = dynamic_cast<TableCell *>(child);
            if (!cell) continue;
            const int colspan = std::max(1, cell->GetColspan());
            const auto rangeIsFree = [&](int start) {
                for (int offset = 0; offset < colspan; ++offset) {
                    const int candidate = start + offset;
                    if ((candidate < static_cast<int>(occupied[rowIndex].size())) && occupied[rowIndex][candidate]) {
                        return false;
                    }
                }
                return true;
            };
            while (!rangeIsFree(column)) ++column;
            int rowspan = std::max(1, cell->GetRowspan());
            if (rowIndex + rowspan > rows.size()) {
                LogWarning("Table cell '%s' rowspan exceeds the final row; clamping it", cell->GetID().c_str());
                rowspan = std::max(1, static_cast<int>(rows.size() - rowIndex));
            }
            for (size_t r = rowIndex; r < rowIndex + rowspan; ++r) {
                if (occupied[r].size() < static_cast<size_t>(column + colspan)) {
                    occupied[r].resize(column + colspan, false);
                }
                std::fill(occupied[r].begin() + column, occupied[r].begin() + column + colspan, true);
            }
            result.cells.push_back({ cell, static_cast<int>(rowIndex), column, colspan, rowspan, 0, 0, 0, 0, 0 });
            column += colspan;
        }
        result.columns = std::max(result.columns, static_cast<int>(occupied[rowIndex].size()));
    }
    result.columns = std::max(1, result.columns);
    const int totalGutters = (result.columns - 1) * result.gutter;
    const int columnWidth = std::max(0, (result.width - totalGutters) / result.columns);
    for (TextFlowTableCellLayout &cell : result.cells) {
        cell.x = cell.column * (columnWidth + result.gutter);
        cell.width = cell.colspan * columnWidth + (cell.colspan - 1) * result.gutter;
    }
    result.rowHeights.assign(rows.size(), std::max(1, m_lineHeight));
    return result;
}

void TextFlowLayout::ResolveTableHeights(TextFlowTableLayoutResult &table) const
{
    for (const TextFlowTableCellLayout &cell : table.cells) {
        if (cell.rowspan == 1) {
            table.rowHeights[cell.row] = std::max(table.rowHeights[cell.row], cell.preferredHeight);
        }
    }
    for (const TextFlowTableCellLayout &cell : table.cells) {
        if (cell.rowspan <= 1) continue;
        int available = (cell.rowspan - 1) * table.gutter;
        for (int row = cell.row; row < cell.row + cell.rowspan; ++row) available += table.rowHeights[row];
        int deficit = std::max(0, cell.preferredHeight - available);
        for (int row = 0; row < cell.rowspan && deficit > 0; ++row) {
            const int share = (deficit + cell.rowspan - row - 1) / (cell.rowspan - row);
            table.rowHeights[cell.row + row] += share;
            deficit -= share;
        }
    }

    table.gridY = table.caption ? table.captionHeight + table.gutter : 0;
    std::vector<int> rowY(table.rowHeights.size(), table.gridY);
    for (size_t row = 1; row < rowY.size(); ++row) {
        rowY[row] = rowY[row - 1] + table.rowHeights[row - 1] + table.gutter;
    }
    for (TextFlowTableCellLayout &cell : table.cells) {
        cell.y = rowY[cell.row];
        cell.height = (cell.rowspan - 1) * table.gutter;
        for (int row = cell.row; row < cell.row + cell.rowspan; ++row) cell.height += table.rowHeights[row];
    }
    table.height = table.gridY;
    for (int rowHeight : table.rowHeights) table.height += rowHeight;
    if (!table.rowHeights.empty()) table.height += (static_cast<int>(table.rowHeights.size()) - 1) * table.gutter;
}

std::vector<TextFlowRow> TextFlowLayout::WrapUnits(const std::vector<TextFlowUnit> &units) const
{
    std::vector<TextFlowRow> rows;
    TextFlowRow row;
    const auto finishRow = [&]() {
        if (!row.items.empty() || rows.empty()) rows.push_back(row);
        row = TextFlowRow();
    };
    const auto place = [&](size_t index, int gap) {
        const TextFlowUnit &unit = units[index];
        row.items.push_back({ index, row.width + gap });
        row.width += gap + unit.metrics.width;
        row.rowCount = std::max(row.rowCount, std::max(1, unit.metrics.rowCount));
    };

    size_t index = 0;
    while (index < units.size()) {
        if (units[index].metrics.hardBreak) {
            finishRow();
            ++index;
            continue;
        }

        size_t end = index + 1;
        while ((end < units.size()) && !units[end].metrics.hardBreak && units[end].joinsPrevious) ++end;
        int wordWidth = 0;
        for (size_t i = index; i < end; ++i) wordWidth += units[i].metrics.width;
        const int wordGap = row.items.empty() ? 0 : units[index].metrics.gapBefore;

        if ((m_availableWidth <= 0) || (wordWidth <= m_availableWidth)) {
            if (!row.items.empty() && (row.width + wordGap + wordWidth > m_availableWidth)) finishRow();
            for (size_t i = index; i < end; ++i) place(i, (i == index && !row.items.empty()) ? wordGap : 0);
        }
        else {
            if (!row.items.empty()) finishRow();
            for (size_t i = index; i < end; ++i) {
                const int trailingConnector = (i + 1 < end) ? m_hyphenWidth : 0;
                if (!row.items.empty() && (row.width + units[i].metrics.width + trailingConnector > m_availableWidth)) {
                    row.width += std::min(m_hyphenWidth, std::max(0, m_availableWidth - row.width));
                    finishRow();
                }
                place(i, 0);
            }
        }
        index = end;
    }
    if (!row.items.empty() || rows.empty()) finishRow();
    return rows;
}

std::vector<TextFlowConnector> TextFlowLayout::PositionConnectors(
    const std::vector<TextFlowUnit> &units, std::vector<TextFlowRow> &rows) const
{
    struct Location {
        size_t row = 0;
        int x = 0;
        bool found = false;
    };
    std::vector<Location> locations(units.size());
    for (size_t rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        for (const TextFlowPlacedItem &placed : rows[rowIndex].items) {
            locations[placed.item] = { rowIndex, placed.x, true };
        }
    }

    std::vector<TextFlowConnector> connectors;
    for (size_t i = 1; i < units.size(); ++i) {
        if (!units[i].joinsPrevious || !units[i].syl || !locations[i - 1].found || !locations[i].found) continue;
        const Location &previous = locations[i - 1];
        const Location &current = locations[i];
        if (previous.row != current.row) {
            const int previousRight = previous.x + units[i - 1].lyricX + units[i - 1].lyricWidth;
            const int connectorX = rows[previous.row].width - m_hyphenWidth;
            if (connectorX >= previousRight) connectors.push_back({ units[i].syl, previous.row, connectorX });
            continue;
        }
        const int previousRight = previous.x + units[i - 1].lyricX + units[i - 1].lyricWidth;
        const int currentLeft = current.x + units[i].lyricX;
        const int gap = currentLeft - previousRight;
        if (gap >= m_hyphenWidth) {
            connectors.push_back({ units[i].syl, current.row, previousRight + (gap - m_hyphenWidth) / 2 });
        }
    }
    return connectors;
}

std::vector<TextFlowRow> TextFlowLayout::Wrap(const std::vector<TextFlowItemMetrics> &items, int availableWidth)
{
    std::vector<TextFlowRow> rows;
    TextFlowRow row;

    const auto finishRow = [&rows, &row](bool force) {
        if (force || !row.items.empty() || rows.empty()) rows.push_back(row);
        row = TextFlowRow();
    };

    for (size_t i = 0; i < items.size(); ++i) {
        const TextFlowItemMetrics &item = items[i];
        if (item.hardBreak) {
            finishRow(true);
            continue;
        }

        const int gap = row.items.empty() ? 0 : item.gapBefore;
        if (!row.items.empty() && (availableWidth > 0) && (row.width + gap + item.width > availableWidth)) {
            finishRow(false);
        }

        const int placedGap = row.items.empty() ? 0 : item.gapBefore;
        row.items.push_back({ i, row.width + placedGap });
        row.width += placedGap + item.width;
        row.rowCount = std::max(row.rowCount, std::max(1, item.rowCount));
    }

    if (!row.items.empty() || rows.empty()) finishRow(false);
    return rows;
}

} // namespace vrv
