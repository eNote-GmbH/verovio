/////////////////////////////////////////////////////////////////////////////
// Name:        chorddiagram.h
// Author:      Simon Waloschek
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_CHORDDIAGRAM_H__
#define __VRV_CHORDDIAGRAM_H__

#include <string>
#include <vector>

namespace vrv {

class ChordDef;
class Harm;

struct HarmDrawingMode {
    bool m_hasText = false;
    bool m_drawGrid = false;
    bool m_drawText = false;
};

enum class ChordDiagramMarkerType { Open, Muted, Stopped };

enum class ChordDiagramDiagnosticType {
    InvalidCourse,
    DuplicateCourse,
    UnsupportedStringCount,
    CourseCountConflict,
    InvalidPosition,
    InvalidStringState,
    InvalidBarre,
    NoDrawableData
};

struct ChordDiagramDiagnostic {
    ChordDiagramDiagnosticType m_type;
    std::string m_message;
};

struct ChordDiagramMarker {
    ChordDiagramMarkerType m_type;
    int m_course;
    int m_fret;
    double m_x;
    double m_y;
};

struct ChordDiagramBarre {
    int m_firstCourse;
    int m_lastCourse;
    int m_fret;
    double m_x1;
    double m_x2;
    double m_y;
};

/** Device-independent chord-diagram geometry. Coordinates are expressed in staff spaces, with y increasing down. */
struct ChordDiagramLayout {
    static constexpr double s_stringSpacing = 0.69;
    static constexpr double s_fretSpacing = 1.0;
    static constexpr double s_markerDiameter = 0.58;
    static constexpr double s_gridLineWidth = 0.08;
    static constexpr double s_nutLineWidth = 0.20;
    static constexpr double s_barreLineWidth = 0.50;
    static constexpr double s_indicatorOffset = 0.55;
    static constexpr double s_labelGap = 0.45;
    static constexpr double s_rowGap = 1.0;

    int m_stringCount = 0;
    int m_firstFret = 1;
    int m_fretCount = 4;
    double m_width = 0.0;
    double m_height = 4.0;
    double m_topExtent = 0.0;
    std::vector<ChordDiagramMarker> m_markers;
    std::vector<ChordDiagramBarre> m_barres;
};

struct ChordDiagramLayoutResult {
    bool m_valid = false;
    ChordDiagramLayout m_layout;
    std::vector<ChordDiagramDiagnostic> m_diagnostics;
};

ChordDiagramLayoutResult BuildChordDiagramLayout(const ChordDef &chordDef);
HarmDrawingMode GetHarmDrawingMode(const Harm &harm);

} // namespace vrv

#endif
