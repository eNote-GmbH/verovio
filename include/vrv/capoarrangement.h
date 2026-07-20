/////////////////////////////////////////////////////////////////////////////
// Name:        capoarrangement.h
// Author:      Verovio contributors
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef __VRV_CAPOARRANGEMENT_H__
#define __VRV_CAPOARRANGEMENT_H__

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace vrv {

/** A supported chord label reduced to pitch classes. */
struct CapoChord {
    int root = 0;
    std::string quality;
    std::optional<int> bass;
};

/** A single detected barre, expressed as zero-based string indexes, low E first. */
struct CapoBarre {
    int fret = 0;
    int firstString = 0;
    int lastString = 0;
};

/** One playable, strictly matching guitar shape. Muted strings use -1. */
struct CapoFingering {
    std::array<int, 6> frets{};
    std::array<int, 6> fingers{};
    std::optional<CapoBarre> barre;
    int difficultyLevel = 0;
    int difficultyPoints = 0;
    int soundLoss = 0;
    int bassPenalty = 0;
};

/** One ordered occurrence used by the per-mdiv capo optimiser. */
struct CapoOccurrence {
    std::optional<CapoChord> soundingChord;
    std::optional<std::array<int, 6>> preferredFrets;
};

/** Complete input for a per-mdiv arrangement decision. */
struct CapoArrangementRequest {
    std::vector<CapoOccurrence> occurrences;
    int transpositionSemitones = 0;
    int minimumCapo = 0;
    int maximumCapo = 7;
    std::optional<int> fixedCapo;
};

/** The selected target capo and best shape for every occurrence. */
struct CapoArrangementPlan {
    int capo = 0;
    std::vector<std::optional<CapoChord>> shapeChords;
    std::vector<std::optional<CapoFingering>> fingerings;
};

/** Parse the supported chord-label subset. */
std::optional<CapoChord> ParseCapoChord(const std::u32string &label);

#ifdef VRV_TRANSPOSE_CAPO_TESTING
/** Test-only access to candidates; production callers use PlanCapoArrangement. */
std::vector<CapoFingering> GenerateCapoFingerings(const CapoChord &chord, int capo = 0);
/** Test-only access to the physical solver for a literal low-E-to-high-E pattern. */
std::vector<CapoFingering> SolveCapoFingeringPatternForTesting(const std::array<int, 6> &frets);
/** Test-only validation of one concrete finger assignment. */
bool ValidateCapoFingeringForTesting(const std::array<int, 6> &frets, const std::array<int, 6> &fingers,
    const std::optional<CapoBarre> &barre = std::nullopt);
#endif

/** Select one capo and one best shape per occurrence. */
CapoArrangementPlan PlanCapoArrangement(const CapoArrangementRequest &request);

} // namespace vrv

#endif
