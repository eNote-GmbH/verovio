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

} // namespace

int main(int argc, char **argv)
{
    if (argc != 3) return 2;

    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(argv[2]);
    bool ok = Expect(toolkit.SetOptions(R"({"svgBoundingBoxes":true})"), "bounding-box option was rejected");
    ok &= Expect(toolkit.LoadFile(argv[1]), "beat-slash fixture did not load");
    const std::string svg = toolkit.RenderToSVG(1);

    // SVG coordinates grow downwards: the melody beam clears the head of the beat slash in the other layer, but is
    // not lifted above its stem and articulations
    const double beamY = StemEndY(svg, "melody-first");
    const double slashHeadTop = BoundingBoxTop(svg, "slash");
    const double slashStemTop = StemEndY(svg, "slash");
    ok &= Expect((beamY > 0.0) && (slashHeadTop > 0.0) && (slashStemTop > 0.0), "the fixture was not rendered");
    ok &= Expect(beamY < slashHeadTop, "the melody beam collides with the beat-slash head of the other layer");
    ok &= Expect(beamY > slashStemTop, "the melody beam was lifted above the beat-slash stem of the other layer");

    return ok ? 0 : 1;
}
