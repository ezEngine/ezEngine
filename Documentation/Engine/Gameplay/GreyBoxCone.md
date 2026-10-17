# GreyBox Cone

Select **Cone** in the built-in **Grey Boxing** component's **Shape** property. No additional plugin is required. The cone points along local +Z and uses the existing six size extents, material, color, collision and occluder options.

- **BaseRadiusScale** (0–4, default 1): bottom radius relative to half the X/Y size.
- **TopRadiusScale** (0–4, default 0): zero makes a pointed cone; nonzero makes a frustum. A zero base and nonzero top make an inverted cone.
- **Sides** (3–128, default 32): circumference subdivisions. Four sides produce an axis-aligned square pyramid that fills the X/Y extents at radius scale 1.
- **HeightSegments** (1–64, default 8): height subdivisions. Increase this to show the profile curve.
- **ProfileCurve** (-0.95–1, default 0): negative values pinch the middle; positive values bulge it. End radii stay fixed.
- **SmoothShading** (default true): smooth side normals, or flat normals per side face. Caps retain hard edges.

The size extents describe the reference volume: radius scales above 1 and positive profile curvature can extend beyond it. Mesh bounds, collision and occlusion use the generated surface. Degenerate dimensions or two zero radii use the existing tiny-box fallback. Cone settings only appear when Shape is Cone and do not alter other GreyBox shapes.

Existing GreyBox enum values are unchanged. Component serialization version 8 appends cone settings; older scenes retain the defaults. This change does not migrate the separate experimental GreyBoxConeComponent; create a native GreyBox component and copy the cone settings if migrating such a scene.

## Verification

Build `GreyBoxTest` and `EditorPluginScene`, then run `GreyBoxTest -nogui -nosave`. Tests cover cone/frustum topology, normals, pyramid bounds, profile curvature, cache keys, clamping, degenerate fallback, and world serialization. The geometry test does not require a GPU.
