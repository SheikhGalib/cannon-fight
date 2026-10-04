#ifndef DIMENSIONS_H
#define DIMENSIONS_H

// Every measurement of the cannon, in metres, in one place.
//
// Wheel.cpp, Carriage.cpp, Shaft.cpp and Main.cpp all read from here, which
// is what keeps the parts fitting together: the wheels sit exactly on the
// axle because BOTH the wheel placement and the axle geometry are derived
// from WheelRadius / WheelTrack, rather than each file guessing its own
// numbers. Change one value here and the whole assembly follows.
//
// Frame convention used everywhere in this project:
//     +X  forward   (the direction the barrel points at 0 degrees elevation)
//     +Y  up
//     +Z  to the right (the wheel axle runs along Z)
//
// These values are carried over from the original Python version
// (legacy/cannon.py), which is why the proportions match docs/images/01_overview.png.
namespace Dim {

    // --- wheels ----------------------------------------------------------
    inline constexpr float WheelRadius = 0.62f;  // outer edge of the iron tire
    inline constexpr float WheelWidth  = 0.16f;  // how thick the wheel is (along Z)
    inline constexpr float WheelTrack  = 0.68f;  // |Z| of each wheel, i.e. half the axle span
    inline constexpr int   SpokeCount  = 10;

    inline constexpr float FelloeInner = 0.44f;   // wooden rim: inner edge
    inline constexpr float FelloeOuter = 0.555f;  // wooden rim: outer edge = where the tire starts
    inline constexpr float SpokeInner  = 0.12f;   // spokes run from here...
    inline constexpr float SpokeOuter  = 0.46f;   // ...to here (slightly into the felloe)
    inline constexpr float SpokeThick  = 0.075f;
    inline constexpr float HubRadius   = 0.135f;
    inline constexpr float HubLength   = 0.32f;

    // --- carriage: the two trail beams ------------------------------------
    // A beam is one long box running from its front end (up near the axle) down
    // to its rear end (resting on the ground) - that tilt is what makes the
    // classic field-carriage silhouette.
    inline constexpr float BeamFrontX =  0.70f, BeamFrontY = 0.85f;
    inline constexpr float BeamRearX  = -2.35f, BeamRearY  = 0.17f;
    inline constexpr float BeamThick  =  0.20f;  // height of the beam box (Y)
    inline constexpr float BeamWide   =  0.15f;  // width of the beam box (Z)
    inline constexpr float BeamZ      =  0.32f;  // |Z| of each beam

    // --- barrel ------------------------------------------------------------
    // The trunnion pivot: the point the barrel tips around when elevating.
    // It sits above and slightly forward of the axle, carried by the cheeks.
    inline constexpr float PivotX = 0.10f, PivotY = 1.15f, PivotZ = 0.0f;

    inline constexpr float BarrelBreechX = -0.45f;  // rearmost point of the tube, in barrel space
    inline constexpr float MuzzleX       =  1.70f;  // the muzzle face, in barrel space
    inline constexpr float BoreRadius    =  0.075f;

}

#endif
