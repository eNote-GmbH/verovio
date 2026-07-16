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
#include "rend.h"
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

Syl *TextFlowLayout::GetSyl(Object *object) const
{
    if (object->Is(SYL)) return vrv_cast<Syl *>(object);
    if (object->Is(STACK)) return vrv_cast<Syl *>(object->FindDescendantByType(SYL));
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
    std::vector<TextFlowUnit> units;
    Syl *previousSyl = nullptr;
    bool pendingSpace = false;

    const auto addSemanticUnit = [&](Object *object) {
        TextFlowUnit unit;
        unit.object = object;
        unit.syl = this->GetSyl(object);
        if (object->Is(STACK)) {
            unit.stackRows = this->BuildStackRows(object);
            unit.metrics.width = this->PositionStackRows(object, unit.stackRows);
            unit.metrics.rowCount = std::max(1, static_cast<int>(unit.stackRows.size()));
        }
        else {
            unit.metrics.width = this->MeasureObject(object, m_effectiveFont, m_effectiveFont.GetPointSize());
        }

        if (previousSyl && unit.syl) {
            const bool hyphen = (previousSyl->GetCon() == sylLog_CON_d)
                || (previousSyl->GetWordpos() == sylLog_WORDPOS_i) || (previousSyl->GetWordpos() == sylLog_WORDPOS_m);
            unit.metrics.gapBefore = hyphen ? m_hyphenWidth : m_spaceWidth;
            unit.prefixHyphen = hyphen;
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
        for (Object *child : parent->GetChildren()) {
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

    std::vector<TextFlowItemMetrics> metrics;
    metrics.reserve(result.units.size());
    for (const TextFlowUnit &unit : result.units) metrics.push_back(unit.metrics);
    result.rows = TextFlowLayout::Wrap(metrics, m_availableWidth);
    for (const TextFlowRow &row : result.rows) {
        result.width = std::max(result.width, row.width);
        result.height += std::max(1, row.rowCount) * result.lineHeight;
    }
    return result;
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
