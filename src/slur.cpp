/////////////////////////////////////////////////////////////////////////////
// Name:        slur.cpp
// Author:      Rodolfo Zitellini
// Created:     26/06/2012
// Copyright (c) Authors and others. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "slur.h"

//----------------------------------------------------------------------------

#include <assert.h>
#include <cmath>
#include <iterator>
#include <math.h>
#include <type_traits>

//----------------------------------------------------------------------------

#include "chord.h"
#include "doc.h"
#include "layer.h"
#include "layerelement.h"
#include "staff.h"
#include "verticalaligner.h"
#include "vrv.h"

namespace vrv {

template <typename Iter> struct is_reverse_iterator : std::false_type {
};

template <typename Iter>
struct is_reverse_iterator<std::reverse_iterator<Iter>>
    : std::integral_constant<bool, !is_reverse_iterator<Iter>::value> {
};

//----------------------------------------------------------------------------
// Slur
//----------------------------------------------------------------------------

static const ClassRegistrar<Slur> s_factory("slur", SLUR);

Slur::Slur() : ControlElement("slur-"), TimeSpanningInterface(), AttColor(), AttCurvature(), AttCurveRend()
{
    RegisterInterface(TimeSpanningInterface::GetAttClasses(), TimeSpanningInterface::IsInterface());
    RegisterAttClass(ATT_COLOR);
    RegisterAttClass(ATT_CURVATURE);
    RegisterAttClass(ATT_CURVEREND);

    Reset();
}

Slur::Slur(const std::string &classid)
    : ControlElement(classid), TimeSpanningInterface(), AttColor(), AttCurvature(), AttCurveRend()
{
    RegisterInterface(TimeSpanningInterface::GetAttClasses(), TimeSpanningInterface::IsInterface());
    RegisterAttClass(ATT_COLOR);
    RegisterAttClass(ATT_CURVATURE);
    RegisterAttClass(ATT_CURVEREND);

    Reset();
}

Slur::~Slur() {}

void Slur::Reset()
{
    ControlElement::Reset();
    TimeSpanningInterface::Reset();
    ResetColor();
    ResetCurvature();
    ResetCurveRend();

    m_drawingCurvedir = curvature_CURVEDIR_NONE;
    // m_isCrossStaff = false;
}

bool Slur::AdjustSlur(Doc *doc, FloatingCurvePositioner *curve, Staff *staff)
{
    assert(doc);
    assert(curve);
    assert(staff);

    float slurAngle = curve->GetAngle();
    curvature_CURVEDIR curveDir = curve->GetDir();
    Point points[4];
    curve->GetPoints(points);
    const ArrayOfCurveSpannedElements *spannedElements = curve->GetSpannedElements();

    BezierCurve bezier(points[0], points[1], points[2], points[3]);
    bezier.Rotate(-slurAngle, points[0]);
    bezier.CalculateControlPointOffset(doc, staff->m_drawingStaffSize);

    GetSpannedPointPositions(doc, spannedElements, bezier.p1, slurAngle, curveDir, staff->m_drawingStaffSize);

    bool adjusted = false;
    if (!spannedElements->empty()) {

        // Adjust the curvatur (control points are move)
        int adjustedHeight
            = AdjustSlurCurve(doc, spannedElements, bezier, curveDir, slurAngle, staff->m_drawingStaffSize, true);

        // The slur is being adjusted
        adjusted = true;
        bezier.Rotate(slurAngle, bezier.p1);

        // The adjustedHeight value is 0 if everything fits within the slur
        // If not we need to move its position
        bool ignoreAngle = false;
        if (adjustedHeight != 0) {
            bezier.SetControlHeight(adjustedHeight);
            // Use the adjusted control points for adjusting the position (p1, p2 and angle will be updated)
            ignoreAngle = AdjustSlurPosition(doc, curve, bezier, slurAngle, false);
            // Re-calculate the control points with the new height
            GetControlPoints(bezier, curveDir, ignoreAngle);

            points[0] = bezier.p1;
            points[1] = bezier.c1;
            points[2] = bezier.c2;
            points[3] = bezier.p2;
            curve->UpdateCurveParams(points, slurAngle, curve->GetThickness(), curveDir);
        }

        // If we still have spanning points then move the slur but now by forcing both sides to be move
        if (!spannedElements->empty()) {

            // First re-calcuate the spanning point positions
            GetSpannedPointPositions(doc, spannedElements, bezier.p1, slurAngle, curveDir, staff->m_drawingStaffSize);

            // Move it and force both sides to move
            AdjustSlurPosition(doc, curve, bezier, slurAngle, true);
            GetControlPoints(bezier, curveDir, ignoreAngle);
        }
    }

    if (adjusted) {
        points[0] = bezier.p1;
        points[1] = bezier.c1; // BoundingBox::CalcPositionAfterRotation(rotatedC1, slurAngle, *p1);
        points[2] = bezier.c2; // BoundingBox::CalcPositionAfterRotation(rotatedC2, slurAngle, *p1);
        points[3] = bezier.p2; // BoundingBox::CalcPositionAfterRotation(rotatedP2, slurAngle, *p1);
        curve->UpdateCurveParams(points, slurAngle, curve->GetThickness(), curveDir);
        // Since we are going to redraw-it reset its bounding box
        curve->BoundingBox::ResetBoundingBox();
    }

    return adjusted;
}

int Slur::AdjustSlurCurve(Doc *doc, const ArrayOfCurveSpannedElements *spannedElements, BezierCurve &bezierCurve,
    curvature_CURVEDIR curveDir, float angle, int staffSize, bool posRatio)
{
    Point bezier[4];
    bezier[0] = bezierCurve.p1;
    bezier[1] = bezierCurve.c1;
    bezier[2] = bezierCurve.c2;
    bezier[3] = bezierCurve.p2;

    int dist = abs(bezierCurve.p2.x - bezierCurve.p1.x);
    int currentHeight = abs(bezierCurve.c1.y - bezierCurve.p1.y);
    int maxHeight = 0;

    // 0.2 for avoiding / by 0 (below)
    float maxHeightFactor = std::max(0.2f, fabsf(angle));
    maxHeight = dist
        / (maxHeightFactor
            * (doc->GetOptions()->m_slurCurveFactor.GetValue()
                + 5)); // 5 is the minimum - can be increased for limiting curvature

    maxHeight = std::max(maxHeight, currentHeight);
    maxHeight = std::min(maxHeight, doc->GetDrawingOctaveSize(staffSize));

    maxHeight = currentHeight;
    if (!spannedElements->empty()) return maxHeight;

    return 0;
}

bool Slur::AdjustSlurPosition(
    Doc *doc, FloatingCurvePositioner *curve, BezierCurve &bezierCurve, float &angle, bool forceBothSides)
{
    bool isNotAdjustable = false;
    const int margin = doc->GetDrawingUnit(100);
    int maxShiftLeft = 0;
    int maxShiftRight = 0;
    std::tie(maxShiftLeft, maxShiftRight)
        = CalculateAdjustedSlurShift(curve, bezierCurve, margin, forceBothSides, isNotAdjustable);
    if (!maxShiftLeft && !maxShiftRight) return false;

    // If curve is cross staff and shifts are larger than current height of the control points - adjust control point
    // height to make sure that slur bends around the overlapping elements
    if (curve->IsCrossStaff() && !isNotAdjustable) {
        if ((maxShiftLeft > bezierCurve.GetLeftControlHeight())
            || (maxShiftRight > bezierCurve.GetRightControlHeight())) {
            if ((bezierCurve.c1.x < bezierCurve.p1.x) || (bezierCurve.c2.x > bezierCurve.p2.x)) return true;
            bezierCurve.SetLeftControlPointOffset(0.5 * bezierCurve.GetLeftControlPointOffset());
            bezierCurve.SetRightControlPointOffset(0.5 * bezierCurve.GetRightControlPointOffset());
            bezierCurve.SetLeftControlHeight(bezierCurve.GetLeftControlHeight() + 1.1 * maxShiftLeft);
            bezierCurve.SetRightControlHeight(bezierCurve.GetRightControlHeight() + 1.1 * maxShiftRight);
            if ((maxShiftLeft > maxShiftRight) && (maxShiftRight == 0)) {
                bezierCurve.SetLeftControlHeight(1.5 * bezierCurve.GetLeftControlHeight());
                bezierCurve.SetRightControlPointOffset(2 * bezierCurve.GetRightControlPointOffset());
                bezierCurve.SetRightControlHeight(0.5 * bezierCurve.GetRightControlHeight());
            }
            else if ((maxShiftRight > maxShiftLeft) && (maxShiftLeft == 0)) {
                bezierCurve.SetRightControlHeight(1.5 * bezierCurve.GetRightControlHeight());
                bezierCurve.SetLeftControlPointOffset(2 * bezierCurve.GetLeftControlPointOffset());
                bezierCurve.SetLeftControlHeight(0.5 * bezierCurve.GetLeftControlHeight());
            }
            if (std::abs(double(bezierCurve.p2.y - bezierCurve.p1.y) / double(bezierCurve.p2.x - bezierCurve.p1.x))
                > 2.0) {
                if (((curve->GetDir() == curvature_CURVEDIR_below) && (bezierCurve.p1.y > bezierCurve.p2.y))
                    || ((curve->GetDir() == curvature_CURVEDIR_above) && (bezierCurve.p1.y < bezierCurve.p2.y))) {
                    bezierCurve.SetLeftControlPointOffset(0.5 * bezierCurve.GetLeftControlPointOffset());
                }
                else if (((curve->GetDir() == curvature_CURVEDIR_above) && (bezierCurve.p1.y > bezierCurve.p2.y))
                    || ((curve->GetDir() == curvature_CURVEDIR_below) && (bezierCurve.p1.y < bezierCurve.p2.y))) {
                    bezierCurve.SetRightControlPointOffset(0.5 * bezierCurve.GetRightControlPointOffset());
                }
            }
            return true;
        }
        else {
            Point points[4];
            points[0] = bezierCurve.p1;
            points[1] = bezierCurve.c1;
            points[2] = bezierCurve.c2;
            points[3] = bezierCurve.p2;
            // Approximate bezier extrema and find time at which curve has highest/lowest Y value
            double time = 0.0;
            int yPos = 0;
            std::tie(time, yPos)
                = BoundingBox::ApproximateBezierExtrema(points, (curve->GetDir() == curvature_CURVEDIR_above));

            const double extremaShift = time - 0.5;
            const int relevantPoint = extremaShift < 0 ? bezierCurve.p1.y : bezierCurve.p2.y;
            Object *startMeasure
                = GetStart()->m_crossStaff ? GetStart()->m_crossStaff : GetStart()->GetFirstAncestor(MEASURE);
            Object *endMeasure = GetEnd()->m_crossStaff ? GetEnd()->m_crossStaff : GetEnd()->GetFirstAncestor(MEASURE);
            // We need to adjust curve based whether extrema time is higher/lower that 0.2 from the center (i.e. values
            // between [0.3; 0.7] are ok). For values that are lower than 0.3 we need to shift left control point base
            // to the right, and vice versa for the values above 0.7. This wouldn't exactly work for the slur that are
            // spanning over several measures, so we ignore them here
            if ((std::abs(extremaShift) > 0.2) && (startMeasure == endMeasure)
                && ((std::abs(relevantPoint - yPos) > (std::abs(bezierCurve.p1.y - bezierCurve.p2.y) / 50)))) {
                const int xDist = std::abs(bezierCurve.p1.x - bezierCurve.p2.x);
                if (extremaShift < 0) {
                    bezierCurve.SetLeftControlPointOffset(xDist / 2 - bezierCurve.GetRightControlPointOffset());
                    bezierCurve.SetRightControlPointOffset(0);
                }
                else {
                    bezierCurve.SetRightControlPointOffset(xDist / 2 - bezierCurve.GetLeftControlPointOffset());
                    bezierCurve.SetLeftControlPointOffset(0);
                }
                return true;
            }
            else {
                maxShiftLeft = maxShiftRight = 0.8 * std::min(maxShiftLeft, maxShiftRight);
                bezierCurve.p1.y += (curve->GetDir() == curvature_CURVEDIR_above) ? maxShiftLeft : -maxShiftLeft;
                bezierCurve.p2.y += (curve->GetDir() == curvature_CURVEDIR_above) ? maxShiftRight : -maxShiftRight;
                return false;
            }
        }
    }
    // otherwise it is normal slur - just move position of the start/end points up or down and recalculate angle
    else {
        // if slur is in the state where it cannot be adjusted (e.g. when there is too bid intersection with other
        // elements), then try to adjust one of the ends of the slur. Non-adjustable slur generally end up having one of
        // their ends just hanging over the staff (since we lift both ends of slur), so by doing following adjustment
        // it's possible to make those slurs look slightly better
        if (isNotAdjustable) {
            if (std::abs(maxShiftLeft) > std::abs(maxShiftRight)) {
                maxShiftRight /= 4;
            }
            else if (std::abs(maxShiftLeft) < std::abs(maxShiftRight)) {
                maxShiftLeft /= 4;
            }
        }
        bezierCurve.p1.y += (curve->GetDir() == curvature_CURVEDIR_above) ? maxShiftLeft : -maxShiftLeft;
        bezierCurve.p2.y += (curve->GetDir() == curvature_CURVEDIR_above) ? maxShiftRight : -maxShiftRight;

        angle = GetAdjustedSlurAngle(
            doc, bezierCurve.p1, bezierCurve.p2, curve->GetDir(), !curve->IsCrossStaff() && !isNotAdjustable);
        return false;
    }
}

template <typename Iterator, typename Comparator>
std::pair<double, double> Slur::CalculateAngleAndDistance(Iterator begin, Iterator end, Comparator comp, int xDistance)
{
    double largestAngle = 0.0;
    double largestDistance = 0.0;
    const bool isReverseOrder = is_reverse_iterator<Iterator>::value;
    // calculate distance and angles from all elements to the corresponding end of slur - start of the slur if iterator
    // to begin is used, and end of slur, if reverse iterator is used
    for (auto iter = std::next(begin); (iter != end)
         && (!isReverseOrder ? (iter->x <= begin->x + xDistance * 0.5) : (iter->x >= begin->x - xDistance * 0.5));
         ++iter) {
        const int xDiff = iter->x - begin->x;
        const int yDiff = iter->y - begin->y;
        const double distance = std::sqrt(xDiff * xDiff + yDiff * yDiff);
        const double angle = std::atan2(yDiff, xDiff);

        // There are two different cases for slurs that have above/below curvature, resulting in 4 total cases:
        // 1. angles (-pi; 0] - items in first half of "below" curve
        // 2. angles [0; pi) - items in first half of "above" curve
        // 3. angles (-2pi; pi] - items in second half of "below" curve
        // 4. angles [pi; 2pi) - items in second half of "above" curve
        // Depending on this, we either want to compare if element is lesser than 0 or otherwise. For angles in second
        // half of the slur, some non-overlapping elements are going to have angle that satisfies first condition
        // comp(angle, largestAngle) but actually is invalid angle (since it's not overlapping). For that reason, we
        // need to reverse comparison in second part, comparing whether 0 is lesser/greater than angle value, and not
        // vice versa.
        if ((largestAngle == 0.0 || comp(angle, largestAngle)) && (isReverseOrder ? comp(0, angle) : comp(angle, 0))) {
            largestAngle = angle;
            largestDistance = distance;
        }
    }

    return { largestAngle, largestDistance };
}

template <typename Comparator>
void Slur::AdjustSlurControlPoints(
    const std::set<Point, Comparator> &points, curvature_CURVEDIR curveDirection, BezierCurve &bezierCurve)
{
    // horizontal length of the slur
    const int xDistance = points.rbegin()->x - points.begin()->x;
    const int sign = curveDirection == curvature_CURVEDIR_above ? 1 : -1;
    double leftAngle = 0.0, leftDistance = 0.0, rightAngle = 0.0, rightDistance = 0.0;

    // slurs curving above
    if (sign > 0) {
        std::tie(leftAngle, leftDistance)
            = this->CalculateAngleAndDistance(points.begin(), points.end(), std::greater_equal{}, xDistance);
        std::tie(rightAngle, rightDistance)
            = this->CalculateAngleAndDistance(points.rbegin(), points.rend(), std::less_equal{}, xDistance);
    }
    // slurs curving below
    else {
        std::tie(leftAngle, leftDistance)
            = this->CalculateAngleAndDistance(points.begin(), points.end(), std::less_equal{}, xDistance);
        std::tie(rightAngle, rightDistance)
            = this->CalculateAngleAndDistance(points.rbegin(), points.rend(), std::greater_equal{}, xDistance);
    }

    if (leftAngle < 0.08) {
        leftAngle = 0.0;
    }
    double leftAdjust = std::abs(leftAngle);
    leftAngle -= 2 * leftAdjust;

    if (std::fmod(std::abs(rightAngle), M_PI) > M_PI - 0.08) {
        rightAngle = 0.0;
    }
    double rightAdjust = rightAngle < 0 ? M_PI + rightAngle : std::abs(rightAngle);    
    rightAngle -= 2 * rightAdjust;

    // Calculate new control points for left and right side of the slur if they have angles. To calculate X/Y positions
    // for the new control point, take into consideration angle and distance to the element that has the largest
    // overlap. Also, apply tension to the calculation, which increases depending on the steepness of the angle. If
    // angle is 0.0 it would mean that corresponding side does not need ajustmentes.
    if (leftAngle != 0.0) {
        const double xTension = 0.5;
        const double yTension = 1.5 + std::abs(std::sin(leftAngle));
        const int adjustedC1X = points.begin()->x + leftDistance * xTension * std::abs(std::cos(leftAngle));
        const int adjustedC1Y = points.begin()->y + sign * leftDistance * yTension * std::abs(std::sin(leftAngle));
        bezierCurve.c1.x = adjustedC1X;
        bezierCurve.c1.y = adjustedC1Y;
    }
    if (rightAngle != 0.0) {
        const double xTension = 0.5;
        const double yTension = 1.5 + std::abs(std::sin(rightAngle));
        const int adjustedC2X = points.rbegin()->x - rightDistance * xTension * std::abs(std::cos(rightAngle));
        const int adjustedC2Y = points.rbegin()->y + sign * rightDistance * yTension * std::abs(std::sin(rightAngle));
        bezierCurve.c2.x = adjustedC2X;
        bezierCurve.c2.y = adjustedC2Y;
    }
}

bool Slur::AlterSlurPosition(Doc *doc, FloatingCurvePositioner *curve, BezierCurve &bezierCurve)
{
    const ArrayOfCurveSpannedElements *spannedElements = curve->GetSpannedElements();

    auto xCompare = [](const Point a, const Point &b) { return a.x < b.x; };
    std::set<Point, decltype(xCompare)> points(xCompare);
    points.insert(bezierCurve.p1);
    points.insert(bezierCurve.p2);

    for (auto spannedElement : *spannedElements) {
        const int y = curve->GetDir() == curvature_CURVEDIR_above ? spannedElement->m_boundingBox->GetContentTop()
                                                            : spannedElement->m_boundingBox->GetContentBottom();
        const int x = spannedElement->m_boundingBox->GetDrawingX();
        if (x > points.begin()->x && x < points.rbegin()->x) {
            points.insert(Point(spannedElement->m_boundingBox->GetDrawingX(), y));
        }     
    }

    this->AdjustSlurControlPoints(points, curve->GetDir(), bezierCurve);

    return true;
}

std::pair<int, int> Slur::CalculateAdjustedSlurShift(FloatingCurvePositioner *curve, const BezierCurve &bezierCurve,
    int margin, bool forceBothSides, bool &isNotAdjustable)
{
    int maxShiftLeft = 0;
    int maxShiftRight = 0;

    int dist = std::abs(bezierCurve.p2.x - bezierCurve.p1.x);
    float posXRatio = 1.0;

    const ArrayOfCurveSpannedElements *spannedElements = curve->GetSpannedElements();
    // Actually nothing to do
    if (spannedElements->empty()) return { 0, 0 };

    // Find max/min value for the spanning elements within the slur
    int extremeY = VRV_UNSET;
    std::for_each(spannedElements->begin(), spannedElements->end(),
        [dir = curve->GetDir(), &extremeY](CurveSpannedElement *element) {
            if (dir == curvature_CURVEDIR_above) {
                const int y = element->m_boundingBox->GetSelfTop();
                extremeY = (extremeY == VRV_UNSET) ? y : std::max(y, extremeY);
            }
            else {
                const int y = element->m_boundingBox->GetSelfBottom();
                extremeY = (extremeY == VRV_UNSET) ? y : std::min(y, extremeY);
            }
        });
    const int leftPointMaxHeight = extremeY - bezierCurve.p1.y;
    const int rightPointMaxHeight = extremeY - bezierCurve.p2.y;

    for (auto spannedElement : *spannedElements) {

        if (spannedElement->m_discarded) {
            continue;
        }

        bool discard = false;
        int intersection = curve->CalcAdjustment(spannedElement->m_boundingBox, discard, margin);

        if (discard == true) {
            spannedElement->m_discarded = true;
            continue;
        }

        if (intersection == 0) {
            continue;
        }

        // In case of cross-staff, intersection with the note that is in the staff directly under the start/end point
        // might result in too big curve or strange slurs. If intersection is bigger than maximum height of the
        // cross-staff slur, we should ignore it.
        if (curve->IsCrossStaff()
            && (intersection > std::max(std::abs(rightPointMaxHeight), std::abs(leftPointMaxHeight)))) {
            continue;
        }

        int xLeft = std::max(bezierCurve.p1.x, spannedElement->m_boundingBox->GetSelfLeft());
        int xRight = std::min(bezierCurve.p2.x, spannedElement->m_boundingBox->GetSelfRight());
        int xMiddle = xLeft + ((xRight - xLeft) / 2);
        int posX = xMiddle - bezierCurve.p1.x;

        // Weight the desired height according to the x position on the other side
        posXRatio = 1.0;
        bool leftPoint = true;
        if (posX > dist / 2) {
            posX = bezierCurve.p2.x - xMiddle;
            leftPoint = false;
        }
        if (dist != 0) posXRatio = (float)posX / ((float)dist / 2.0);

        if (intersection > 0) {
            int leftShift = (forceBothSides || leftPoint) ? intersection : intersection * posXRatio;
            int rightShift = (forceBothSides || !leftPoint) ? intersection : intersection * posXRatio;
            // Keep the maximum shift on the left and right
            maxShiftLeft = leftShift > maxShiftLeft ? leftShift : maxShiftLeft;
            maxShiftRight = rightShift > maxShiftRight ? rightShift : maxShiftRight;
        }

        // if intersection happens on the start/end of the slur, make sure that there is enough place for proper slur to
        // be drawn. If intersection is too large, cross-staff slurs should not be drawn with adjusted angles to avoid
        // extreme cases
        const float distanceRatio = float(xMiddle - bezierCurve.p1.x) / float(dist);
        if (((distanceRatio < 0.1) && (intersection > leftPointMaxHeight / 2))
            || ((distanceRatio > 0.9) && (intersection > rightPointMaxHeight / 2)))
            isNotAdjustable = true;
    }
    if (curve->GetDir() == curvature_CURVEDIR_above) {
        if (leftPointMaxHeight + margin < 0) maxShiftLeft = 0;
        if (rightPointMaxHeight + margin < 0) maxShiftRight = 0;
    }
    else {
        if (leftPointMaxHeight + margin > 0) maxShiftLeft = 0;
        if (rightPointMaxHeight + margin > 0) maxShiftRight = 0;
    }

    return { maxShiftLeft, maxShiftRight };
    // Unrotated the slur
    //*p2 = BoundingBox::CalcPositionAfterRotation(*p2, (*angle), *p1)
}

float Slur::GetAdjustedSlurAngle(Doc *doc, Point &p1, Point &p2, curvature_CURVEDIR curveDir, bool withPoints)
{
    float slurAngle = (p1 == p2) ? 0 : atan2(p2.y - p1.y, p2.x - p1.x);
    float maxSlope = (float)doc->GetOptions()->m_slurMaxSlope.GetValue() * M_PI / 180.0;

    // For slurs without spanning points allow for double angle
    // This normally looks better with slurs with two notes and high ambitus
    if (!withPoints) maxSlope *= 2.0;

    // the slope of the slur is high and needs to be corrected
    if (fabs(slurAngle) > maxSlope) {
        int side = (p2.x - p1.x) * sin(maxSlope) / sin(M_PI / 2 - maxSlope);
        if (p2.y > p1.y) {
            if (curveDir == curvature_CURVEDIR_above)
                p1.y = p2.y - side;
            else
                p2.y = p1.y + side;
            slurAngle = maxSlope;
        }
        else {
            if (curveDir == curvature_CURVEDIR_above)
                p2.y = p1.y - side;
            else
                p1.y = p2.y + side;
            slurAngle = -maxSlope;
        }
    }

    return slurAngle;
}

void Slur::GetControlPoints(BezierCurve &bezier, curvature_CURVEDIR curveDir, bool ignoreAngle)
{
    const float slurAngle = (bezier.p2 == bezier.p1) ? 0 : atan2(bezier.p2.y - bezier.p1.y, bezier.p2.x - bezier.p1.x);
    if ((slurAngle != 0.0) && !ignoreAngle) {
        bezier.p2 = BoundingBox::CalcPositionAfterRotation(bezier.p2, -slurAngle, bezier.p1);
        // It should not be the case but we do need to avoid recursive calls whatever the effect in the resutls
        if (bezier.p2.y != bezier.p1.y) bezier.p2.y = bezier.p1.y;
        GetControlPoints(bezier, curveDir);
        bezier.Rotate(slurAngle, bezier.p1);
        return;
    }

    bezier.c1.x = bezier.p1.x + bezier.GetLeftControlPointOffset();
    bezier.c2.x = bezier.p2.x - bezier.GetRightControlPointOffset();

    if (curveDir == curvature_CURVEDIR_above) {
        bezier.c1.y = bezier.p1.y + bezier.GetLeftControlHeight();
        bezier.c2.y = bezier.p2.y + bezier.GetRightControlHeight();
    }
    else {
        bezier.c1.y = bezier.p1.y - bezier.GetLeftControlHeight();
        bezier.c2.y = bezier.p2.y - bezier.GetRightControlHeight();
    }
}

void Slur::GetSpannedPointPositions(Doc *doc, const ArrayOfCurveSpannedElements *spannedElements, Point p1, float angle,
    curvature_CURVEDIR curveDir, int staffSize)
{
    /*
    for (auto &spannedElement : *spannedElements) {
        int margin = 1;
        // Not sure if it is better to add the margin before or after the rotation...
        // if (up) p.y += m_doc->GetDrawingUnit(staffSize) * 2;
        // else p.y -= m_doc->GetDrawingUnit(staffSize) * 2;
        itPoint->second.second = BoundingBox::CalcPositionAfterRotation(itPoint->second.first, -angle, p1);
        // This would add it after
        if (curveDir == curvature_CURVEDIR_above) {
            itPoint->second.second.y += doc->GetDrawingUnit(staffSize) * margin;
        }
        else {
            itPoint->second.second.y -= doc->GetDrawingUnit(staffSize) * margin;
        }
    }
    */
}

//----------------------------------------------------------------------------
// Functors methods
//----------------------------------------------------------------------------

int Slur::ResetDrawing(FunctorParams *functorParams)
{
    // Call parent one too
    ControlElement::ResetDrawing(functorParams);

    m_drawingCurvedir = curvature_CURVEDIR_NONE;
    // m_isCrossStaff = false;

    return FUNCTOR_CONTINUE;
}

} // namespace vrv
