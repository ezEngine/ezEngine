# Renderer: Switch the Engine to Inverse Depth

## Summary

Replace the engine's conventional depth rendering with inverse depth rendering. This is a fixed engine-wide convention. There is no runtime setting or shader permutation for switching between conventional and inverse depth.


The engine now consistently uses:

| Behavior | Previous | New |
| --- | --- | --- |
| Near-plane device depth | `0` | `1` |
| Far-plane device depth | `1` | `0` |
| Default depth clear | `1` | `0` |
| Default depth comparison | `Less` | `Greater` |
| Material depth comparison | `LessEqual` | `GreaterEqual` |
| Closer device depth | Smaller value | Larger value |
| Farther device depth | Larger value | Smaller value |


## API Behavior Changes

### Cameras and Projection Matrices

- `ezCamera::SetCameraMode()` continues to take physical near and far distances with `near < far`.
- `ezCamera::GetNearPlane()` and `GetFarPlane()` continue to return those physical distances.
- Projection matrices returned by `ezCamera::GetProjectionMatrix()` now map the physical near plane to device depth `1` and the physical far plane to device depth `0`.
- `ezCamera::SetStereoProjection()` now expects matrices that already use inverse depth. Supplying conventional projection matrices produces incorrect depth testing and reconstruction.
- Stereo projection matrices are converted between clip-space depth ranges when requested, but their near/far mapping is not inverted by the camera.

The low-level `ezGraphicsUtils::CreatePerspectiveProjectionMatrix*()` and `CreateOrthographicProjectionMatrix()` functions still determine the depth direction from their Z-plane arguments. Engine rendering code that constructs a projection directly must pass the physical far distance as `fNearZ` and the physical near distance as `fFarZ`, for example:

```cpp
const ezMat4 projection = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(fov, aspectRatio, farPlane, nearPlane);
```

### Screen and World Conversion

Depth values passed to or returned by screen-space conversion APIs now follow inverse device depth:

- `ezGraphicsUtils::ConvertWorldPosToScreenPos()` returns `z = 1` at the near plane and `z = 0` at the far plane.
- `ezGraphicsUtils::ConvertScreenPosToWorldPos()` expects that same mapping.
- To construct a picking ray at the near plane, use screen-space `z = 1`.
- `ezViewData::ComputePickingRay()` and `ComputeWorldSpacePos()` use the inverse-depth near plane.
- `ezGraphicsUtils::ComputeInterpolatedFrustumPlane` retains the same meaning as before: `fLerpFactor = 0` selects the physical near plane.

Code that stores or compares the `z` component returned by these APIs must not assume that depth increases with distance.

### Rendering Defaults

The following defaults now match inverse depth:

- `ezGALDepthStencilStateCreationDescription::m_DepthTestFunc` defaults to `ezGALCompareFunc::Greater`.
- `ezGALRenderingSetup::SetClearDepth()` defaults to `0`.
- `ezRenderGraphPassBuilder::SetClearDepth()` defaults to `0`.
- `ezGALCommandEncoder::Clear()` defaults to a depth clear value of `0`.
- Source and history render-pipeline passes default their depth clear value to `0`.
- Shader material states default to `CompareFunc_GreaterEqual`.
- Visual Shader render-state defaults use `CompareFunc_GreaterEqual`.

Custom passes that relied on omitted arguments producing a clear value of `1` or a `Less` comparison must update their explicit state. Specialized passes that intentionally use another depth convention must continue to specify every clear and comparison value explicitly.

The shader permutation cache version was incremented to recompile cached render states with the inverse-depth defaults. Loading an outdated permutation now forces recompilation even when its shader source dependencies have not changed.

### Depth Bias API Behavior Change

**Explicit raw rasterizer depth biases change their physical direction under inverse depth. They are not automatically migrated.**

Low level render-state `m_iDepthBias`, `m_fSlopeScaledDepthBias`, and `m_fDepthBiasClamp` values in `ezGALRasterizerStateCreationDescription`, are passed to the graphics backend without sign compensation. To preserve the physical direction of an existing bias, these values need to be negated.

| Raw rasterizer setting | Conventional depth | Inverse depth |
| --- | --- | --- |
| Positive constant or slope-scaled bias | Pushes away from the camera | Pulls toward the camera |
| Negative constant or slope-scaled bias | Pulls toward the camera | Pushes away from the camera |

Higher-level controls retain their previous user-facing conventions:

- `GetVertexDepthBias()`: positive pushes away from the camera; the shared vertex shader compensates for inverse depth internally.
- Mesh-decal material `DepthBias`: positive pulls toward the camera, because the material returns the negated parameter from `GetVertexDepthBias()`. This material parameter is distinct from the raw rasterizer render-state setting with the same name.
- Light `ConstantBias` and `SlopeBias`: retain their shadow-artifact reduction behavior. Constant bias compensates for inverse depth; slope bias remains a world-space normal offset.
- Lens-flare `OcclusionDepthOffset`: positive moves the occlusion sample away from the camera in world space.
- Mesh `SortingDepthOffset`: positive increases sorting distance only; it does not modify depth-buffer values.

### Serialized Render Pipelines

`ezSourcePass` and `ezHistorySourcePass` type versions were incremented. Existing serialized `ClearDepth` values are migrated by replacing `depth` with `1 - depth`, both through graph patches and binary deserialization.

This preserves the semantic meaning of existing clear values when older render-pipeline assets are loaded.

### Shader Depth Semantics

`CameraConstantsAccess.h` now provides semantic helpers for the fixed depth convention:

```hlsl
GetNearPlaneDeviceDepth()       // 1
GetFarPlaneDeviceDepth()        // 0
IsDeviceDepthCloser(a, b)       // a > b
IsDeviceDepthFarther(a, b)      // a < b
SelectClosestDeviceDepth(a, b)  // max(a, b)
SelectFarthestDeviceDepth(a, b) // min(a, b)
ReconstructCameraPositionFromDeviceDepth(...)
ReconstructWorldPositionFromDeviceDepth(...)
ConvertDeviceDepthToLinearDepth(float deviceDepth)
```

Function renames:

| Previous name | New name | Location / notes |
| --- | --- | --- |
| `LinearizeZBufferDepth()` | `ConvertDeviceDepthToLinearDepth()` | Remains in `Common/CameraConstantsAccess.h`. |
| `FastScreenCoordToViewSpaceLinear()` | `ReconstructPerspectiveCameraPositionFromLinearDepth()` | Moves from `Pipeline/Utils.h` to `Common/CameraConstantsAccess.h`. |
| `FastScreenCoordToViewSpace()` | `ReconstructPerspectiveCameraPositionFromDeviceDepth()` | Moves from `Pipeline/Utils.h` to `Common/CameraConstantsAccess.h`. |
| `ReconstructViewSpaceNormal()` | `ReconstructCameraNormalFromDeviceDepth()` | Remains in `Pipeline/Utils.h` with the neighborhood sampling utilities. |

The perspective position helpers and neighborhood normal reconstruction retain their centered-perspective implementations. The new general inverse-matrix helper `ReconstructCameraPositionFromDeviceDepth()` remains separate; the renames do not change projection support or calculations.

Custom shaders should:

- Use the semantic helpers instead of hardcoded `0`, `1`, `min`, `max`, `<`, or `>` when operating on device depth.
- Write `GetFarPlaneDeviceDepth()` for background or sky geometry.
- Use `Greater` or `GreaterEqual` depth states for ordinary geometry.
- Use comparison samplers configured with `Greater` for inverse-depth shadow maps.
- Reverse the direction of manual device-depth biases where necessary: increasing device depth moves a value closer to the camera.
- Keep device depth and linear camera-space depth distinct. Reductions over inverse device depth select farther values with `min`, while reductions over positive linear depth select farther values with `max`.

> NOTE: This means that min/max logic is now inversed between linear depth and device depth. I.e. see DownscaleDepth.ezShader for an example.

## Engine Integration Changes

- Directional, point, and spot shadows use inverse-depth projections, clears, comparisons, cascade selection, fade parameters, and bias direction.
- MSAA depth resolve selects the farthest inverse-depth sample.
- SSAO, LSAO, screen-space shadows, selection highlighting, refraction, lens flares, sky rendering, and depth-aware blur/downscale passes use inverse-depth semantics.
- Editor picking, marquee selection, plane interaction, and gizmo rays start at inverse-depth near values.
- OpenXR projection and submitted depth-layer near/far metadata follow the engine's inverse-depth mapping.
- Dummy XR, baking, RmlUi, standalone samples, and direct projection-matrix users construct inverse-depth projections.
- The software rasterizer explicitly converts the camera's inverse-depth projection back to the conventional depth mapping it requires internally.

## Tests

Coverage was updated or added for:

- Perspective, orthographic, stereo, and both clip-space depth ranges.
- Camera near/far device-depth mapping.
- Screen/world conversion and frustum extraction with inverse depth.
- Default depth comparison and clear values.
- Render-graph depth clears.
- Shader depth reconstruction and renderer image behavior.
