#include "textflowlayout.h"
#include "toolkit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <vector>

namespace {

bool Expect(bool condition, const std::string &message)
{
    if (condition) return true;
    std::cerr << message << '\n';
    return false;
}

size_t CountOccurrences(const std::string &value, const std::string &needle)
{
    size_t count = 0;
    size_t offset = 0;
    while ((offset = value.find(needle, offset)) != std::string::npos) {
        ++count;
        offset += needle.size();
    }
    return count;
}

size_t CountVersesWithNumber(const std::string &mei, const std::string &number)
{
    size_t count = 0;
    size_t position = 0;
    while ((position = mei.find("<verse", position)) != std::string::npos) {
        const size_t end = mei.find('>', position);
        if (end == std::string::npos) break;
        if (mei.substr(position, end - position).find("n=\"" + number + "\"") != std::string::npos) ++count;
        position = end + 1;
    }
    return count;
}

std::string RenderAllPages(vrv::Toolkit &toolkit)
{
    std::string svg;
    for (int page = 1; page <= toolkit.GetPageCount(); ++page) svg += toolkit.RenderToSVG(page);
    return svg;
}

double FirstTranslateYAfter(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"" + id + "\"");
    if (position == std::string::npos) return -1.0;
    position = svg.find("translate(", position);
    if (position == std::string::npos) return -1.0;
    position = svg.find(' ', position);
    if (position == std::string::npos) return -1.0;
    return std::strtod(svg.c_str() + position + 1, nullptr);
}

double FirstTranslateXAfter(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"" + id + "\"");
    if (position == std::string::npos) return -1.0;
    position = svg.find("translate(", position);
    if (position == std::string::npos) return -1.0;
    position += std::string("translate(").size();
    return std::strtod(svg.c_str() + position, nullptr);
}

size_t CountTranslateRowsBetween(const std::string &svg, const std::string &firstId, const std::string &nextId)
{
    const size_t begin = svg.find("id=\"" + firstId + "\"");
    const size_t end = svg.find("id=\"" + nextId + "\"", begin);
    if ((begin == std::string::npos) || (end == std::string::npos)) return 0;

    std::set<int> rows;
    size_t position = begin;
    while ((position = svg.find("translate(", position)) != std::string::npos && position < end) {
        position = svg.find(' ', position);
        if ((position == std::string::npos) || (position >= end)) break;
        rows.insert(static_cast<int>(std::strtod(svg.c_str() + position + 1, nullptr)));
    }
    return rows.size();
}

std::string SvgGroup(const std::string &svg, const std::string &id)
{
    const size_t idPosition = svg.find("id=\"" + id + "\"");
    if (idPosition == std::string::npos) return {};
    const size_t begin = svg.rfind("<g", idPosition);
    if (begin == std::string::npos) return {};

    int depth = 0;
    size_t position = begin;
    while (position < svg.size()) {
        const size_t open = svg.find("<g", position);
        const size_t close = svg.find("</g>", position);
        if (close == std::string::npos) return {};
        if ((open != std::string::npos) && (open < close)) {
            ++depth;
            position = open + 2;
        }
        else {
            --depth;
            position = close + 4;
            if (depth == 0) return svg.substr(begin, position - begin);
        }
    }
    return {};
}

double FirstRectangleX(const std::string &svg, const std::string &id)
{
    const std::string group = SvgGroup(svg, id);
    size_t position = group.find("<rect ");
    if (position == std::string::npos) return -1.0;
    position = group.find(" x=\"", position);
    if (position == std::string::npos) return -1.0;
    position += std::string(" x=\"").size();
    return std::strtod(group.c_str() + position, nullptr);
}

std::vector<std::string> GlyphReferences(const std::string &svg, const std::string &id)
{
    const std::string group = SvgGroup(svg, id);
    std::vector<std::string> references;
    size_t position = 0;
    const std::string prefix = "xlink:href=\"#";
    while ((position = group.find(prefix, position)) != std::string::npos) {
        position += prefix.size();
        const size_t end = group.find('"', position);
        if (end == std::string::npos) break;
        references.push_back(group.substr(position, end - position));
        position = end + 1;
    }
    return references;
}

bool TestHarmonyPointerTransposition(const char *file, const std::string &resourcePath, const std::string &options,
    const std::string &expectedRoot, const std::string &label)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    bool ok = true;
    if (!options.empty()) ok &= Expect(toolkit.SetOptions(options), label + " options were rejected");
    ok &= Expect(toolkit.LoadFile(file), label + " fixture did not load");
    if (!ok) return false;

    const std::string mei = toolkit.GetMEI();
    const size_t id = mei.find("xml:id=\"harm-1\"");
    const size_t begin = mei.rfind("<harm", id);
    const size_t end = mei.find("</harm>", id);
    const std::string harm = ((begin != std::string::npos) && (end != std::string::npos))
        ? mei.substr(begin, end + std::string("</harm>").size() - begin)
        : std::string();
    ok &= Expect(harm.find(">" + expectedRoot) != std::string::npos,
        label + " did not produce the expected referenced harmony " + expectedRoot);
    ok &= Expect(mei.find("target=\"#harm-1\"") != std::string::npos, label + " lost the harmony pointer target");

    const std::string svg = RenderAllPages(toolkit);
    const std::vector<std::string> source = GlyphReferences(svg, "harm-1");
    const std::vector<std::string> pointer = GlyphReferences(svg, "ptr-harm-1");
    ok &= Expect(!pointer.empty(), label + " rendered no glyphs through the harmony pointer");
    ok &= Expect(source == pointer, label + " rendered different glyphs for the harmony and its pointer");
    return ok;
}

bool TestTextFlowPagination()
{
    using vrv::TextFlowBreakUnit;
    using vrv::TextFlowLayout;
    bool ok = true;

    const auto ordinary = TextFlowLayout::Paginate(
        { TextFlowBreakUnit{ 0, 40, 0 }, TextFlowBreakUnit{ 40, 40, 0 }, TextFlowBreakUnit{ 80, 40, 0 } }, 70, 100);
    ok &= Expect((ordinary.size() == 2) && (ordinary[0].startY == 0) && (ordinary[0].endY == 40)
            && !ordinary[0].startsNewPage && (ordinary[1].startY == 40) && (ordinary[1].endY == 120)
            && ordinary[1].startsNewPage,
        "ordinary text rows were not split at the available-height boundary");

    const auto keep
        = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 40, 0 }, TextFlowBreakUnit{ 40, 40, 1 } }, 50, 100);
    ok &= Expect((keep.size() == 1) && keep[0].startsNewPage && (keep[0].startY == 0) && (keep[0].endY == 80),
        "an unbreakable group was not moved intact to a fresh page");

    const auto tallFirstPage
        = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 60, 0 }, TextFlowBreakUnit{ 60, 60, 1 } }, 140, 100);
    ok &= Expect((tallFirstPage.size() == 1) && !tallFirstPage[0].startsNewPage && !tallFirstPage[0].forcedSplit
            && (tallFirstPage[0].endY == 120),
        "a keep group that fits a taller first page was split unnecessarily");

    const auto nestedKeep
        = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 30, 0 }, TextFlowBreakUnit{ 30, 30, 1 },
                                       TextFlowBreakUnit{ 60, 30, 2 }, TextFlowBreakUnit{ 90, 20, 0 } },
            50, 60);
    ok &= Expect((nestedKeep.size() == 3) && nestedKeep[0].startsNewPage && nestedKeep[0].forcedSplit
            && (nestedKeep[0].startY == 0) && (nestedKeep[0].endY == 30) && nestedKeep[1].startsNewPage
            && nestedKeep[1].forcedSplit && (nestedKeep[1].startY == 30) && (nestedKeep[1].endY == 90)
            && nestedKeep[2].startsNewPage && (nestedKeep[2].startY == 90) && (nestedKeep[2].endY == 110),
        "an oversized outer keep group did not preserve its nested keep group");

    const auto atomicOverflow = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 120, 0 } }, 30, 100);
    ok &= Expect((atomicOverflow.size() == 1) && atomicOverflow[0].startsNewPage && atomicOverflow[0].overflow
            && !atomicOverflow[0].forcedSplit,
        "an oversized atomic unit did not use the documented overflow fallback");

    const auto explicitBreak
        = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 30, 0, 0 }, TextFlowBreakUnit{ 30, 30, 1, 1 } }, 100, 100);
    ok &= Expect((explicitBreak.size() == 2) && !explicitBreak[0].startsNewPage && explicitBreak[1].startsNewPage
            && (explicitBreak[0].endY == explicitBreak[1].startY),
        "an explicit text-flow page break did not override an unbreakable boundary");

    const auto leadingBreakOnOccupiedPage
        = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 30, 0, 1 } }, 100, 100, true);
    ok &= Expect((leadingBreakOnOccupiedPage.size() == 1) && leadingBreakOnOccupiedPage[0].startsNewPage,
        "a leading text-flow page break did not leave an already occupied page");

    const auto leadingBreakOnEmptyPage
        = TextFlowLayout::Paginate({ TextFlowBreakUnit{ 0, 30, 0, 1 } }, 100, 100, false);
    ok &= Expect((leadingBreakOnEmptyPage.size() == 1) && !leadingBreakOnEmptyPage[0].startsNewPage,
        "a leading text-flow page break created a spurious empty page");

    return ok;
}

// The horizontal extent (left, right) of the first bounding box drawn after the element with the given id
std::pair<double, double> FirstBoundingBoxX(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"" + id + "\"");
    if (position == std::string::npos) return { -1.0, -1.0 };
    position = svg.find("<rect x=\"", position);
    if (position == std::string::npos) return { -1.0, -1.0 };
    const double x = std::strtod(svg.c_str() + position + 9, nullptr);
    position = svg.find(" width=\"", position);
    if (position == std::string::npos) return { -1.0, -1.0 };
    return { x, x + std::strtod(svg.c_str() + position + 8, nullptr) };
}

// The vertical extent (top, bottom) of the first bounding box drawn after the element with the given id
std::pair<double, double> FirstBoundingBoxY(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"" + id + "\"");
    if (position == std::string::npos) return { -1.0, -1.0 };
    position = svg.find("<rect x=\"", position);
    if (position == std::string::npos) return { -1.0, -1.0 };
    position = svg.find(" y=\"", position);
    if (position == std::string::npos) return { -1.0, -1.0 };
    const double y = std::strtod(svg.c_str() + position + 4, nullptr);
    position = svg.find(" height=\"", position);
    if (position == std::string::npos) return { -1.0, -1.0 };
    return { y, y + std::strtod(svg.c_str() + position + 9, nullptr) };
}

// The x positions of the glyphs drawn within the element with the given id
std::vector<double> GlyphXs(const std::string &svg, const std::string &id)
{
    const std::string group = SvgGroup(svg, id);
    std::vector<double> xs;
    size_t position = 0;
    while ((position = group.find("translate(", position)) != std::string::npos) {
        position += std::string("translate(").size();
        xs.push_back(std::strtod(group.c_str() + position, nullptr));
    }
    return xs;
}

// The scale factors of the glyphs drawn within the element with the given id
std::vector<double> GlyphScales(const std::string &svg, const std::string &id)
{
    const std::string group = SvgGroup(svg, id);
    std::vector<double> scales;
    size_t position = 0;
    while ((position = group.find("scale(", position)) != std::string::npos) {
        position += std::string("scale(").size();
        scales.push_back(std::strtod(group.c_str() + position, nullptr));
    }
    return scales;
}

bool TestInterruptedWordHyphens(const char *file, const std::string &resourcePath)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    bool ok = Expect(toolkit.SetOptions(R"({"svgBoundingBoxes":true})"), "bounding-box option was rejected");
    ok &= Expect(toolkit.LoadFile(file), "core text-flow fixture did not load");
    const std::string svg = RenderAllPages(toolkit);

    // A chord without lyrics between two syllables of a word: the hyphens span the gap between the syllables
    const double firstRight = FirstBoundingBoxX(svg, "interrupted-first").second;
    const double lastLeft = FirstBoundingBoxX(svg, "interrupted-last").first;
    const std::vector<double> hyphens = GlyphXs(svg, "interrupted-last-connector");
    ok &= Expect(hyphens.size() > 1, "the gap left by a chord within a word was not filled with several hyphens");
    if (hyphens.size() < 2) return false;
    // With evenly distributed hyphens, the margins before the first and after the last add up to their spacing,
    // independently of the hyphen width
    const double spacing = hyphens[1] - hyphens[0];
    const double margins = (hyphens.front() - firstRight) + (lastLeft - hyphens.back());
    ok &= Expect((hyphens.front() > firstRight) && (hyphens.back() < lastLeft) && (std::abs(margins - spacing) <= 3.0),
        "the hyphens were not distributed between the syllables around a chord");

    // A word split across two lines of a stanza ends the first line with a hyphen
    const std::vector<double> trailing = GlyphXs(svg, "split-first-trailing-connector");
    ok &= Expect((trailing.size() == 1) && (trailing.front() > FirstBoundingBoxX(svg, "split-first").second),
        "a word continuing on the next line did not end its line with a hyphen");
    ok &= Expect(svg.find("id=\"split-last-connector\"") == std::string::npos,
        "the continuation of a split word was preceded by a hyphen on its own line");

    // A line grows with larger text within it instead of assuming the height of the block font
    const double descenderBottom = FirstBoundingBoxY(svg, "descender-syl").second;
    const double largeTop = FirstBoundingBoxY(svg, "large-syl").first;
    ok &= Expect((descenderBottom > 0.0) && (largeTop > 0.0) && (descenderBottom <= largeTop),
        "larger text within a line collided with the line above");

    // Hyphens are drawn in the font of the syllable before them
    const std::vector<double> syllableScales = GlyphScales(svg, "large-split");
    const std::vector<double> hyphenScales = GlyphScales(svg, "large-split-trailing-connector");
    ok &= Expect(!syllableScales.empty() && (hyphenScales.size() == 1)
            && (std::abs(hyphenScales.front() - syllableScales.front()) < 1e-6),
        "a hyphen was not drawn in the font of the syllable before it");
    return ok;
}

std::string ScoreDefTextSizeMei(const std::string &scoreDefAttributes)
{
    return R"(<?xml version="1.0" encoding="UTF-8"?>
<mei xmlns="http://www.music-encoding.org/ns/mei" meiversion="5.1">
<meiHead><fileDesc><titleStmt><title>Text sizes</title></titleStmt><pubStmt/></fileDesc></meiHead>
<music><body><mdiv><score>
<scoreDef )"
        + scoreDefAttributes
        + R"(><staffGrp><staffDef n="1" lines="5" clef.shape="G" clef.line="2"/></staffGrp></scoreDef>
<section><measure n="1"><staff n="1"><layer n="1">
<note xml:id="size-note" dur="1" oct="4" pname="c"><verse n="1"><syl xml:id="size-syl">Ly</syl></verse>
<verse n="2"><syl xml:id="size-pt-syl" fontsize="12pt">Ly</syl></verse></note>
</layer></staff>
<harm xml:id="size-harm" staff="1" startid="#size-note">C</harm>
<reh xml:id="size-reh" staff="1" tstamp="1">A</reh>
<tempo xml:id="size-tempo" staff="1" tstamp="1">Tempo</tempo>
<dir xml:id="size-dir" staff="1" tstamp="1" place="below">dir</dir>
</measure></section></score></mdiv></body></music></mei>)";
}

bool TestScoreDefTextSize(const std::string &resourcePath)
{
    const auto render = [&](const std::string &attributes) {
        vrv::Toolkit toolkit(false);
        toolkit.SetResourcePath(resourcePath);
        toolkit.LoadData(ScoreDefTextSizeMei(attributes));
        return toolkit.RenderToSVG(1);
    };
    const std::string plain = render("");
    const std::string sized = render(R"(text.size="16pt" lyric.size="8pt")");
    const auto scale = [](const std::string &svg, const std::string &id) {
        const std::vector<double> scales = GlyphScales(svg, id);
        return scales.empty() ? 0.0 : scales.front();
    };

    // The syllable with an absolute size of 12pt is the reference for the sizes in points
    const double reference = scale(sized, "size-pt-syl");
    bool ok = Expect(reference > 0.0, "the text-size fixture was not rendered");
    ok &= Expect(
        std::abs(reference - scale(plain, "size-pt-syl")) < 1e-6, "syl@fontsize in points depended on the lyric size");
    // Before, a size in points was taken as a size in drawing units, i.e., about a tenth of the intended size
    ok &= Expect(
        scale(plain, "size-pt-syl") > 0.5 * scale(plain, "size-syl"), "syl@fontsize in points was not converted");
    for (const std::string id : { "size-harm", "size-reh", "size-tempo", "size-dir" }) {
        ok &= Expect(
            std::abs(scale(sized, id) / reference - 16.0 / 12.0) < 0.01, "scoreDef@text.size was not applied to " + id);
    }
    ok &= Expect(std::abs(scale(sized, "size-syl") / reference - 8.0 / 12.0) < 0.01,
        "scoreDef@lyric.size was not applied to lyrics");
    return ok;
}

struct TextFlowSpacingMetrics {
    double chordLane = 0.0;
    double lyricLine = 0.0;
    double tableStanza = 0.0;
    double siblingStanza = 0.0;
    double nextBlock = 0.0;
    double firstBlock = 0.0;
};

TextFlowSpacingMetrics MeasureTextFlowSpacing(
    const char *file, const std::string &resourcePath, const std::string &options)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    toolkit.SetOptions(options);
    toolkit.LoadFile(file);
    const std::string svg = RenderAllPages(toolkit);

    const double chordY = FirstTranslateYAfter(svg, "stanza-1-chord");
    const double lyricY = FirstTranslateYAfter(svg, "stanza-1-line-1-lyric");
    TextFlowSpacingMetrics metrics;
    metrics.chordLane = lyricY - chordY;
    metrics.lyricLine = FirstTranslateYAfter(svg, "stanza-1-line-2-lyric") - lyricY;
    metrics.tableStanza
        = FirstTranslateYAfter(svg, "stanza-2-lyric") - FirstTranslateYAfter(svg, "stanza-1-line-2-lyric");
    metrics.siblingStanza = FirstTranslateYAfter(svg, "stanza-4-lyric") - FirstTranslateYAfter(svg, "stanza-3-lyric");
    metrics.nextBlock = FirstTranslateYAfter(svg, "block-lyric") - FirstTranslateYAfter(svg, "stanza-4-lyric");
    metrics.firstBlock = FirstTranslateYAfter(svg, "stanza-1-head");
    return metrics;
}

bool TestTextFlowSpacing(const char *file, const std::string &resourcePath)
{
    const TextFlowSpacingMetrics none = MeasureTextFlowSpacing(file, resourcePath,
        R"({"textFlowChordLaneSpacing":0,"textFlowLineSpacing":0,"textFlowStanzaSpacing":0,"textFlowScoreMargin":0,
            "textFlowBlockSpacing":0})");
    const TextFlowSpacingMetrics spaced = MeasureTextFlowSpacing(file, resourcePath,
        R"({"textFlowChordLaneSpacing":0.5,"textFlowLineSpacing":1,"textFlowStanzaSpacing":2,"textFlowScoreMargin":3,
            "textFlowBlockSpacing":1.5})");

    // Without chord-lane spacing, stacked lanes are exactly one text line apart
    const double lineHeight = none.chordLane;
    const auto added = [&](double spacedValue, double noneValue, double factor) {
        return std::abs((spacedValue - noneValue) - factor * lineHeight) <= 2.0;
    };

    bool ok = Expect(lineHeight > 0.0, "the chord lane was not rendered above its lyric line");
    ok &= Expect(std::abs(none.lyricLine - lineHeight) <= 2.0, "unspaced lyric lines were not one line apart");
    ok &= Expect(added(spaced.chordLane, none.chordLane, 0.5), "textFlowChordLaneSpacing was not applied");
    ok &= Expect(added(spaced.lyricLine, none.lyricLine, 1.0), "textFlowLineSpacing was not applied");
    ok &= Expect(added(spaced.tableStanza, none.tableStanza, 2.0),
        "textFlowStanzaSpacing was not applied between stanza table rows");
    ok &= Expect(added(spaced.siblingStanza, none.siblingStanza, 2.0),
        "textFlowStanzaSpacing was not applied between sibling line groups");
    ok &= Expect(added(spaced.firstBlock, none.firstBlock, 3.0), "textFlowScoreMargin was not applied");
    ok &= Expect(
        added(spaced.nextBlock, none.nextBlock, 1.5), "textFlowBlockSpacing was not applied between text blocks");
    return ok;
}

bool TestTextFlowScale(const char *file, const std::string &resourcePath)
{
    const auto render = [&](const std::string &options) {
        vrv::Toolkit toolkit(false);
        toolkit.SetResourcePath(resourcePath);
        toolkit.SetOptions(options);
        toolkit.LoadFile(file);
        return RenderAllPages(toolkit);
    };
    const std::string plain = render(R"({"textFlowChordLaneSpacing":0})");
    const std::string scaled = render(R"({"textFlowChordLaneSpacing":0,"textFlowScale":0.8})");
    const auto scale = [](const std::string &svg, const std::string &id) {
        const std::vector<double> scales = GlyphScales(svg, id);
        return scales.empty() ? 0.0 : scales.front();
    };
    const auto lineHeight = [](const std::string &svg) {
        return FirstTranslateYAfter(svg, "stanza-1-line-1-lyric") - FirstTranslateYAfter(svg, "stanza-1-chord");
    };

    bool ok = Expect(scale(plain, "stanza-1-line-1-lyric") > 0.0, "the text-flow scale fixture was not rendered");
    ok &= Expect(std::abs(scale(scaled, "stanza-1-line-1-lyric") / scale(plain, "stanza-1-line-1-lyric") - 0.8) < 0.02,
        "textFlowScale was not applied to text-block lyrics");
    // The line height, and with it every text-flow spacing, follows the scaled font
    ok &= Expect(std::abs(lineHeight(scaled) / lineHeight(plain) - 0.8) < 0.02,
        "textFlowScale was not applied to the text-block line height");
    ok &= Expect(std::abs(scale(scaled, "harm-1") - scale(plain, "harm-1")) < 1e-6,
        "textFlowScale changed the size of text in the score");
    return ok;
}

bool TestStyledHarmonyPointer(const std::string &resourcePath)
{
    const std::string mei = R"(<?xml version="1.0" encoding="UTF-8"?>
<mei xmlns="http://www.music-encoding.org/ns/mei" meiversion="5.1">
<meiHead><fileDesc><titleStmt><title>Styled chords</title></titleStmt><pubStmt/></fileDesc></meiHead>
<music><body><mdiv><score>
<scoreDef><staffGrp><staffDef n="1" lines="5" clef.shape="G" clef.line="2"/></staffGrp></scoreDef>
<section><measure n="1"><staff n="1"><layer n="1"><note xml:id="styled-note" dur="1" oct="4" pname="c"/></layer></staff>
<harm xml:id="styled-harm" staff="1" startid="#styled-note">Dm</harm>
</measure>
<div><lg><l>
<syl><stack delim="|" align="left"><ptr xml:id="plain-ptr" target="#styled-harm"/>|Ly</stack></syl>
<syl><stack delim="|" align="left"><rend fontstyle="italic"><ptr xml:id="italic-ptr" target="#styled-harm"/></rend>|ric</stack></syl>
</l></lg></div>
</section></score></mdiv></body></music></mei>)";
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    bool ok = Expect(toolkit.LoadData(mei), "the styled chord fixture did not load");
    const std::string svg = RenderAllPages(toolkit);
    const std::vector<std::string> harm = GlyphReferences(svg, "styled-harm");
    const std::vector<std::string> italic = GlyphReferences(svg, "italic-ptr");
    ok &= Expect(
        !harm.empty() && (GlyphReferences(svg, "plain-ptr") == harm), "a chord pointer was not drawn as its harm");
    ok &= Expect(italic.size() == harm.size(), "a chord pointer within <rend> was not drawn");
    ok &= Expect(italic != harm, "a chord pointer within <rend fontstyle=\"italic\"> was not drawn in italics");
    const std::string output = toolkit.GetMEI();
    const size_t pointer = output.find("xml:id=\"italic-ptr\"");
    const size_t rend = output.rfind("<rend", pointer);
    ok &= Expect((pointer != std::string::npos) && (rend != std::string::npos)
            && (output.find("</rend>", rend) > pointer)
            && (output.substr(rend, pointer - rend).find("fontstyle=\"italic\"") != std::string::npos),
        "a chord pointer within <rend> was not preserved in the MEI output");
    return ok;
}

std::string WhitespaceMei(const std::string &separator)
{
    return R"(<?xml version="1.0" encoding="UTF-8"?>
<mei xmlns="http://www.music-encoding.org/ns/mei" meiversion="5.1">
<meiHead><fileDesc><titleStmt><title>Whitespace</title><composer>Text: A)"
        + separator + R"(Melody: B</composer></titleStmt><pubStmt/></fileDesc></meiHead>
<music><body><mdiv><score>
<scoreDef><staffGrp><staffDef n="1" lines="5" clef.shape="G" clef.line="2"/></staffGrp></scoreDef>
<section><measure n="1"><staff n="1"><layer n="1"><note dur="1" oct="4" pname="c"/></layer></staff>
<dir xml:id="whitespace-dir" staff="1" tstamp="1">first)"
        + separator + R"(second</dir>
</measure></section></score></mdiv></body></music></mei>)";
}

bool TestTextWhitespace(const std::string &resourcePath)
{
    const auto render = [&](const std::string &separator) {
        vrv::Toolkit toolkit(false);
        toolkit.SetResourcePath(resourcePath);
        toolkit.SetOptions(R"({"header":"auto"})");
        toolkit.LoadData(WhitespaceMei(separator));
        return toolkit.RenderToSVG(1);
    };
    const std::string spaced = render(" ");
    const std::string broken = render("\n        ");
    const auto pgHead = [](const std::string &svg) {
        const size_t begin = svg.find("class=\"pgHead");
        const size_t end = svg.find("class=\"system", begin);
        return (begin == std::string::npos) ? std::string() : svg.substr(begin, end - begin);
    };
    // The glyphs with their positions, without the document-specific suffix of the glyph ids
    const auto glyphs = [](const std::string &group) {
        static const std::regex suffix("-[a-z0-9]+\"");
        std::vector<std::string> uses;
        size_t position = 0;
        while ((position = group.find("<use", position)) != std::string::npos) {
            const size_t end = group.find("/>", position);
            uses.push_back(std::regex_replace(group.substr(position, end - position), suffix, "\""));
            position = end;
        }
        return uses;
    };
    bool ok = Expect(!glyphs(SvgGroup(spaced, "whitespace-dir")).empty(), "the whitespace fixture was not rendered");
    ok &= Expect(glyphs(SvgGroup(broken, "whitespace-dir")) == glyphs(SvgGroup(spaced, "whitespace-dir")),
        "a line break within a text was not drawn as a single space");
    ok &= Expect(!glyphs(pgHead(spaced)).empty() && (glyphs(pgHead(broken)) == glyphs(pgHead(spaced))),
        "a line break within a header text was not drawn as a single space");
    return ok;
}

bool TestHeaderPersons(const std::string &resourcePath)
{
    const auto render = [&](const std::string &persons) {
        vrv::Toolkit toolkit(false);
        toolkit.SetResourcePath(resourcePath);
        toolkit.SetOptions(R"({"header":"auto"})");
        toolkit.LoadData(R"(<?xml version="1.0" encoding="UTF-8"?>
<mei xmlns="http://www.music-encoding.org/ns/mei" meiversion="5.1">
<meiHead><fileDesc><titleStmt><title>Persons</title>)"
            + persons + R"(</titleStmt><pubStmt/></fileDesc></meiHead>
<music><body><mdiv><score>
<scoreDef><staffGrp><staffDef n="1" lines="5" clef.shape="G" clef.line="2"/></staffGrp></scoreDef>
<section><measure n="1"><staff n="1"><layer n="1"><note dur="1" oct="4" pname="c"/></layer></staff></measure></section>
</score></mdiv></body></music></mei>)");
        const std::string svg = toolkit.RenderToSVG(1);
        const size_t begin = svg.find("class=\"pgHead");
        const size_t end = svg.find("class=\"system", begin);
        if (begin == std::string::npos) return std::make_pair(size_t(0), size_t(0));
        const std::string pgHead = svg.substr(begin, end - begin);
        return std::make_pair(CountOccurrences(pgHead, "<use"), CountOccurrences(pgHead, "class=\"rend\""));
    };
    const auto glyphs = [&](const std::string &persons) { return render(persons).first; };
    const size_t title = glyphs("");
    const size_t composer = glyphs("<composer>C</composer>");
    bool ok = Expect(composer > title, "the composer was not drawn in the page header");
    for (const std::string person : { "arranger", "lyricist" }) {
        ok &= Expect(glyphs("<composer>C</composer><" + person + ">P</" + person + ">") > composer,
            "the " + person + " was not drawn in the page header");
    }
    ok &= Expect(glyphs(R"(<respStmt><persName role="lyricist">P</persName></respStmt>)") > title,
        "a person with a role was not drawn in the page header");
    ok &= Expect(glyphs("<respStmt><persName>P</persName></respStmt>") == title,
        "a person without a role was drawn in the page header");
    ok &= Expect(render("<composer/><respStmt><persName/></respStmt>").second == render("").second,
        "empty persons added groups to the page header");
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 8) return 2;

    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath("../data");

    bool ok = TestTextFlowPagination();
    // A page height at which the later verses start below the score and continue on the next page
    ok &= Expect(toolkit.SetOptions(R"({"pageHeight":2850})"), "text-block fixture options were rejected");
    ok &= Expect(toolkit.LoadFile(argv[1]), "text-block fixture did not load");

    const std::string mei = toolkit.GetMEI();
    ok &= Expect(mei.find("<lg") != std::string::npos, "<lg> was discarded during import/export");
    ok &= Expect(CountOccurrences(mei, "<l ") == 2, "expected two exported <l> elements");
    ok &= Expect(CountOccurrences(mei, "<stack ") == 54, "expected 54 exported <stack> elements");
    ok &= Expect(CountOccurrences(mei, "<ptr ") == 54, "expected 54 exported harmony pointers");
    ok &= Expect(CountOccurrences(mei, "tusk-syl-verse-2-line-") == 92,
        "expected exactly 92 extracted later-verse syllables to survive export");
    ok &= Expect(CountVersesWithNumber(mei, "1") > 0, "score lyric line 1 was not retained");
    ok &= Expect(CountVersesWithNumber(mei, "2") > 0, "score lyric line 2 was not retained");
    ok &= Expect(CountVersesWithNumber(mei, "3") == 0, "extracted lyric line 3 remained on the score");
    ok &= Expect(CountVersesWithNumber(mei, "4") == 0, "extracted lyric line 4 remained on the score");
    ok &= Expect(mei.find("target=\"#tusk-harm-20\"") != std::string::npos,
        "the first text-flow harmony pointer was not preserved");
    ok &= Expect(mei.find("staff=\"1\"") != std::string::npos, "line-group @staff was not preserved");
    ok &= Expect(mei.find("place=\"below\"") != std::string::npos, "line-group @place was not preserved");

    const std::string svg = RenderAllPages(toolkit);
    ok &= Expect(svg.find("tusk-div-later-verse-2") != std::string::npos, "text div was not rendered");
    ok &= Expect(svg.find("tusk-lg-verse-2") != std::string::npos, "line group was not rendered");
    ok &= Expect(svg.find("tusk-stack-verse-2-line-1-001") != std::string::npos, "stack was not rendered");
    ok &= Expect(
        svg.find("tusk-note-1") < svg.find("tusk-div-later-verse-2"), "later-verse div did not render after the score");
    int divPage = 0;
    int firstStackPage = 0;
    int lastStackPage = 0;
    for (int page = 1; page <= toolkit.GetPageCount(); ++page) {
        const std::string pageSvg = toolkit.RenderToSVG(page);
        if (!divPage && (pageSvg.find("tusk-div-later-verse-2") != std::string::npos)) divPage = page;
        if (pageSvg.find("tusk-stack-verse-2-line-1-001") != std::string::npos) firstStackPage = page;
        if (pageSvg.find("tusk-stack-verse-2-line-2-046") != std::string::npos) lastStackPage = page;
    }
    ok &= Expect(divPage > 0, "the later-verse div was not rendered on any cast-off page");
    ok &= Expect(firstStackPage == divPage, "the first text row was split from its canonical div fragment");
    ok &= Expect(lastStackPage > firstStackPage, "the overflowing later verses were not continued on another page");
    ok &= Expect(CountOccurrences(svg, "id=\"tusk-div-later-verse-2\"") == 1,
        "the canonical div SVG id was duplicated across page fragments");
    ok &= Expect(svg.find("id=\"tusk-div-later-verse-2-continuation-1\"") != std::string::npos,
        "the text continuation did not receive its stable generated SVG id");
    ok &= Expect(CountOccurrences(svg, "id=\"tusk-stack-verse-2-line-1-001\"") == 1,
        "a semantic stack SVG id was duplicated across page fragments");
    const std::string syllableAttributes = toolkit.GetElementAttr("tusk-syl-verse-2-line-1-001");
    ok &= Expect((syllableAttributes.find("#tusk-note-23") != std::string::npos)
            && (syllableAttributes.find("#tusk-harm-20") == std::string::npos),
        "the text-flow syllable did not retain only its note synchronization");

    vrv::Toolkit coreToolkit(false);
    coreToolkit.SetResourcePath("../data");
    ok &= Expect(coreToolkit.LoadFile(argv[2]), "core text-flow fixture did not load");

    const std::string coreMei = coreToolkit.GetMEI();
    ok &= Expect(coreMei.find("<head xml:id=\"flow-head\"") != std::string::npos, "head did not round-trip");
    ok &= Expect(coreMei.find("<p xml:id=\"flow-paragraph\"") != std::string::npos, "paragraph did not round-trip");
    ok &= Expect(coreMei.find("xml:space=\"preserve\"") != std::string::npos, "xml:space did not round-trip");
    ok &= Expect(coreMei.find("<div xml:id=\"nested-div\"") != std::string::npos, "nested div did not round-trip");
    ok &= Expect(coreMei.find("n=\"2\"") != std::string::npos, "common @n did not round-trip");
    ok &= Expect(coreMei.find("type=\"later-verses\"") != std::string::npos, "common @type did not round-trip");
    ok &= Expect(coreMei.find("label=\"Later verses\"") != std::string::npos, "common @label did not round-trip");
    ok &= Expect(coreMei.find("xml:lang=\"de\"") != std::string::npos, "common language did not round-trip");
    ok &= Expect(coreMei.find("rhythm=\"8 8 4\"") != std::string::npos, "line @rhythm did not round-trip");
    ok &= Expect(coreMei.find("wordpos=\"i\"") != std::string::npos, "syllable @wordpos did not round-trip");
    ok &= Expect(coreMei.find("con=\"d\"") != std::string::npos, "syllable @con did not round-trip");
    ok &= Expect(CountOccurrences(coreMei, "align=\"") == 5, "stack alignments did not round-trip");
    ok &= Expect(coreMei.find("<ptr xml:id=\"ptr-harm-1\" target=\"#harm-1\"") != std::string::npos,
        "the harmony pointer did not round-trip");
    ok &= Expect(coreMei.find("synch=\"#note-1\"") != std::string::npos, "linking @synch did not round-trip");
    ok &= Expect(coreMei.find("staff=\"1\"") != std::string::npos, "line-group @staff did not round-trip");
    ok &= Expect(coreMei.find("layer=\"1\"") != std::string::npos, "line-group @layer did not round-trip");
    ok &= Expect(coreMei.find("fontfam=\"Times\"") != std::string::npos, "typography did not round-trip");

    const std::string coreSvg = RenderAllPages(coreToolkit);
    ok &= Expect(coreSvg.find("id=\"flow-head\"") != std::string::npos, "heading was not rendered");
    ok &= Expect(coreSvg.find("id=\"nested-div\"") != std::string::npos, "nested div was not rendered");
    ok &= Expect(coreSvg.find("id=\"hard-break\"") != std::string::npos, "hard line break was not rendered");
    ok &= Expect(coreSvg.find("id=\"mixed-legacy-rend\"") != std::string::npos,
        "direct legacy rend content disappeared from a text-flow div");
    ok &= Expect(coreSvg.find("id=\"mixed-legacy-fig\"") != std::string::npos,
        "direct legacy fig content disappeared from a text-flow div");
    ok &= Expect(coreSvg.find("id=\"nested-legacy-rend\"") != std::string::npos,
        "nested legacy div content disappeared from a text-flow div");
    ok &= Expect(
        (FirstTranslateYAfter(coreSvg, "nested-paragraph") < FirstTranslateYAfter(coreSvg, "mixed-legacy-rend"))
            && (FirstTranslateYAfter(coreSvg, "mixed-legacy-rend")
                < FirstTranslateYAfter(coreSvg, "nested-legacy-rend"))
            && (FirstTranslateYAfter(coreSvg, "nested-legacy-rend") < FirstTranslateYAfter(coreSvg, "stanza-head")),
        "mixed inline and block children were not laid out in document order");
    ok &= Expect(
        coreSvg.find("id=\"chord-extension\"") != std::string::npos, "superscript chord extension was not rendered");
    ok &= Expect(coreSvg.find("id=\"ptr-harm-1\"") != std::string::npos, "the referenced harmony was not rendered");
    ok &= Expect(coreSvg.find("id=\"joined-last-connector\"") == std::string::npos,
        "an ordinary intra-word syllable boundary rendered an unnecessary hyphen");
    ok &= Expect(coreSvg.find("id=\"wide-last-connector\"") != std::string::npos,
        "a syllable pushed apart by colliding chords did not render its semantic hyphen");
    ok &= Expect(coreSvg.find("id=\"overhang-last-connector\"") == std::string::npos,
        "a chord overhanging the next syllable stretched its word apart");
    ok &= Expect(CountOccurrences(SvgGroup(coreSvg, "wide-last-connector"), "<use ") > 1,
        "a wide intra-word gap was not filled with several hyphens");
    ok &= Expect(CountTranslateRowsBetween(coreSvg, "flow-paragraph", "nested-div") >= 3,
        "paragraph did not wrap or honor its hard break");
    ok &= Expect(CountTranslateRowsBetween(coreSvg, "stack-left", "syl-hap") >= 2,
        "stack chord row was not rendered above its syllable row");

    const std::string resourcePath = "../data";
    ok &= TestHarmonyPointerTransposition(argv[2], resourcePath, "", "D", "untransposed harmony pointer");
    ok &= TestHarmonyPointerTransposition(
        argv[2], resourcePath, R"({"transpose":"M2","transposeCapo":"off"})", "E", "document transposition");
    ok &= TestHarmonyPointerTransposition(argv[2], resourcePath,
        R"({"transposeMdiv":{"core-mdiv":"M2"},"transposeCapo":"off"})", "E", "mdiv transposition");
    ok &= TestHarmonyPointerTransposition(
        argv[2], resourcePath, R"({"transposeToSoundingPitch":true})", "C", "sounding-pitch transposition");
    ok &= TestHarmonyPointerTransposition(
        argv[2], resourcePath, R"({"transpose":"M2","transposeCapo":"5"})", "D", "capo-aware transposition");

    vrv::Toolkit legacyToolkit(false);
    legacyToolkit.SetResourcePath("../data");
    ok &= Expect(legacyToolkit.LoadFile(argv[3]), "legacy div fixture did not load");
    const std::string legacyMei = legacyToolkit.GetMEI();
    ok &= Expect(
        legacyMei.find("<div xml:id=\"legacy-div\"") != std::string::npos, "legacy div did not survive import/export");
    const std::string legacySvg = RenderAllPages(legacyToolkit);
    ok &= Expect(legacySvg.find("id=\"legacy-rend\"") != std::string::npos, "legacy div/rend rendering regressed");

    vrv::Toolkit tableToolkit(false);
    tableToolkit.SetResourcePath("../data");
    ok &= Expect(tableToolkit.LoadFile(argv[4]), "text-flow table fixture did not load");
    const std::string tableMei = tableToolkit.GetMEI();
    ok &= Expect(tableMei.find("<table xml:id=\"two-column-table\"") != std::string::npos,
        "table did not survive import/export");
    ok &= Expect(tableMei.find("<caption xml:id=\"table-caption\"") != std::string::npos,
        "table caption did not survive import/export");
    ok &= Expect(tableMei.find("<th xml:id=\"heading-cell\" colspan=\"2\"") != std::string::npos,
        "table heading or colspan did not survive import/export");
    ok &= Expect(tableMei.find("<td xml:id=\"rowspan-cell\" rowspan=\"2\"") != std::string::npos,
        "table rowspan did not survive import/export");
    ok &= Expect(tableMei.find("<td xml:id=\"invalid-span\" colspan=\"0\"") != std::string::npos,
        "invalid encoded span was not preserved during round-trip");
    ok &= Expect(tableMei.find("corresp=\"#table-note\"") != std::string::npos,
        "table linking attributes did not survive import/export");

    const std::string tableSvg = RenderAllPages(tableToolkit);
    for (const std::string &id : { "two-column-table", "table-caption", "parallel-row", "left-cell", "right-cell",
             "heading-cell", "rowspan-cell", "nested-table", "nested-table-cell", "empty-row", "invalid-span",
             "overwide-word-cell", "inline-table", "inline-cell", "collision-table", "collision-colspan" }) {
        ok &= Expect(
            tableSvg.find("id=\"" + id + "\"") != std::string::npos, "table semantic object was not rendered: " + id);
    }
    const double leftX = FirstTranslateXAfter(tableSvg, "left-cell");
    const double rightX = FirstTranslateXAfter(tableSvg, "right-cell");
    ok &= Expect(
        (leftX >= 0.0) && (rightX - leftX > 1500.0), "two text cells were not placed in separate equal-width columns");
    ok &= Expect(
        std::abs(FirstTranslateYAfter(tableSvg, "left-cell") - FirstTranslateYAfter(tableSvg, "right-cell")) < 1.0,
        "parallel table cells were not top-aligned");
    ok &= Expect(FirstTranslateYAfter(tableSvg, "table-caption") < FirstTranslateYAfter(tableSvg, "left-cell"),
        "table caption did not render above the grid");
    const double beforeTableY = FirstTranslateYAfter(tableSvg, "before-inline-table");
    const double inlineTableY = FirstTranslateYAfter(tableSvg, "inline-table");
    const double afterTableY = FirstTranslateYAfter(tableSvg, "after-inline-table");
    ok &= Expect((beforeTableY >= 0.0) && (beforeTableY < inlineTableY) && (inlineTableY < afterTableY),
        "a table inside a phrase did not flush and resume the inline text in document order");
    const double splitConnectorY = FirstTranslateYAfter(tableSvg, "overwide-middle-connector");
    const double splitFirstY = FirstTranslateYAfter(tableSvg, "overwide-first");
    const double splitMiddleY = FirstTranslateYAfter(tableSvg, "overwide-middle");
    ok &= Expect((splitConnectorY >= 0.0) && (std::abs(splitConnectorY - splitFirstY) < 1.0)
            && (std::abs(splitConnectorY - splitMiddleY) > 1.0),
        "an overwide semantic word did not reserve a trailing connector on the preceding row");
    ok &= Expect(
        FirstTranslateXAfter(tableSvg, "collision-colspan") > FirstTranslateXAfter(tableSvg, "collision-rowspan"),
        "a colspan overlapped a column occupied by a rowspan");
    const std::string cellAttributes = tableToolkit.GetElementAttr("heading-cell");
    ok &= Expect(cellAttributes.find("colspan") != std::string::npos,
        "GetElementAttr did not expose table-cell span attributes");

    vrv::Toolkit enclosureToolkit(false);
    enclosureToolkit.SetResourcePath("../data");
    ok &= Expect(enclosureToolkit.LoadFile(argv[5]), "offset text-enclosure fixture did not load");
    const std::string enclosureSvg = RenderAllPages(enclosureToolkit);
    const double enclosureX = FirstRectangleX(enclosureSvg, "offset-boxed-dir");
    const double textX = FirstTranslateXAfter(enclosureSvg, "offset-boxed-rend");
    ok &= Expect((enclosureX >= 0.0) && (textX >= 0.0) && (textX > enclosureX) && ((textX - enclosureX) < 200.0),
        "@ho shifted enclosed text without shifting its enclosure");
    const double mixedEnclosureX = FirstRectangleX(enclosureSvg, "offset-mixed-dir");
    const double mixedTextX = FirstTranslateXAfter(enclosureSvg, "offset-mixed-boxed-rend");
    ok &= Expect((mixedEnclosureX >= 0.0) && (mixedTextX >= 0.0) && (mixedTextX > mixedEnclosureX)
            && ((mixedTextX - mixedEnclosureX) < 200.0),
        "@ho did not keep a mixed-rend enclosure with its enclosed text");

    vrv::Toolkit paginationToolkit(false);
    paginationToolkit.SetResourcePath("../data");
    ok &= Expect(paginationToolkit.SetOptions(R"({"pageWidth":500,"pageHeight":300})"),
        "pagination fixture options were rejected");
    ok &= Expect(paginationToolkit.LoadFile(argv[6]), "pagination fixture did not load");
    const std::string paginationMei = paginationToolkit.GetMEI();
    ok &= Expect(paginationMei.find("type=\"notice unbreakable\"") != std::string::npos,
        "the unbreakable token was not preserved alongside an existing @type token");
    ok &= Expect(paginationMei.find("type=\"translation unbreakable\"") != std::string::npos,
        "a nested unbreakable token was not preserved");
    for (const std::string &id : { "between-text-divs-pb", "div-text-pb", "head-text-pb", "paragraph-text-pb",
             "line-text-pb", "caption-text-pb", "cell-text-pb", "heading-cell-text-pb" }) {
        ok &= Expect(paginationMei.find("<pb xml:id=\"" + id + "\"") != std::string::npos,
            "a text-flow page break did not round-trip: " + id);
    }

    const auto pageContaining = [&](const std::string &id) {
        for (int page = 1; page <= paginationToolkit.GetPageCount(); ++page) {
            if (paginationToolkit.RenderToSVG(page).find("id=\"" + id + "\"") != std::string::npos) return page;
        }
        return 0;
    };
    const int leadPage = pageContaining("lead-first");
    const int keptFirstPage = pageContaining("kept-first");
    const int keptLastPage = pageContaining("kept-last");
    ok &= Expect((leadPage > 0) && (keptFirstPage > leadPage) && (keptFirstPage == keptLastPage),
        "an unbreakable div was not moved intact when it did not fit the preceding page");
    ok &= Expect((pageContaining("nested-kept-first") > 0)
            && (pageContaining("nested-kept-first") == pageContaining("nested-kept-last")),
        "a nested unbreakable paragraph was split across pages");
    ok &= Expect((pageContaining("oversized-row-1") > 0)
            && (pageContaining("oversized-row-1") < pageContaining("oversized-row-6")),
        "an oversized unbreakable div did not fall back to safe inner page boundaries");
    ok &= Expect(pageContaining("oversized-row-3") == pageContaining("oversized-row-4"),
        "the oversized-block fallback split a nested unbreakable group");
    ok &= Expect(pageContaining("pagination-caption") == pageContaining("pagination-row-1"),
        "a table caption was split from its first row");
    ok &= Expect(pageContaining("pagination-row-2") == pageContaining("pagination-row-3"),
        "rows connected by rowspan were split across pages");
    ok &= Expect(
        (pageContaining("div-pb-before") > 0) && (pageContaining("div-pb-before") < pageContaining("div-pb-after")),
        "a page break between direct div children did not force a new page");
    ok &= Expect((pageContaining("paragraph-pb-before") > 0)
            && (pageContaining("paragraph-pb-before") < pageContaining("paragraph-pb-after")),
        "a page break inside a paragraph did not force a new page");
    ok &= Expect(
        (pageContaining("head-pb-before") > 0) && (pageContaining("head-pb-before") < pageContaining("head-pb-after")),
        "a page break inside a heading did not force a new page");
    ok &= Expect(
        (pageContaining("line-pb-before") > 0) && (pageContaining("line-pb-before") < pageContaining("line-pb-after")),
        "a page break inside a verse line did not force a new page");
    ok &= Expect(pageContaining("caption-pb-before") == pageContaining("pb-table-row"),
        "a page break inside a caption split the caption from the first table row");
    ok &= Expect(pageContaining("pb-table-row") < pageContaining("cell-pb-after"),
        "a page break inside a table cell did not move its atomic row group to a new page");
    ok &= Expect(pageContaining("cell-pb-after") < pageContaining("heading-cell-pb-after"),
        "a page break inside a table heading cell did not move its atomic row group to a new page");
    ok &= Expect((pageContaining("between-text-divs-pb") == pageContaining("div-pb-before"))
            && (pageContaining("pagination-row-4") < pageContaining("div-pb-before")),
        "a page break between text divs did not force the following div onto a new page");

    const std::string paginationRoundTrip = paginationToolkit.GetMEI();
    ok &= Expect(paginationRoundTrip.find("continuation-") == std::string::npos,
        "generated continuation ids leaked into MEI export");
    ok &= Expect(CountOccurrences(paginationRoundTrip, "xml:id=\"kept-div\"") == 1,
        "the canonical unbreakable div was lost or duplicated during un-cast-off");

    vrv::Toolkit continuousToolkit(false);
    continuousToolkit.SetResourcePath("../data");
    ok &= Expect(continuousToolkit.SetOptions(R"({"pageWidth":500,"pageHeight":300,"breaks":"none"})"),
        "breaks=none options were rejected");
    ok &= Expect(continuousToolkit.LoadFile(argv[6]), "pagination fixture did not load with breaks=none");
    ok &= Expect(
        continuousToolkit.GetPageCount() == 1, "automatic text pagination changed the continuous breaks=none path");

    vrv::Toolkit encodedToolkit(false);
    encodedToolkit.SetResourcePath("../data");
    ok &= Expect(encodedToolkit.SetOptions(R"({"pageWidth":500,"pageHeight":300,"breaks":"encoded"})"),
        "breaks=encoded options were rejected");
    ok &= Expect(encodedToolkit.LoadFile(argv[6]), "pagination fixture did not load with breaks=encoded");
    ok &= Expect(encodedToolkit.GetPageCount() >= 7, "text-flow page breaks were not applied with breaks=encoded");
    const std::string encodedSecondPage = encodedToolkit.RenderToSVG(2);
    ok &= Expect((encodedSecondPage.find("lead-first") != std::string::npos)
            && (encodedSecondPage.find("oversized-row-6") != std::string::npos)
            && (encodedSecondPage.find("pagination-row-4") != std::string::npos),
        "encoded output unexpectedly fragmented text after its explicit page break");
    const auto encodedPageContaining = [&](const std::string &id) {
        for (int page = 1; page <= encodedToolkit.GetPageCount(); ++page) {
            if (encodedToolkit.RenderToSVG(page).find("id=\"" + id + "\"") != std::string::npos) return page;
        }
        return 0;
    };
    ok &= Expect(encodedPageContaining("div-pb-before") < encodedPageContaining("div-pb-after"),
        "a div-level text page break was ignored with breaks=encoded");
    ok &= Expect(encodedPageContaining("paragraph-pb-before") < encodedPageContaining("paragraph-pb-after"),
        "a paragraph text page break was ignored with breaks=encoded");
    ok &= Expect(encodedPageContaining("pagination-row-4") < encodedPageContaining("div-pb-before"),
        "a page break between text divs was ignored with breaks=encoded");

    for (const std::string &breakMode : { "line", "smart" }) {
        vrv::Toolkit automaticToolkit(false);
        automaticToolkit.SetResourcePath("../data");
        const std::string options = "{\"pageWidth\":500,\"pageHeight\":300,\"breaks\":\"" + breakMode + "\"}";
        ok &= Expect(automaticToolkit.SetOptions(options), "automatic break options were rejected: " + breakMode);
        ok &= Expect(automaticToolkit.LoadFile(argv[6]), "pagination fixture did not load with breaks=" + breakMode);
        const std::string automaticSvg = RenderAllPages(automaticToolkit);
        ok &= Expect((automaticToolkit.GetPageCount() > 2) && (automaticSvg.find("kept-div") != std::string::npos)
                && (automaticSvg.find("continuation-1") != std::string::npos),
            "text pagination was not active for breaks=" + breakMode);
        const auto automaticPageContaining = [&](const std::string &id) {
            for (int page = 1; page <= automaticToolkit.GetPageCount(); ++page) {
                if (automaticToolkit.RenderToSVG(page).find("id=\"" + id + "\"") != std::string::npos) return page;
            }
            return 0;
        };
        ok &= Expect(automaticPageContaining("pagination-row-4") < automaticPageContaining("div-pb-before"),
            "a page break between text divs was ignored with breaks=" + breakMode);
        ok &= Expect(automaticPageContaining("paragraph-pb-before") < automaticPageContaining("paragraph-pb-after"),
            "a paragraph text page break was ignored with breaks=" + breakMode);
    }

    ok &= TestTextFlowSpacing(argv[7], resourcePath);
    ok &= TestTextFlowScale(argv[7], resourcePath);
    ok &= TestInterruptedWordHyphens(argv[2], resourcePath);
    ok &= TestScoreDefTextSize(resourcePath);
    ok &= TestStyledHarmonyPointer(resourcePath);
    ok &= TestTextWhitespace(resourcePath);
    ok &= TestHeaderPersons(resourcePath);

    return ok ? 0 : 1;
}
