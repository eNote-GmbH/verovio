#include "atts_shared.h"
#include "breath.h"
#include "caesura.h"
#include "cpmark.h"
#include "fermata.h"
#include "fing.h"
#include "harm.h"
#include "mordent.h"
#include "octave.h"
#include "ornam.h"
#include "repeatmark.h"
#include "scoredef.h"
#include "staffdef.h"
#include "staffgrp.h"
#include "tempo.h"
#include "toolkit.h"
#include "trill.h"
#include "turn.h"

#include "pugixml.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

bool Expect(bool condition, const std::string &message)
{
    if (condition) return true;
    std::cerr << message << '\n';
    return false;
}

bool SameOrder(const vrv::data_STAFFITEM_List &actual, const vrv::data_STAFFITEM_List &expected)
{
    return actual == expected;
}

std::string ReadFile(const std::string &path)
{
    std::ifstream input(path);
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

bool TestListParser()
{
    const std::string all
        = "accid annot artic dir dynam harm ornam sp stageDir tempo beam bend bracketSpan breath cpMark fermata "
          "fing hairpin harpPedal lv mordent octave pedal reh tie trill tuplet turn ligature";

    pugi::xml_document input;
    pugi::xml_node element = input.append_child("scoreDef");
    element.append_attribute("aboveorder").set_value((all + "\taccid").c_str());
    element.append_attribute("beloworder").set_value("dir\n  dynam\t harm");
    element.append_attribute("betweenorder").set_value("beam invalid-token tuplet");

    vrv::InstStaffItems attributes;
    bool ok = true;
    ok &= Expect(attributes.ReadStaffItems(element, false), "staff-item list attributes were not read");
    ok &= Expect(attributes.GetAboveorder().size() == 30, "not every data.STAFFITEM token or duplicate was read");
    ok &= Expect(attributes.GetAboveorder().front() == vrv::STAFFITEM_accid
            && attributes.GetAboveorder().back() == vrv::STAFFITEM_accid,
        "a duplicate valid token was not preserved");
    ok &= Expect(
        SameOrder(attributes.GetBeloworder(), { vrv::STAFFITEM_dir, vrv::STAFFITEM_dynam, vrv::STAFFITEM_harm }),
        "arbitrary XML whitespace was not accepted");
    ok &= Expect(SameOrder(attributes.GetBetweenorder(), { vrv::STAFFITEM_beam, vrv::STAFFITEM_tuplet }),
        "an invalid token was not discarded independently of valid tokens");

    pugi::xml_document output;
    pugi::xml_node written = output.append_child("scoreDef");
    ok &= Expect(attributes.WriteStaffItems(written), "staff-item list attributes were not written");
    ok &= Expect(std::string(written.attribute("aboveorder").value()) == all + " accid",
        "staff-item serialization was not whitespace-normalized or lost a duplicate");
    ok &= Expect(std::string(written.attribute("beloworder").value()) == "dir dynam harm",
        "beloworder did not serialize as a whitespace-separated list");
    ok &= Expect(std::string(written.attribute("betweenorder").value()) == "beam tuplet",
        "invalid data.STAFFITEM input was emitted again");
    return ok;
}

bool TestRegistrationCopyAndReset()
{
    vrv::ScoreDef source;
    source.SetAboveorder({ vrv::STAFFITEM_harm, vrv::STAFFITEM_harm, vrv::STAFFITEM_beam });
    source.SetBeloworder({ vrv::STAFFITEM_dir });

    bool ok = true;
    vrv::ArrayOfStrAttr attributes;
    source.GetAttributes(&attributes);
    ok &= Expect(std::find(attributes.begin(), attributes.end(),
                     std::pair<std::string, std::string>{ "aboveorder", "harm harm beam" })
            != attributes.end(),
        "ScoreDefElement did not register AttStaffItems for generic attribute access");

    vrv::ScoreDef copy;
    source.CopyAttributesTo(&copy);
    ok &= Expect(copy.GetAboveorder() == source.GetAboveorder() && copy.GetBeloworder() == source.GetBeloworder(),
        "generic scoreDef attribute copying lost staff-item orders");
    copy.Reset();
    ok &= Expect(!copy.HasAboveorder() && !copy.HasBeloworder() && !copy.HasBetweenorder(),
        "scoreDef reset retained a staff-item order");

    vrv::StaffDef staffSource;
    staffSource.SetBetweenorder({ vrv::STAFFITEM_dynam, vrv::STAFFITEM_harm });
    vrv::StaffDef staffCopy;
    staffSource.CopyAttributesTo(&staffCopy);
    ok &= Expect(staffCopy.GetBetweenorder() == staffSource.GetBetweenorder(),
        "generic staffDef attribute copying lost a staff-item order");
    return ok;
}

template <class T> bool TestVerticalGroupClass(const std::string &name)
{
    T source;
    source.SetVgrp(7);
    vrv::ArrayOfStrAttr attributes;
    source.GetAttributes(&attributes);

    bool ok = true;
    ok &= Expect(std::find(attributes.begin(), attributes.end(), std::pair<std::string, std::string>{ "vgrp", "7" })
            != attributes.end(),
        name + " did not register AttVerticalGroup");

    T copy;
    source.CopyAttributesTo(&copy);
    ok &= Expect(copy.HasVgrp() && (copy.GetVgrp() == 7), name + " did not copy @vgrp");
    copy.Reset();
    ok &= Expect(!copy.HasVgrp(), name + " retained @vgrp after reset");
    return ok;
}

bool TestVerticalGroupClasses()
{
    bool ok = true;
    ok &= TestVerticalGroupClass<vrv::Breath>("breath");
    ok &= TestVerticalGroupClass<vrv::Caesura>("caesura");
    ok &= TestVerticalGroupClass<vrv::CpMark>("cpMark");
    ok &= TestVerticalGroupClass<vrv::Fermata>("fermata");
    ok &= TestVerticalGroupClass<vrv::Fing>("fing");
    ok &= TestVerticalGroupClass<vrv::Harm>("harm");
    ok &= TestVerticalGroupClass<vrv::Mordent>("mordent");
    ok &= TestVerticalGroupClass<vrv::Octave>("octave");
    ok &= TestVerticalGroupClass<vrv::Ornam>("ornam");
    ok &= TestVerticalGroupClass<vrv::RepeatMark>("repeatMark");
    ok &= TestVerticalGroupClass<vrv::Tempo>("tempo");
    ok &= TestVerticalGroupClass<vrv::Trill>("trill");
    ok &= TestVerticalGroupClass<vrv::Turn>("turn");
    return ok;
}

bool TestResolver()
{
    vrv::ScoreDef drawing;
    drawing.SetAboveorder({ vrv::STAFFITEM_harm, vrv::STAFFITEM_dynam });
    drawing.SetBeloworder({ vrv::STAFFITEM_dir, vrv::STAFFITEM_dynam });
    drawing.SetBetweenorder({ vrv::STAFFITEM_harm, vrv::STAFFITEM_dir });

    auto *group = new vrv::StaffGrp();
    auto *first = new vrv::StaffDef();
    first->SetN(1);
    first->SetAboveorder({ vrv::STAFFITEM_dir, vrv::STAFFITEM_harm });
    auto *second = new vrv::StaffDef();
    second->SetN(2);
    group->AddChild(first);
    group->AddChild(second);
    drawing.AddChild(group);

    bool ok = true;
    ok &= Expect(
        SameOrder(drawing.GetStaffItemOrder(1, vrv::STAFFREL_above), { vrv::STAFFITEM_dir, vrv::STAFFITEM_harm }),
        "staffDef aboveorder did not override scoreDef aboveorder");
    ok &= Expect(
        SameOrder(drawing.GetStaffItemOrder(1, vrv::STAFFREL_below), { vrv::STAFFITEM_dir, vrv::STAFFITEM_dynam }),
        "an unset staffDef direction did not inherit from scoreDef");
    ok &= Expect(
        SameOrder(drawing.GetStaffItemOrder(2, vrv::STAFFREL_above), { vrv::STAFFITEM_harm, vrv::STAFFITEM_dynam }),
        "a second staff did not use the scoreDef order");

    vrv::ScoreDef laterScoreDef;
    laterScoreDef.SetBeloworder({ vrv::STAFFITEM_harm });
    drawing.ReplaceDrawingValues(&laterScoreDef);
    ok &= Expect(
        SameOrder(drawing.GetStaffItemOrder(1, vrv::STAFFREL_above), { vrv::STAFFITEM_dir, vrv::STAFFITEM_harm })
            && SameOrder(
                drawing.GetStaffItemOrder(2, vrv::STAFFREL_above), { vrv::STAFFITEM_harm, vrv::STAFFITEM_dynam })
            && SameOrder(drawing.GetStaffItemOrder(1, vrv::STAFFREL_below), { vrv::STAFFITEM_harm }),
        "a later scoreDef did not update only its explicitly set direction");

    vrv::StaffDef laterStaffDef;
    laterStaffDef.SetN(1);
    laterStaffDef.SetBetweenorder({ vrv::STAFFITEM_dynam });
    drawing.ReplaceDrawingValues(&laterStaffDef);
    ok &= Expect(SameOrder(drawing.GetStaffItemOrder(1, vrv::STAFFREL_between), { vrv::STAFFITEM_dynam })
            && SameOrder(
                drawing.GetStaffItemOrder(2, vrv::STAFFREL_between), { vrv::STAFFITEM_harm, vrv::STAFFITEM_dir }),
        "a later staffDef did not update only its staff and direction");
    return ok;
}

bool TestMEIRoundtrip(const std::string &fixture, const std::string &resourcePath)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    bool ok = true;
    ok &= Expect(toolkit.LoadFile(fixture), "staff-item order MEI fixture did not load");
    if (!ok) return false;

    pugi::xml_document document;
    const std::string mei = toolkit.GetMEI();
    ok &= Expect(document.load_string(mei.c_str()), "round-tripped MEI was not valid XML");
    const pugi::xml_node scoreDef = document.select_node("//*[local-name()='scoreDef']").node();
    const pugi::xml_node staffDef = document.select_node("//*[local-name()='staffDef' and @n='2']").node();
    ok &= Expect(std::string(scoreDef.attribute("aboveorder").value()) == "harm dynam dir"
            && std::string(scoreDef.attribute("beloworder").value()) == "dir dynam harm"
            && std::string(scoreDef.attribute("betweenorder").value()) == "harm dynam dir",
        "scoreDef staff-item orders did not survive the MEI roundtrip");
    ok &= Expect(std::string(staffDef.attribute("aboveorder").value()) == "dir dynam harm"
            && std::string(staffDef.attribute("beloworder").value()) == "harm dir"
            && std::string(staffDef.attribute("betweenorder").value()) == "dir harm",
        "staffDef staff-item orders did not survive the MEI roundtrip");
    return ok;
}

bool TestVerticalGroupRoundtrip(const std::string &fixture, const std::string &resourcePath)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    bool ok = true;
    ok &= Expect(toolkit.LoadFile(fixture), "vertical-group MEI fixture did not load");
    if (!ok) return false;

    pugi::xml_document document;
    ok &= Expect(document.load_string(toolkit.GetMEI().c_str()), "vertical-group roundtrip was not valid XML");
    for (const std::string id : { "vg-breath", "vg-caesura", "vg-cpmark", "vg-dir", "vg-dynam", "vg-fermata", "vg-fing",
             "vg-hairpin", "vg-harm", "vg-mordent", "vg-octave", "vg-ornam", "vg-pedal", "vg-repeatmark", "vg-tempo",
             "vg-trill", "vg-turn" }) {
        const std::string query = "//*[@xml:id='" + id + "']";
        const pugi::xml_node node = document.select_node(query.c_str()).node();
        ok &= Expect(node && node.attribute("vgrp"), id + " lost @vgrp in the MEI roundtrip");
    }
    return ok;
}

bool TestHistoricRehVerticalGroupRoundtrip(const std::string &fixture, const std::string &resourcePath)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    bool ok = true;
    ok &= Expect(toolkit.LoadFile(fixture), "historic reh/@vgrp fixture did not load");
    if (!ok) return false;

    pugi::xml_document document;
    ok &= Expect(document.load_string(toolkit.GetMEI().c_str()), "historic reh/@vgrp roundtrip was not valid XML");
    const pugi::xml_node reh = document.select_node("//*[@xml:id='vg-reh']").node();
    ok &= Expect(
        reh && std::string(reh.attribute("vgrp").value()) == "15", "historic reh/@vgrp compatibility was not retained");
    return ok;
}

int TextY(const pugi::xml_document &document, const std::string &id)
{
    const std::string query = "//*[@id='" + id + "']//*[local-name()='text']";
    const pugi::xml_node text = document.select_node(query.c_str()).node();
    if (text) return text.attribute("y").as_int();

    const std::string useQuery = "//*[@id='" + id
        + "']//*[local-name()='g' and contains(concat(' ', normalize-space(@class), ' '), ' text ')]"
          "//*[local-name()='use'][1]";
    const std::string transform = document.select_node(useQuery.c_str()).node().attribute("transform").value();
    const size_t separator = transform.find(' ');
    if (separator == std::string::npos) return 0;
    return std::stoi(transform.substr(separator + 1));
}

int HairpinCenterY(const pugi::xml_document &document, const std::string &id)
{
    const std::string query = "//*[@id='" + id + "']//*[local-name()='polyline']";
    std::istringstream points(document.select_node(query.c_str()).node().attribute("points").value());
    std::string first;
    std::string middle;
    std::string last;
    points >> first >> middle >> last;
    const auto y = [](const std::string &point) { return std::stoi(point.substr(point.find(',') + 1)); };
    return (y(first) + y(last)) / 2;
}

int FirstPathY(const pugi::xml_document &document, const std::string &query)
{
    const std::string path = document.select_node(query.c_str()).node().attribute("d").value();
    const size_t separator = path.find(' ');
    if (separator == std::string::npos) return 0;
    return std::stoi(path.substr(separator + 1));
}

bool LoadSVG(const std::string &fixture, const std::string &resourcePath, pugi::xml_document &document)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    if (!toolkit.LoadFile(fixture)) return false;
    return document.load_string(toolkit.RenderToSVG(1).c_str());
}

bool LoadSVGData(const std::string &mei, const std::string &resourcePath, pugi::xml_document &document)
{
    vrv::Toolkit toolkit(false);
    toolkit.SetResourcePath(resourcePath);
    if (!toolkit.LoadData(mei)) return false;
    return document.load_string(toolkit.RenderToSVG(1).c_str());
}

bool TestRendering(const std::string &orderFixture, const std::string &partialFixture,
    const std::string &defaultFixture, const std::string &verticalGroupFixture, const std::string &vgrpChangeFixture,
    const std::string &vgrpOverlapFixture, const std::string &endingFixture, const std::string &resourcePath)
{
    bool ok = true;
    pugi::xml_document ordered;
    ok &= Expect(LoadSVG(orderFixture, resourcePath, ordered), "ordered rendering fixture did not render");
    if (!ordered) return false;

    ok &= Expect(TextY(ordered, "above-harm-1") > TextY(ordered, "above-dynam-1")
            && TextY(ordered, "above-dynam-1") > TextY(ordered, "above-dir-1"),
        "aboveorder was not applied from staff-nearest to staff-farthest");
    ok &= Expect(TextY(ordered, "below-dir-1") < TextY(ordered, "below-dynam-1")
            && TextY(ordered, "below-dynam-1") < TextY(ordered, "below-harm-1"),
        "beloworder was not applied from staff-nearest to staff-farthest");
    ok &= Expect(TextY(ordered, "above-dir-2") > TextY(ordered, "above-dynam-2")
            && TextY(ordered, "above-dynam-2") > TextY(ordered, "above-harm-2"),
        "the staffDef override did not produce the opposite above order on staff 2");
    ok &= Expect(TextY(ordered, "between-harm") < TextY(ordered, "between-dynam")
            && TextY(ordered, "between-dynam") < TextY(ordered, "between-dir"),
        "betweenorder was not applied from the upper staff to the lower staff");
    ok &= Expect(TextY(ordered, "later-dir") > TextY(ordered, "later-dynam")
            && TextY(ordered, "later-dynam") > TextY(ordered, "later-harm"),
        "a later staffDef did not change the order in the following measure of the same system");
    ok &= Expect(TextY(ordered, "group-dynam") - HairpinCenterY(ordered, "group-hairpin") == 81,
        "explicitly grouped dynamics and hairpins lost their shared baseline invariant");

    pugi::xml_document partial;
    ok &= Expect(LoadSVG(partialFixture, resourcePath, partial), "partial-list rendering fixture did not render");
    if (partial) {
        ok &= Expect(TextY(partial, "partial-harm") > TextY(partial, "partial-stage-dir")
                && TextY(partial, "partial-stage-dir") > TextY(partial, "partial-dynam")
                && TextY(partial, "partial-dynam") > TextY(partial, "partial-dir"),
            "a partial order was not used as a prefix before the stable default categories");
    }

    pugi::xml_document legacy;
    ok &= Expect(LoadSVG(defaultFixture, resourcePath, legacy), "legacy rendering fixture did not render");
    if (legacy) {
        ok &= Expect(TextY(legacy, "default-harm") == 3253 && TextY(legacy, "default-dynam") == 2807
                && TextY(legacy, "default-dir") == 2721,
            "the no-attribute legacy between layout changed");

        std::string unrelatedDirection = ReadFile(defaultFixture);
        const size_t scoreDef = unrelatedDirection.find("<scoreDef") + std::string("<scoreDef").size();
        unrelatedDirection.insert(scoreDef, " aboveorder=\"harm\"");
        pugi::xml_document withAboveOrder;
        ok &= Expect(LoadSVGData(unrelatedDirection, resourcePath, withAboveOrder),
            "fixture with an unrelated aboveorder did not render");
        if (withAboveOrder) {
            ok &= Expect(TextY(withAboveOrder, "default-harm") == TextY(legacy, "default-harm")
                    && TextY(withAboveOrder, "default-dynam") == TextY(legacy, "default-dynam")
                    && TextY(withAboveOrder, "default-dir") == TextY(legacy, "default-dir"),
                "setting aboveorder changed the legacy behavior of an unset betweenorder");
        }
    }

    pugi::xml_document verticalGroups;
    ok &= Expect(LoadSVG(verticalGroupFixture, resourcePath, verticalGroups), "vertical-group fixture did not render");
    if (verticalGroups) {
        const int baseline = TextY(verticalGroups, "vg-harm");
        ok &= Expect(TextY(verticalGroups, "vg-dynam") == baseline && TextY(verticalGroups, "vg-dir") == baseline
                && TextY(verticalGroups, "vg-tempo") == baseline,
            "a cross-class explicit @vgrp did not produce one shared baseline");
    }

    pugi::xml_document vgrpChange;
    ok &= Expect(
        LoadSVG(vgrpChangeFixture, resourcePath, vgrpChange), "cross-measure vertical-group fixture did not render");
    if (vgrpChange) {
        ok &= Expect(TextY(vgrpChange, "m1-dynam") > TextY(vgrpChange, "m1-harm")
                && TextY(vgrpChange, "m1-harm") > TextY(vgrpChange, "m1-dir"),
            "a later group shift broke the first measure's staff-item order");
        ok &= Expect(TextY(vgrpChange, "m2-harm") > TextY(vgrpChange, "m2-dir")
                && TextY(vgrpChange, "m2-dir") > TextY(vgrpChange, "m2-dynam"),
            "the later staffDef order was not retained around the shared group");
        ok &= Expect(TextY(vgrpChange, "m1-dynam") == TextY(vgrpChange, "m2-dynam"),
            "cross-measure group stabilization broke the shared baseline");
    }

    pugi::xml_document vgrpOverlap;
    ok &= Expect(LoadSVG(vgrpOverlapFixture, resourcePath, vgrpOverlap),
        "overlapping cross-measure vgrp fixture did not render");
    if (vgrpOverlap) {
        const int firstHarmony = TextY(vgrpOverlap, "overlap-harm-1");
        const int secondHarmony = TextY(vgrpOverlap, "overlap-harm-2");
        const int direction = TextY(vgrpOverlap, "overlap-dir");
        ok &= Expect(firstHarmony == secondHarmony, "an overlapping later measure broke the shared vgrp baseline");
        ok &= Expect(
            direction < firstHarmony, "a later harmony moved its shared vgrp outside a staff-farther direction");
        const int staffY = FirstPathY(vgrpOverlap,
            "(//*[local-name()='g' and contains(concat(' ', normalize-space(@class), ' '), ' staff ')])[1]"
            "/*[local-name()='path'][1]");
        ok &= Expect(
            (staffY - firstHarmony) < 2500, "cross-measure vgrp stabilization created excessive staff spacing");
    }

    pugi::xml_document ending;
    ok &= Expect(LoadSVG(endingFixture, resourcePath, ending), "ordered ending fixture did not render");
    if (ending) {
        const int bracketY = FirstPathY(ending,
            "(//*[local-name()='g' and contains(concat(' ', normalize-space(@class), ' '), ' ending ')])[1]"
            "//*[local-name()='path'][1]");
        const int staffY = FirstPathY(ending,
            "(//*[local-name()='g' and contains(concat(' ', normalize-space(@class), ' '), ' staff ')])[2]"
            "/*[local-name()='path'][1]");
        ok &= Expect((staffY > bracketY) && ((staffY - bracketY) < 1500),
            "ordered staff items created excessive space below an ending bracket");
    }
    return ok;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 10) {
        std::cerr << "usage: staff-item-order-test ORDER PARTIAL DEFAULT VERTICAL_GROUP VGRP_CHANGE REH "
                     "VGRP_OVERLAP ENDING RESOURCE_PATH\n";
        return 2;
    }

    bool ok = true;
    ok &= TestListParser();
    ok &= TestRegistrationCopyAndReset();
    ok &= TestVerticalGroupClasses();
    ok &= TestResolver();
    ok &= TestMEIRoundtrip(argv[1], argv[9]);
    ok &= TestVerticalGroupRoundtrip(argv[4], argv[9]);
    ok &= TestHistoricRehVerticalGroupRoundtrip(argv[6], argv[9]);
    ok &= TestRendering(argv[1], argv[2], argv[3], argv[4], argv[5], argv[7], argv[8], argv[9]);
    return ok ? 0 : 1;
}
