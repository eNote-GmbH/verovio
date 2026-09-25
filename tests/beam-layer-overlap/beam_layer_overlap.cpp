#include "toolkit.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

bool Expect(bool condition, const std::string &message)
{
    if (condition) return true;
    std::cerr << message << '\n';
    return false;
}

// The end point (y) of the stem path drawn for the element with the given id
double StemEndY(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"" + id + "\"");
    if (position == std::string::npos) return -1.0;
    position = svg.find("class=\"stem\"", position);
    if (position == std::string::npos) return -1.0;
    position = svg.find(" L", svg.find("<path d=\"M", position));
    if (position == std::string::npos) return -1.0;
    char *end = nullptr;
    std::strtod(svg.c_str() + position + 2, &end);
    return std::strtod(end, nullptr);
}

// The top (y) of the bounding box drawn for the element with the given id
double BoundingBoxTop(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"bbox-" + id + "\"");
    if (position == std::string::npos) return -1.0;
    position = svg.find(" y=\"", position);
    if (position == std::string::npos) return -1.0;
    return std::strtod(svg.c_str() + position + 4, nullptr);
}

// The bottom (y) of the bounding box drawn for the element with the given id
double BoundingBoxBottom(const std::string &svg, const std::string &id)
{
    size_t position = svg.find("id=\"bbox-" + id + "\"");
    if (position == std::string::npos) return -1.0;
    position = svg.find(" y=\"", position);
    if (position == std::string::npos) return -1.0;
    const double y = std::strtod(svg.c_str() + position + 4, nullptr);
    position = svg.find(" height=\"", position);
    if (position == std::string::npos) return -1.0;
    return y + std::strtod(svg.c_str() + position + 9, nullptr);
}

std::string RenderWithBoundingBoxes(const char *file, const char *resourcePath, bool &ok)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    ok &= Expect(toolkit.SetOptions(R"({"svgBoundingBoxes":true})"), "bounding-box option was rejected");
    ok &= Expect(toolkit.LoadFile(file), std::string("fixture did not load: ") + file);
    return toolkit.RenderToSVG(1);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 4) return 2;

    bool ok = true;
    const std::string svg = RenderWithBoundingBoxes(argv[1], argv[3], ok);

    // SVG coordinates grow downwards: the melody beam clears the head of the beat slash in the other layer, but is
    // not lifted above its stem and articulations
    const double beamY = StemEndY(svg, "melody-first");
    const double slashHeadTop = BoundingBoxTop(svg, "slash");
    const double slashStemTop = StemEndY(svg, "slash");
    ok &= Expect((beamY > 0.0) && (slashHeadTop > 0.0) && (slashStemTop > 0.0), "the fixture was not rendered");
    ok &= Expect(beamY < slashHeadTop, "the melody beam collides with the beat-slash head of the other layer");
    ok &= Expect(beamY > slashStemTop, "the melody beam was lifted above the beat-slash stem of the other layer");

    // A rest of the beat-slash layer above the melody moves out of the way of its beam instead of lifting the beam
    const std::string restSvg = RenderWithBoundingBoxes(argv[2], argv[3], ok);
    const double restBottom = BoundingBoxBottom(restSvg, "slash-rest");
    const double restBeamY = StemEndY(restSvg, "melody-first");
    ok &= Expect((restBottom > 0.0) && (restBeamY > 0.0), "the rest fixture was not rendered");
    ok &= Expect(restBottom <= restBeamY, "the melody beam was lifted above a rest of the other layer");

    return ok ? 0 : 1;
}
