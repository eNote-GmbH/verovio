#include "textflowlayout.h"
#include "toolkit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
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

    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 7) return 2;

    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath("../data");

    bool ok = TestTextFlowPagination();
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
        "a harmony-created syllable gap did not render its semantic hyphen");
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
    ok &= Expect(encodedToolkit.GetPageCount() == 2,
        "automatic text pagination changed the explicit breaks=encoded page semantics");
    const std::string encodedSecondPage = encodedToolkit.RenderToSVG(2);
    ok &= Expect((encodedSecondPage.find("lead-first") != std::string::npos)
            && (encodedSecondPage.find("oversized-row-6") != std::string::npos)
            && (encodedSecondPage.find("pagination-row-4") != std::string::npos),
        "encoded output unexpectedly fragmented text after its explicit page break");

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
    }

    return ok ? 0 : 1;
}
