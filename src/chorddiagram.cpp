/////////////////////////////////////////////////////////////////////////////
// Name:        chorddiagram.cpp
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "chorddiagram.h"

//----------------------------------------------------------------------------

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <map>
#include <sstream>

//----------------------------------------------------------------------------

#include "barre.h"
#include "chorddef.h"
#include "chordmember.h"
#include "harm.h"

namespace vrv {

namespace {

    int CountTokens(const std::string &value)
    {
        std::istringstream stream(value);
        std::string token;
        int count = 0;
        while (stream >> token) ++count;
        return count;
    }

    bool ParsePositiveInteger(const std::string &value, int &number)
    {
        if (value.empty()) return false;
        errno = 0;
        char *end = NULL;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if ((errno != 0) || (end == value.c_str()) || (*end != '\0') || (parsed <= 0) || (parsed > INT_MAX)) {
            return false;
        }
        number = static_cast<int>(parsed);
        return true;
    }

    bool GetCourse(const ChordMember &member, int &course)
    {
        if (member.HasTabCourse()) {
            course = member.GetTabCourse();
            return true;
        }
        if (member.HasTabString()) return ParsePositiveInteger(member.GetTabString(), course);
        return false;
    }

    bool IsLocalReference(const std::string &reference)
    {
        return (reference.size() > 1) && (reference.front() == '#') && (reference.find('#', 1) == std::string::npos);
    }

    void AddDiagnostic(ChordDiagramLayoutResult &result, ChordDiagramDiagnosticType type, const std::string &message)
    {
        ChordDiagramDiagnostic diagnostic;
        diagnostic.m_type = type;
        diagnostic.m_message = message;
        result.m_diagnostics.push_back(diagnostic);
    }

} // namespace

HarmDrawingMode GetHarmDrawingMode(const Harm &harm)
{
    const bool hasText = (harm.FindDescendantByType(TEXT) != NULL) || (harm.FindDescendantByType(NUM) != NULL)
        || (harm.FindDescendantByType(SYMBOL) != NULL);

    HarmDrawingMode mode;
    mode.m_hasText = hasText;
    mode.m_drawText = hasText;
    switch (harm.GetRendgrid()) {
        case harmVis_RENDGRID_grid:
            mode.m_drawGrid = true;
            mode.m_drawText = false;
            break;
        case harmVis_RENDGRID_gridtext: mode.m_drawGrid = true; break;
        case harmVis_RENDGRID_text: break;
        case harmVis_RENDGRID_NONE: mode.m_drawGrid = !hasText && harm.HasChordDef(); break;
        default: break;
    }
    return mode;
}

ChordDiagramLayoutResult BuildChordDiagramLayout(const ChordDef &chordDef)
{
    ChordDiagramLayoutResult result;
    ChordDiagramLayout &layout = result.m_layout;

    std::vector<const ChordMember *> members;
    std::vector<const Barre *> barres;
    std::map<const ChordMember *, int> memberCourses;
    std::map<int, const ChordMember *> courseMembers;
    int maxCourse = 0;

    for (const Object *child : chordDef.GetChildren()) {
        if (child->Is(CHORDMEMBER))
            members.push_back(vrv_cast<const ChordMember *>(child));
        else if (child->Is(BARRE))
            barres.push_back(vrv_cast<const Barre *>(child));
    }

    for (const ChordMember *member : members) {
        int course = 0;
        if (!GetCourse(*member, course)) {
            if (member->HasTabString()) {
                AddDiagnostic(
                    result, ChordDiagramDiagnosticType::InvalidCourse, "A chordMember has a non-numeric @tab.string");
                return result;
            }
            continue;
        }
        if (course <= 0) {
            AddDiagnostic(
                result, ChordDiagramDiagnosticType::InvalidCourse, "A chordMember has a non-positive course number");
            return result;
        }
        if (courseMembers.count(course)) {
            AddDiagnostic(result, ChordDiagramDiagnosticType::DuplicateCourse,
                "Multiple chordMember elements define the same course");
            return result;
        }
        memberCourses[member] = course;
        courseMembers[course] = member;
        maxCourse = std::max(maxCourse, course);
    }

    int declaredCount = 0;
    if (chordDef.HasTabCourses())
        declaredCount = CountTokens(chordDef.GetTabCourses());
    else if (chordDef.HasTabStrings())
        declaredCount = CountTokens(chordDef.GetTabStrings());

    layout.m_stringCount = declaredCount ? declaredCount : maxCourse;
    if ((layout.m_stringCount < 3) || (layout.m_stringCount > 6)) {
        AddDiagnostic(result, ChordDiagramDiagnosticType::UnsupportedStringCount,
            "Chord diagrams require an explicit string count between 3 and 6");
        return result;
    }
    if (maxCourse > layout.m_stringCount) {
        AddDiagnostic(result, ChordDiagramDiagnosticType::CourseCountConflict,
            "A chordMember course exceeds the chordDef string count");
        return result;
    }

    layout.m_firstFret = chordDef.HasTabPos() ? chordDef.GetTabPos() : 1;
    if (layout.m_firstFret <= 0) {
        AddDiagnostic(result, ChordDiagramDiagnosticType::InvalidPosition, "A chord diagram @tab.pos must be positive");
        return result;
    }

    int maximumFret = layout.m_firstFret + 3;
    for (const ChordMember *member : members) {
        const std::map<const ChordMember *, int>::const_iterator memberCourse = memberCourses.find(member);
        if (memberCourse == memberCourses.end()) continue;
        const int course = memberCourse->second;
        const std::string fingering = member->HasTabFing() ? member->GetTabFing() : "";
        const bool muted = (fingering == "x" || fingering == "X");
        const bool openFingering = (fingering == "o" || fingering == "O");
        const bool hasFret = member->HasTabFret();
        const int fret = hasFret ? member->GetTabFret() : MEI_UNSET;

        if ((hasFret && fret < 0) || (muted && hasFret) || (openFingering && hasFret && fret > 0)) {
            AddDiagnostic(result, ChordDiagramDiagnosticType::InvalidStringState,
                "A chordMember has contradictory or invalid tablature state");
            return result;
        }

        ChordDiagramMarker marker;
        marker.m_course = course;
        // MEI course 1 is the rightmost, highest string; larger course numbers proceed to the left.
        marker.m_x = (layout.m_stringCount - course) * ChordDiagramLayout::s_stringSpacing;
        marker.m_fret = 0;
        if (muted) {
            marker.m_type = ChordDiagramMarkerType::Muted;
            marker.m_y = -ChordDiagramLayout::s_indicatorOffset;
            layout.m_topExtent = ChordDiagramLayout::s_indicatorOffset + ChordDiagramLayout::s_markerDiameter / 2.0;
        }
        else if (openFingering || (hasFret && fret == 0)) {
            marker.m_type = ChordDiagramMarkerType::Open;
            marker.m_y = -ChordDiagramLayout::s_indicatorOffset;
            layout.m_topExtent = ChordDiagramLayout::s_indicatorOffset + ChordDiagramLayout::s_markerDiameter / 2.0;
        }
        else if (hasFret && fret > 0) {
            if (fret < layout.m_firstFret) {
                AddDiagnostic(
                    result, ChordDiagramDiagnosticType::InvalidPosition, "A stopped chordMember lies below @tab.pos");
                return result;
            }
            marker.m_type = ChordDiagramMarkerType::Stopped;
            marker.m_fret = fret;
            marker.m_y = (fret - layout.m_firstFret + 0.5) * ChordDiagramLayout::s_fretSpacing;
            maximumFret = std::max(maximumFret, fret);
        }
        else {
            continue;
        }
        layout.m_markers.push_back(marker);
    }

    for (const Barre *barre : barres) {
        if (!barre->HasFret() || (barre->GetFret() <= 0) || !IsLocalReference(barre->GetStartid())
            || !IsLocalReference(barre->GetEndid())) {
            AddDiagnostic(result, ChordDiagramDiagnosticType::InvalidBarre,
                "A barre is missing a positive @fret or local endpoint references");
            continue;
        }

        const Object *start = chordDef.FindDescendantByID(barre->GetStartid().substr(1));
        const Object *end = chordDef.FindDescendantByID(barre->GetEndid().substr(1));
        if (!start || !end || !start->Is(CHORDMEMBER) || !end->Is(CHORDMEMBER)
            || !memberCourses.count(vrv_cast<const ChordMember *>(start))
            || !memberCourses.count(vrv_cast<const ChordMember *>(end)) || (barre->GetFret() < layout.m_firstFret)) {
            AddDiagnostic(
                result, ChordDiagramDiagnosticType::InvalidBarre, "A barre endpoint or fret could not be laid out");
            continue;
        }

        const int firstCourse = memberCourses.at(vrv_cast<const ChordMember *>(start));
        const int lastCourse = memberCourses.at(vrv_cast<const ChordMember *>(end));
        if (firstCourse == lastCourse) {
            AddDiagnostic(result, ChordDiagramDiagnosticType::InvalidBarre, "A barre must span at least two courses");
            continue;
        }

        ChordDiagramBarre diagramBarre;
        diagramBarre.m_firstCourse = std::min(firstCourse, lastCourse);
        diagramBarre.m_lastCourse = std::max(firstCourse, lastCourse);
        diagramBarre.m_fret = barre->GetFret();
        diagramBarre.m_x1 = (layout.m_stringCount - diagramBarre.m_lastCourse) * ChordDiagramLayout::s_stringSpacing;
        diagramBarre.m_x2 = (layout.m_stringCount - diagramBarre.m_firstCourse) * ChordDiagramLayout::s_stringSpacing;
        diagramBarre.m_y = (barre->GetFret() - layout.m_firstFret + 0.5) * ChordDiagramLayout::s_fretSpacing;
        layout.m_barres.push_back(diagramBarre);
        maximumFret = std::max(maximumFret, barre->GetFret());
    }

    for (const ChordDiagramBarre &barre : layout.m_barres) {
        layout.m_markers.erase(std::remove_if(layout.m_markers.begin(), layout.m_markers.end(),
                                   [&](const ChordDiagramMarker &marker) {
                                       return (marker.m_type == ChordDiagramMarkerType::Stopped)
                                           && (marker.m_fret == barre.m_fret)
                                           && (marker.m_course >= barre.m_firstCourse)
                                           && (marker.m_course <= barre.m_lastCourse);
                                   }),
            layout.m_markers.end());
    }

    if (layout.m_markers.empty() && layout.m_barres.empty()) {
        AddDiagnostic(
            result, ChordDiagramDiagnosticType::NoDrawableData, "The chordDef has no drawable tablature data");
        return result;
    }

    layout.m_fretCount = std::max(4, maximumFret - layout.m_firstFret + 1);
    layout.m_width = (layout.m_stringCount - 1) * ChordDiagramLayout::s_stringSpacing;
    layout.m_height = layout.m_fretCount * ChordDiagramLayout::s_fretSpacing;
    result.m_valid = true;
    return result;
}

} // namespace vrv
