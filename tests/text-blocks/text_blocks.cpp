#include "toolkit.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

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

} // namespace

int main(int argc, char **argv)
{
    if (argc != 5) return 2;

    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath("../data");

    bool ok = true;
    ok &= Expect(toolkit.LoadFile(argv[1]), "text-block fixture did not load");

    const std::string mei = toolkit.GetMEI();
    ok &= Expect(mei.find("<lg") != std::string::npos, "<lg> was discarded during import/export");
    ok &= Expect(CountOccurrences(mei, "<l ") == 2, "expected two exported <l> elements");
    ok &= Expect(CountOccurrences(mei, "<stack ") == 54, "expected 54 exported <stack> elements");
    ok &= Expect(CountOccurrences(mei, "tusk-syl-verse-2-line-") == 92,
        "expected exactly 92 extracted later-verse syllables to survive export");
    ok &= Expect(CountVersesWithNumber(mei, "1") > 0, "score lyric line 1 was not retained");
    ok &= Expect(CountVersesWithNumber(mei, "2") > 0, "score lyric line 2 was not retained");
    ok &= Expect(CountVersesWithNumber(mei, "3") == 0, "extracted lyric line 3 remained on the score");
    ok &= Expect(CountVersesWithNumber(mei, "4") == 0, "extracted lyric line 4 remained on the score");
    ok &= Expect(mei.find("corresp=\"#tusk-harm-20\"") != std::string::npos, "stack @corresp was not preserved");
    ok &= Expect(mei.find("staff=\"1\"") != std::string::npos, "line-group @staff was not preserved");
    ok &= Expect(mei.find("place=\"below\"") != std::string::npos, "line-group @place was not preserved");

    const std::string svg = RenderAllPages(toolkit);
    ok &= Expect(svg.find("tusk-div-later-verse-2") != std::string::npos, "text div was not rendered");
    ok &= Expect(svg.find("tusk-lg-verse-2") != std::string::npos, "line group was not rendered");
    ok &= Expect(svg.find("tusk-stack-verse-2-line-1-001") != std::string::npos, "stack was not rendered");
    ok &= Expect(
        svg.find("tusk-note-1") < svg.find("tusk-div-later-verse-2"), "later-verse div did not render after the score");
    int divPage = 0;
    for (int page = 1; page <= toolkit.GetPageCount(); ++page) {
        const std::string pageSvg = toolkit.RenderToSVG(page);
        if (pageSvg.find("tusk-div-later-verse-2") == std::string::npos) continue;
        divPage = page;
        ok &= Expect(pageSvg.find("tusk-stack-verse-2-line-1-001") != std::string::npos,
            "the beginning of the later-verse div was split from its div");
        ok &= Expect(pageSvg.find("tusk-stack-verse-2-line-2-046") != std::string::npos,
            "the end of the later-verse div was split onto another page");
    }
    ok &= Expect(divPage > 1, "the later-verse div was not cast off after the score page");
    const std::string stackAttributes = toolkit.GetElementAttr("tusk-stack-verse-2-line-1-001");
    ok &= Expect(stackAttributes.find("#tusk-harm-20") != std::string::npos,
        "GetElementAttr did not expose text-flow linking attributes");

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
    ok &= Expect(coreMei.find("corresp=\"#harm-1\"") != std::string::npos, "linking @corresp did not round-trip");
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
    ok &= Expect(coreSvg.find("id=\"joined-last-connector\"") == std::string::npos,
        "an ordinary intra-word syllable boundary rendered an unnecessary hyphen");
    ok &= Expect(coreSvg.find("id=\"wide-last-connector\"") != std::string::npos,
        "a harmony-created syllable gap did not render its semantic hyphen");
    ok &= Expect(CountTranslateRowsBetween(coreSvg, "flow-paragraph", "nested-div") >= 3,
        "paragraph did not wrap or honor its hard break");
    ok &= Expect(FirstTranslateYAfter(coreSvg, "stack-left") < FirstTranslateYAfter(coreSvg, "syl-when"),
        "stack chord row was not rendered above its syllable row");

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

    return ok ? 0 : 1;
}
