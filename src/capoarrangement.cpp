/////////////////////////////////////////////////////////////////////////////
// Name:        capoarrangement.cpp
// Author:      Verovio contributors
// Created:     2026
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "capoarrangement.h"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <tuple>
#include <vector>

namespace vrv {

namespace {

    constexpr std::array<int, 6> s_tuning{ 40, 45, 50, 55, 59, 64 };
    constexpr std::array<int, 8> s_capoPenalty{ 0, 0, 0, 1, 3, 6, 10, 15 };
    constexpr int s_maxShapesPerChord = 12;
    constexpr int s_maxFingeringsPerShape = 4;
    constexpr std::array<std::array<int, 5>, 5> s_fingerSpans{ {
        { 0, 0, 0, 0, 0 },
        { 0, 0, 2, 3, 4 },
        { 0, 2, 0, 1, 3 },
        { 0, 3, 1, 0, 1 },
        { 0, 4, 3, 1, 0 },
    } };

    int Mod12(int value)
    {
        value %= 12;
        return value < 0 ? value + 12 : value;
    }

    struct ChordFormula {
        std::set<int> allowed;
        std::set<int> required;
        int third = -1;
        int seventh = -1;
    };

    std::optional<ChordFormula> Formula(const std::string &quality)
    {
        if (quality.empty()) return ChordFormula{ { 0, 4, 7 }, { 0, 4 }, 4, -1 };
        if (quality == "m") return ChordFormula{ { 0, 3, 7 }, { 0, 3 }, 3, -1 };
        if (quality == "7") return ChordFormula{ { 0, 4, 7, 10 }, { 0, 4, 10 }, 4, 10 };
        if (quality == "maj7") return ChordFormula{ { 0, 4, 7, 11 }, { 0, 4, 11 }, 4, 11 };
        if (quality == "m7") return ChordFormula{ { 0, 3, 7, 10 }, { 0, 3, 10 }, 3, 10 };
        if (quality == "sus2") return ChordFormula{ { 0, 2, 7 }, { 0, 2 }, 2, -1 };
        if (quality == "sus4") return ChordFormula{ { 0, 5, 7 }, { 0, 5 }, 5, -1 };
        if (quality == "add9") return ChordFormula{ { 0, 2, 4, 7 }, { 0, 2, 4 }, 4, -1 };
        if (quality == "6") return ChordFormula{ { 0, 4, 7, 9 }, { 0, 4, 9 }, 4, -1 };
        if (quality == "6add9") return ChordFormula{ { 0, 2, 4, 7, 9 }, { 0, 2, 4, 9 }, 4, -1 };
        return std::nullopt;
    }

    std::set<int> ColorTones(const std::string &quality)
    {
        if (quality == "7" || quality == "m7") return { 10 };
        if (quality == "maj7") return { 11 };
        if (quality == "sus2" || quality == "add9") return { 2 };
        if (quality == "sus4") return { 5 };
        if (quality == "6") return { 9 };
        if (quality == "6add9") return { 2, 9 };
        return {};
    }

    std::optional<int> ParsePitch(const std::u32string &text, size_t &position)
    {
        if (position >= text.size() || text[position] < U'A' || text[position] > U'G') return std::nullopt;
        static constexpr std::array<int, 7> pitchClasses{ 9, 11, 0, 2, 4, 5, 7 };
        int pitch = pitchClasses[text[position++] - U'A'];
        while (position < text.size()) {
            if (text[position] == U'#' || text[position] == U'♯')
                ++pitch;
            else if (text[position] == U'b' || text[position] == U'♭')
                --pitch;
            else if (text[position] == U'𝄪')
                pitch += 2;
            else if (text[position] == U'𝄫')
                pitch -= 2;
            else
                break;
            ++position;
        }
        return Mod12(pitch);
    }

    std::string ToAscii(const std::u32string &text)
    {
        std::string result;
        for (char32_t value : text) {
            if (value < 128)
                result.push_back(static_cast<char>(value));
            else if (value == U'Δ')
                result += "delta";
            else
                return {};
        }
        return result;
    }

    struct PhysicalPattern {
        std::array<int, 6> frets{};
        std::array<int, 6> fingers{};
        std::optional<CapoBarre> barre;
        int sounding = 0;
        int open = 0;
        int span = 0;
        int lowestFret = 0;
        int highestFret = 0;
        int outerMutes = 0;
        int usedFingers = 0;
        int stretchCost = 0;
        int baseDifficulty = 0;
    };

    bool SameBarre(const std::optional<CapoBarre> &left, const std::optional<CapoBarre> &right)
    {
        if (left.has_value() != right.has_value()) return false;
        if (!left) return true;
        return left->fret == right->fret && left->firstString == right->firstString
            && left->lastString == right->lastString;
    }

    bool AssignmentFits(const std::array<int, 6> &frets, const std::array<int, 6> &fingers, int string, int finger)
    {
        for (int otherString = 0; otherString < 6; ++otherString) {
            const int otherFinger = fingers[otherString];
            if (otherFinger <= 0) continue;
            const int distance = std::abs(frets[string] - frets[otherString]);
            if (distance > s_fingerSpans[finger][otherFinger]) return false;
            if (frets[string] < frets[otherString] && finger >= otherFinger) return false;
            if (frets[string] > frets[otherString] && finger <= otherFinger) return false;
            if (frets[string] == frets[otherString]) {
                if (string < otherString && finger >= otherFinger) return false;
                if (string > otherString && finger <= otherFinger) return false;
            }
        }
        return true;
    }

#ifdef VRV_TRANSPOSE_CAPO_TESTING
    bool ValidatePhysicalAssignment(
        const std::array<int, 6> &frets, const std::array<int, 6> &fingers, const std::optional<CapoBarre> &barre)
    {
        int sounding = 0;
        int firstSounding = 6;
        int lastSounding = -1;
        int minimum = 8;
        int maximum = 0;
        for (int string = 0; string < 6; ++string) {
            const int fret = frets[string];
            if (fret < -1 || fret > 7) return false;
            if (fret >= 0) {
                ++sounding;
                firstSounding = std::min(firstSounding, string);
                lastSounding = std::max(lastSounding, string);
            }
            if (fret > 0) {
                minimum = std::min(minimum, fret);
                maximum = std::max(maximum, fret);
                if (fingers[string] < 1 || fingers[string] > 4) return false;
            }
            else if (fingers[string] != 0) {
                return false;
            }
        }
        if (sounding < 4 || minimum == 8 || maximum - minimum > 4) return false;
        for (int string = firstSounding; string <= lastSounding; ++string) {
            if (frets[string] < 0) return false;
        }

        if (barre) {
            if (barre->fret != minimum || barre->firstString < 0 || barre->firstString >= barre->lastString
                || barre->lastString >= 6) {
                return false;
            }
            int notesAtBarreFret = 0;
            for (int string = 0; string < 6; ++string) {
                if (fingers[string] == 1
                    && (string < barre->firstString || string > barre->lastString || frets[string] != barre->fret)) {
                    return false;
                }
                if (string < barre->firstString || string > barre->lastString) continue;
                if (frets[string] <= 0 || frets[string] < barre->fret) return false;
                if (frets[string] == barre->fret) {
                    ++notesAtBarreFret;
                    if (fingers[string] != 1) return false;
                }
                else if (fingers[string] == 1) {
                    return false;
                }
            }
            if (notesAtBarreFret < 2) return false;
        }

        for (int left = 0; left < 6; ++left) {
            if (fingers[left] <= 0) continue;
            for (int right = left + 1; right < 6; ++right) {
                if (fingers[right] <= 0) continue;
                const int leftFinger = fingers[left];
                const int rightFinger = fingers[right];
                const bool sameBarreFinger = barre && leftFinger == 1 && rightFinger == 1 && frets[left] == barre->fret
                    && frets[right] == barre->fret;
                if (leftFinger == rightFinger && !sameBarreFinger) return false;
                if (sameBarreFinger) continue;
                if (std::abs(frets[left] - frets[right]) > s_fingerSpans[leftFinger][rightFinger]) return false;
                if (frets[left] < frets[right] && leftFinger >= rightFinger) return false;
                if (frets[left] > frets[right] && leftFinger <= rightFinger) return false;
                if (frets[left] == frets[right] && leftFinger >= rightFinger) return false;
            }
        }
        return true;
    }
#endif

    void AssignDistinctFingers(const std::array<int, 6> &frets, const std::vector<int> &strings, size_t position,
        int firstFinger, std::array<int, 6> &fingers, std::vector<std::array<int, 6>> &assignments)
    {
        if (position == strings.size()) {
            assignments.push_back(fingers);
            return;
        }
        const int string = strings[position];
        for (int finger = firstFinger; finger <= 4; ++finger) {
            if (std::find(fingers.begin(), fingers.end(), finger) != fingers.end()) continue;
            if (!AssignmentFits(frets, fingers, string, finger)) continue;
            fingers[string] = finger;
            AssignDistinctFingers(frets, strings, position + 1, firstFinger, fingers, assignments);
            fingers[string] = 0;
        }
    }

    PhysicalPattern MakePhysicalPattern(
        const std::array<int, 6> &frets, const std::array<int, 6> &fingers, std::optional<CapoBarre> barre)
    {
        PhysicalPattern pattern;
        pattern.frets = frets;
        pattern.fingers = fingers;
        pattern.barre = barre;
        int firstSounding = 6;
        int lastSounding = -1;
        int minimum = 8;
        int maximum = 0;
        std::array<int, 5> fingerFrets{};
        std::array<bool, 5> used{};
        for (int string = 0; string < 6; ++string) {
            const int fret = frets[string];
            if (fret < 0) continue;
            ++pattern.sounding;
            firstSounding = std::min(firstSounding, string);
            lastSounding = std::max(lastSounding, string);
            if (fret == 0)
                ++pattern.open;
            else {
                minimum = std::min(minimum, fret);
                maximum = std::max(maximum, fret);
                const int finger = fingers[string];
                if (finger > 0) {
                    used[finger] = true;
                    fingerFrets[finger] = fret;
                }
            }
        }
        pattern.span = maximum > 0 ? maximum - minimum : 0;
        pattern.lowestFret = maximum > 0 ? minimum : 0;
        pattern.highestFret = maximum;
        pattern.outerMutes = firstSounding + (5 - lastSounding);
        for (int finger = 1; finger <= 4; ++finger) {
            if (used[finger]) ++pattern.usedFingers;
            for (int other = finger + 1; other <= 4; ++other) {
                if (used[finger] && used[other])
                    pattern.stretchCost += std::abs(fingerFrets[finger] - fingerFrets[other]);
            }
        }
        pattern.baseDifficulty = pattern.usedFingers * 4 + pattern.stretchCost + pattern.outerMutes * 5;
        pattern.baseDifficulty += std::max(0, pattern.highestFret - 3) * 6;
        if (barre) pattern.baseDifficulty += 6 + barre->lastString - barre->firstString + 1;
        return pattern;
    }

    std::vector<PhysicalPattern> SolvePhysicalPattern(const std::array<int, 6> &frets)
    {
        int sounding = 0;
        int firstSounding = 6;
        int lastSounding = -1;
        int minimum = 8;
        int maximum = 0;
        std::vector<int> frettedStrings;
        for (int string = 0; string < 6; ++string) {
            const int fret = frets[string];
            if (fret < 0) continue;
            ++sounding;
            firstSounding = std::min(firstSounding, string);
            lastSounding = std::max(lastSounding, string);
            if (fret > 0) {
                frettedStrings.push_back(string);
                minimum = std::min(minimum, fret);
                maximum = std::max(maximum, fret);
            }
        }
        if (sounding < 4 || frettedStrings.empty() || maximum - minimum > 4) return {};
        for (int string = firstSounding; string <= lastSounding; ++string) {
            if (frets[string] < 0) return {};
        }

        std::vector<PhysicalPattern> result;
        if (frettedStrings.size() <= 4) {
            std::array<int, 6> fingers{};
            std::vector<std::array<int, 6>> assignments;
            AssignDistinctFingers(frets, frettedStrings, 0, 1, fingers, assignments);
            for (const auto &assignment : assignments)
                result.push_back(MakePhysicalPattern(frets, assignment, std::nullopt));
        }

        std::vector<int> minimumStrings;
        for (int string : frettedStrings) {
            if (frets[string] == minimum) minimumStrings.push_back(string);
        }
        if (minimumStrings.size() >= 2) {
            const int first = minimumStrings.front();
            const int last = minimumStrings.back();
            bool validBarre = true;
            for (int string = first; string <= last; ++string) {
                if (frets[string] <= 0 || frets[string] < minimum) validBarre = false;
            }
            if (validBarre) {
                std::array<int, 6> fingers{};
                for (int string : minimumStrings) fingers[string] = 1;
                std::vector<int> remaining;
                for (int string : frettedStrings) {
                    if (frets[string] > minimum) remaining.push_back(string);
                }
                if (remaining.size() <= 3) {
                    std::vector<std::array<int, 6>> assignments;
                    AssignDistinctFingers(frets, remaining, 0, 2, fingers, assignments);
                    const CapoBarre barre{ minimum, first, last };
                    for (const auto &assignment : assignments)
                        result.push_back(MakePhysicalPattern(frets, assignment, barre));
                }
            }
        }

        std::sort(result.begin(), result.end(), [](const PhysicalPattern &left, const PhysicalPattern &right) {
            return std::tie(left.baseDifficulty, left.stretchCost, left.usedFingers, left.fingers)
                < std::tie(right.baseDifficulty, right.stretchCost, right.usedFingers, right.fingers);
        });
        result.erase(std::unique(result.begin(), result.end(),
                         [](const PhysicalPattern &left, const PhysicalPattern &right) {
                             return left.fingers == right.fingers && SameBarre(left.barre, right.barre);
                         }),
            result.end());
        if (result.size() > s_maxFingeringsPerShape) result.resize(s_maxFingeringsPerShape);
        return result;
    }

    const std::vector<PhysicalPattern> &PhysicalPatterns()
    {
        static const std::vector<PhysicalPattern> patterns = []() {
            std::vector<PhysicalPattern> result;
            std::array<int, 6> frets{};
            for (int encoded = 0; encoded < 531441; ++encoded) {
                int value = encoded;
                for (int string = 0; string < 6; ++string) {
                    frets[string] = value % 9 - 1;
                    value /= 9;
                }
                std::vector<PhysicalPattern> solved = SolvePhysicalPattern(frets);
                result.insert(result.end(), solved.begin(), solved.end());
            }
            return result;
        }();
        return patterns;
    }

    int DifficultyLevel(int points)
    {
        if (points <= 28) return 1;
        if (points <= 34) return 2;
        if (points <= 40) return 3;
        if (points <= 48) return 4;
        return 5;
    }

    int LowestFret(const CapoFingering &fingering)
    {
        int minimum = 8;
        for (int fret : fingering.frets) {
            if (fret > 0) minimum = std::min(minimum, fret);
        }
        return minimum == 8 ? 0 : minimum;
    }

    auto FingeringKey(const CapoFingering &fingering)
    {
        return std::tuple{ fingering.difficultyLevel, fingering.bassPenalty,
            fingering.difficultyPoints + fingering.soundLoss * 4, fingering.soundLoss, fingering.difficultyPoints,
            fingering.frets, fingering.fingers };
    }

    int CapoPenalty(int capo)
    {
        if (capo >= 0 && capo < static_cast<int>(s_capoPenalty.size())) return s_capoPenalty[capo];
        return s_capoPenalty.back() + (capo - 7) * 8;
    }

    struct CandidateOption {
        CapoFingering fingering;
        bool preferred = false;
    };

    auto CandidateKey(const CandidateOption &candidate)
    {
        const CapoFingering &fingering = candidate.fingering;
        return std::tuple{ fingering.difficultyLevel, fingering.bassPenalty,
            fingering.difficultyPoints + fingering.soundLoss * 4, fingering.soundLoss, fingering.difficultyPoints,
            LowestFret(fingering), candidate.preferred ? 0 : 1, fingering.frets, fingering.fingers };
    }

    int StaticEffort(const CapoFingering &fingering)
    {
        return fingering.difficultyPoints + fingering.soundLoss * 4;
    }

    std::vector<CandidateOption> SelectCandidates(
        std::vector<CapoFingering> fingerings, const std::optional<std::array<int, 6>> &preferredFrets)
    {
        std::vector<CandidateOption> ranked;
        ranked.reserve(fingerings.size());
        for (CapoFingering &fingering : fingerings) {
            const bool preferred = preferredFrets && *preferredFrets == fingering.frets;
            ranked.push_back({ std::move(fingering), preferred });
        }
        std::sort(ranked.begin(), ranked.end(), [](const CandidateOption &left, const CandidateOption &right) {
            return CandidateKey(left) < CandidateKey(right);
        });

        std::vector<CandidateOption> selected;
        std::map<std::array<int, 6>, int> assignmentsPerShape;
        int shapeCount = 0;
        for (CandidateOption &candidate : ranked) {
            auto found = assignmentsPerShape.find(candidate.fingering.frets);
            if (found == assignmentsPerShape.end()) {
                if (shapeCount >= s_maxShapesPerChord) continue;
                found = assignmentsPerShape.emplace(candidate.fingering.frets, 0).first;
                ++shapeCount;
            }
            if (found->second >= s_maxFingeringsPerShape) continue;
            ++found->second;
            selected.push_back(std::move(candidate));
        }
        return selected;
    }

    struct FingerPosition {
        bool present = false;
        int fret = 0;
        int doubledString = 0;
    };

    FingerPosition GetFingerPosition(const CapoFingering &fingering, int finger)
    {
        if (finger == 1 && fingering.barre) {
            return { true, fingering.barre->fret, fingering.barre->firstString + fingering.barre->lastString };
        }
        for (int string = 0; string < 6; ++string) {
            if (fingering.fingers[string] == finger) return { true, fingering.frets[string], string * 2 };
        }
        return {};
    }

    int TransitionCost(const CapoFingering &left, const CapoFingering &right)
    {
        int cost = 2 * std::abs(LowestFret(left) - LowestFret(right));
        for (int finger = 1; finger <= 4; ++finger) {
            const FingerPosition from = GetFingerPosition(left, finger);
            const FingerPosition to = GetFingerPosition(right, finger);
            if (from.present != to.present) {
                cost += 2;
            }
            else if (from.present) {
                cost += 2 * std::abs(from.fret - to.fret);
                cost += (std::abs(from.doubledString - to.doubledString) + 1) / 2;
            }
        }
        if (left.barre.has_value() != right.barre.has_value()) {
            const int width = left.barre ? left.barre->lastString - left.barre->firstString + 1
                                         : right.barre->lastString - right.barre->firstString + 1;
            cost += 2 + width;
        }
        else if (left.barre && right.barre) {
            const int leftWidth = left.barre->lastString - left.barre->firstString + 1;
            const int rightWidth = right.barre->lastString - right.barre->firstString + 1;
            if (left.barre->fret != right.barre->fret || left.barre->firstString != right.barre->firstString
                || left.barre->lastString != right.barre->lastString) {
                cost += 2 + std::abs(leftWidth - rightWidth);
            }
        }
        return cost;
    }

    using StableState = std::tuple<int, std::array<int, 6>, std::array<int, 6>>;

    StableState GetStableState(const CandidateOption &candidate)
    {
        return { candidate.preferred ? 0 : 1, candidate.fingering.frets, candidate.fingering.fingers };
    }

    struct PathState {
        int maximumDifficulty = 0;
        int bassPenalty = 0;
        long long effort = 0;
        long long soundLoss = 0;
        std::vector<StableState> stable;
        std::vector<int> choices;
    };

    auto PathKey(const PathState &path)
    {
        return std::tie(path.maximumDifficulty, path.bassPenalty, path.effort, path.soundLoss, path.stable);
    }

} // namespace

std::optional<CapoChord> ParseCapoChord(const std::u32string &label)
{
    size_t position = 0;
    while (position < label.size() && (label[position] == U' ' || label[position] == U'\t')) ++position;
    const std::optional<int> root = ParsePitch(label, position);
    if (!root) return std::nullopt;

    const size_t qualityStart = position;
    while (position < label.size() && label[position] != U'/') ++position;
    size_t qualityEnd = position;
    while (qualityEnd > qualityStart && label[qualityEnd - 1] == U' ') --qualityEnd;
    std::string quality = ToAscii(label.substr(qualityStart, qualityEnd - qualityStart));
    if (quality == "min" || quality == "minor")
        quality = "m";
    else if (quality == "min7")
        quality = "m7";
    else if (quality == "M7" || quality == "delta7")
        quality = "maj7";
    if (!Formula(quality)) return std::nullopt;

    std::optional<int> bass;
    if (position < label.size()) {
        ++position;
        while (position < label.size() && label[position] == U' ') ++position;
        bass = ParsePitch(label, position);
        if (!bass) return std::nullopt;
    }
    while (position < label.size() && label[position] == U' ') ++position;
    if (position != label.size()) return std::nullopt;
    return CapoChord{ *root, quality, bass };
}

std::vector<CapoFingering> GenerateCapoFingerings(const CapoChord &chord, int capo)
{
    using CacheKey = std::tuple<int, std::string, int, int>;
    static std::mutex cacheMutex;
    static std::map<CacheKey, std::vector<CapoFingering>> cache;
    const CacheKey cacheKey{ Mod12(chord.root), chord.quality, chord.bass.value_or(-1), capo };
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (const auto found = cache.find(cacheKey); found != cache.end()) return found->second;

    const std::optional<ChordFormula> formula = Formula(chord.quality);
    if (!formula) return {};
    unsigned int allowedMask = 0;
    unsigned int requiredMask = 0;
    for (int pitch : formula->allowed) allowedMask |= 1U << pitch;
    for (int pitch : formula->required) requiredMask |= 1U << pitch;
    std::vector<CapoFingering> result;
    for (const PhysicalPattern &pattern : PhysicalPatterns()) {
        unsigned int presentMask = 0;
        std::array<int, 12> multiplicity{};
        int lowestMidi = 1000;
        int lowestPitchClass = -1;
        std::array<int, 6> soundingMidi{};
        int soundingCount = 0;
        for (int string = 0; string < 6; ++string) {
            const int fret = pattern.frets[string];
            if (fret < 0) continue;
            const int midi = s_tuning[string] + capo + fret;
            const int relative = Mod12(midi - (chord.root + capo));
            presentMask |= 1U << relative;
            ++multiplicity[relative];
            soundingMidi[soundingCount++] = midi;
            if (midi < lowestMidi) {
                lowestMidi = midi;
                lowestPitchClass = Mod12(midi);
            }
        }
        const unsigned int foreignMask = presentMask & ~allowedMask;
        if (foreignMask) {
            if (!chord.bass || lowestPitchClass != Mod12(*chord.bass + capo)) continue;
            const int pedalRelative = Mod12(*chord.bass - chord.root);
            if (foreignMask != (1U << pedalRelative) || multiplicity[pedalRelative] != 1) continue;
        }
        if ((presentMask & requiredMask) != requiredMask) continue;
        if (chord.bass && lowestPitchClass != Mod12(*chord.bass + capo)) continue;

        const int positionCost = std::max(0, capo + pattern.lowestFret - 3);
        const int difficulty = pattern.baseDifficulty + positionCost;
        int soundLoss = pattern.sounding == 4 ? 3 : (pattern.sounding == 5 ? 1 : 0);
        if (!(presentMask & (1U << 7)) && formula->allowed.contains(7)) soundLoss += 2;
        for (int colorTone : ColorTones(chord.quality)) {
            if (multiplicity[colorTone] > 1) soundLoss += 2 * (multiplicity[colorTone] - 1);
        }
        std::sort(soundingMidi.begin(), soundingMidi.begin() + soundingCount);
        for (int index = 1; index < soundingCount; ++index) {
            if (soundingMidi[index] - soundingMidi[index - 1] > 12) soundLoss += 2;
        }
        const int bassPenalty = chord.bass || lowestPitchClass == Mod12(chord.root + capo) ? 0 : 1;
        result.push_back({ pattern.frets, pattern.fingers, pattern.barre, DifficultyLevel(difficulty), difficulty,
            soundLoss, bassPenalty });
    }
    std::sort(result.begin(), result.end(),
        [](const CapoFingering &left, const CapoFingering &right) { return FingeringKey(left) < FingeringKey(right); });
    cache.emplace(cacheKey, result);
    return result;
}

#ifdef VRV_TRANSPOSE_CAPO_TESTING
std::vector<CapoFingering> SolveCapoFingeringPatternForTesting(const std::array<int, 6> &frets)
{
    std::vector<CapoFingering> result;
    for (const PhysicalPattern &pattern : SolvePhysicalPattern(frets)) {
        result.push_back({ pattern.frets, pattern.fingers, pattern.barre, DifficultyLevel(pattern.baseDifficulty),
            pattern.baseDifficulty, 0, 0 });
    }
    return result;
}

bool ValidateCapoFingeringForTesting(
    const std::array<int, 6> &frets, const std::array<int, 6> &fingers, const std::optional<CapoBarre> &barre)
{
    return ValidatePhysicalAssignment(frets, fingers, barre);
}
#endif

CapoArrangementPlan PlanCapoArrangement(const CapoArrangementRequest &request)
{
    CapoArrangementPlan best;
    bool hasBest = false;
    using CapoKey = std::tuple<int, int, int, int, long long, long long, int, std::vector<StableState>>;
    CapoKey bestKey;
    const int minimum = request.fixedCapo.value_or(std::min(request.minimumCapo, request.maximumCapo));
    const int maximum = request.fixedCapo.value_or(std::max(request.minimumCapo, request.maximumCapo));
    for (int capo = minimum; capo <= maximum; ++capo) {
        CapoArrangementPlan plan;
        plan.capo = capo;
        plan.shapeChords.resize(request.occurrences.size());
        plan.fingerings.resize(request.occurrences.size());
        std::vector<std::vector<CandidateOption>> candidateSets(request.occurrences.size());
        int missingOccurrences = 0;
        std::set<std::tuple<int, std::string, int>> missingChords;

        for (size_t index = 0; index < request.occurrences.size(); ++index) {
            const CapoOccurrence &occurrence = request.occurrences[index];
            if (!occurrence.soundingChord) continue;
            CapoChord shape = *occurrence.soundingChord;
            shape.root = Mod12(shape.root + request.transpositionSemitones - capo);
            if (shape.bass) shape.bass = Mod12(*shape.bass + request.transpositionSemitones - capo);
            plan.shapeChords[index] = shape;
            candidateSets[index] = SelectCandidates(GenerateCapoFingerings(shape, capo), occurrence.preferredFrets);
            if (candidateSets[index].empty()) {
                ++missingOccurrences;
                missingChords.insert({ shape.root, shape.quality, shape.bass.value_or(-1) });
            }
        }

        int maximumDifficulty = 0;
        int totalBassPenalty = 0;
        long long totalEffort = CapoPenalty(capo);
        long long totalSoundLoss = 0;
        size_t segmentStart = 0;
        while (segmentStart < candidateSets.size()) {
            while (segmentStart < candidateSets.size() && candidateSets[segmentStart].empty()) ++segmentStart;
            if (segmentStart == candidateSets.size()) break;
            size_t segmentEnd = segmentStart;
            while (segmentEnd < candidateSets.size() && !candidateSets[segmentEnd].empty()) ++segmentEnd;

            std::vector<PathState> previous;
            previous.reserve(candidateSets[segmentStart].size());
            for (size_t candidate = 0; candidate < candidateSets[segmentStart].size(); ++candidate) {
                const CandidateOption &option = candidateSets[segmentStart][candidate];
                previous.push_back(
                    { option.fingering.difficultyLevel, option.fingering.bassPenalty, StaticEffort(option.fingering),
                        option.fingering.soundLoss, { GetStableState(option) }, { static_cast<int>(candidate) } });
            }
            for (size_t position = segmentStart + 1; position < segmentEnd; ++position) {
                std::vector<PathState> current(candidateSets[position].size());
                for (size_t candidate = 0; candidate < candidateSets[position].size(); ++candidate) {
                    bool found = false;
                    const CandidateOption &option = candidateSets[position][candidate];
                    for (size_t prior = 0; prior < previous.size(); ++prior) {
                        PathState path = previous[prior];
                        path.maximumDifficulty = std::max(path.maximumDifficulty, option.fingering.difficultyLevel);
                        path.bassPenalty += option.fingering.bassPenalty;
                        path.effort += StaticEffort(option.fingering)
                            + TransitionCost(
                                candidateSets[position - 1][path.choices.back()].fingering, option.fingering);
                        path.soundLoss += option.fingering.soundLoss;
                        path.stable.push_back(GetStableState(option));
                        path.choices.push_back(static_cast<int>(candidate));
                        if (!found || PathKey(path) < PathKey(current[candidate])) {
                            found = true;
                            current[candidate] = std::move(path);
                        }
                    }
                }
                previous = std::move(current);
            }
            const PathState &segmentBest = *std::min_element(previous.begin(), previous.end(),
                [](const PathState &left, const PathState &right) { return PathKey(left) < PathKey(right); });
            maximumDifficulty = std::max(maximumDifficulty, segmentBest.maximumDifficulty);
            totalBassPenalty += segmentBest.bassPenalty;
            totalEffort += segmentBest.effort;
            totalSoundLoss += segmentBest.soundLoss;
            for (size_t offset = 0; offset < segmentBest.choices.size(); ++offset) {
                plan.fingerings[segmentStart + offset]
                    = candidateSets[segmentStart + offset][segmentBest.choices[offset]].fingering;
            }
            segmentStart = segmentEnd;
        }

        const std::array<int, 6> missingFrets{ -1, -1, -1, -1, -1, -1 };
        const std::array<int, 6> missingFingers{};
        std::vector<StableState> stable;
        stable.reserve(plan.fingerings.size());
        for (size_t index = 0; index < plan.fingerings.size(); ++index) {
            if (!plan.fingerings[index]) {
                stable.emplace_back(2, missingFrets, missingFingers);
                continue;
            }
            const bool preferred = request.occurrences[index].preferredFrets
                && *request.occurrences[index].preferredFrets == plan.fingerings[index]->frets;
            stable.emplace_back(preferred ? 0 : 1, plan.fingerings[index]->frets, plan.fingerings[index]->fingers);
        }
        const CapoKey key{ missingOccurrences, static_cast<int>(missingChords.size()), maximumDifficulty,
            totalBassPenalty, totalEffort, totalSoundLoss, capo, stable };
        if (!hasBest || key < bestKey) {
            hasBest = true;
            bestKey = key;
            best = std::move(plan);
        }
    }
    return best;
}

} // namespace vrv
