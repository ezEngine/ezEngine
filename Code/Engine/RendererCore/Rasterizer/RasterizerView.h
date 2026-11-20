#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezCVarEvent;

class Rasterizer;
class MaskedOcclusionCulling;
class ezRasterizerObject;
class ezColorLinearUB;
class ezCamera;
class ezSimdBBox;

/// Selects which software rasterizer implementation to use.
enum class ezRasterizerImplementation
{
  ThirdParty,                   ///< AVX2-optimized rasterizer.
  MaskedOcclusionCullingSSE2,   ///< Intel's Masked Occlusion Culling library (native SIMD), SSE2 instruction set.
  MaskedOcclusionCullingSSE41,  ///< Intel's Masked Occlusion Culling library (native SIMD), SSE4.1 instruction set.
  MaskedOcclusionCullingAVX2,   ///< Intel's Masked Occlusion Culling library (native SIMD), AVX2 instruction set.
  MaskedOcclusionCullingEZ,     ///< Intel's MOC algorithm ported to ezSimdVec4f (portable).
  Auto
};

/// Returns whether a concrete implementation can run on this build and CPU without falling back.
///
/// Auto is a selection policy, not an implementation, and returns false.
EZ_RENDERERCORE_DLL bool ezIsRasterizerImplementationSupported(ezRasterizerImplementation impl);

/// The SIMD variants of the native MaskedOcclusionCulling implementations, ordered from the lowest to the highest instruction set.
inline constexpr ezRasterizerImplementation ezMOCInstructionSets[] = {
  ezRasterizerImplementation::MaskedOcclusionCullingSSE2,
  ezRasterizerImplementation::MaskedOcclusionCullingSSE41,
  ezRasterizerImplementation::MaskedOcclusionCullingAVX2};

/// Index into ezMOCInstructionSets of a native MaskedOcclusionCulling implementation, or -1 for any other implementation.
/// The indices match the values of MaskedOcclusionCulling::Implementation, so the result can be cast to that type.
constexpr ezInt32 ezMOCInstructionSetIndex(ezRasterizerImplementation impl)
{
  for (ezInt32 i = 0; i < (ezInt32)sizeof(ezMOCInstructionSets) / sizeof(ezMOCInstructionSets[0]); ++i)
  {
    if (ezMOCInstructionSets[i] == impl)
      return i;
  }

  return -1;
}

/// True for the native MaskedOcclusionCulling implementations and for the portable GenericMOC port.
constexpr bool ezIsMaskedOcclusionCullingImplementation(ezRasterizerImplementation impl)
{
  return ezMOCInstructionSetIndex(impl) >= 0 || impl == ezRasterizerImplementation::MaskedOcclusionCullingEZ;
}

class EZ_RENDERERCORE_DLL ezRasterizerView final
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRasterizerView);

public:
  /// On platforms without the native x86 rasterizers, requests fall back to GenericMOC when MOC support is enabled.
  ///
  /// A view created with `Auto` keeps following `Spatial.Occlusion.Implementation` whenever that CVar changes.
  /// Views created with a concrete implementation are not affected by it.
  explicit ezRasterizerView(ezRasterizerImplementation impl);
  ~ezRasterizerView();

  /// Changes the resolution of the view. Has to be called at least once before starting to render anything.
  /// Both dimensions must be multiples of 4; individual rasterizers may have stricter requirements.
  void SetResolution(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio);

  ezUInt32 GetResolutionX() const { return m_uiResolutionX; }
  ezUInt32 GetResolutionY() const { return m_uiResolutionY; }

  bool IsUsingOptimizedRasterizer() const
  {
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
    return m_Implementation == ezRasterizerImplementation::ThirdParty;
#else
    return false;
#endif
  }
  /// The rasterizer implementation currently in use. For a view created with `Auto` this can change at runtime.
  ezRasterizerImplementation GetImplementation() const { return m_Implementation; }

  /// The instruction set that is actually in use for a MaskedOcclusionCulling view.
  ///
  /// Requesting an instruction set that the current CPU doesn't support falls back to a lower one, so this
  /// can differ from GetImplementation(). For every other implementation this simply returns GetImplementation().
  ezRasterizerImplementation GetMOCInstructionSet() const;

  /// Prepares the view to rasterize a new scene.
  void BeginScene();

  /// Finishes rasterizing the scene. Visibility queries only work after this.
  void EndScene();

  /// Writes a grayscale image of the depth buffer to targetBuffer, starting with the bottom row.
  ///
  /// The buffer must be large enough for the chosen resolution.
  /// Brightness falls off linearly with view distance, from white at the camera near plane to black at
  /// `Spatial.Occlusion.ReadbackFarDistance` (100 meters by default) and beyond.
  /// Uncovered pixels are transparent black (all channels zero).
  /// MOC stores inverse clip W, so with orthographic cameras all covered pixels are white.
  /// With QUICK_MASK enabled, MOC only shows the reference layer that occlusion queries use, so subtiles that are only partially
  /// covered can appear uncovered or farther away than the rasterized geometry.
  /// The depth is decoded per pixel, so this is only meant for debugging.
  void ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const;

  /// Writes the raw depth value of every pixel to targetBuffer, row by row starting at the top of the image.
  ///
  /// Only supported by the MaskedOcclusionCulling implementations, returns EZ_FAILURE otherwise.
  /// Values are 1 / view space distance, pixels that no occluder covered are negative.
  /// The depth is decoded per pixel, so this is only meant for debugging and tests.
  ezResult ReadBackDepth(ezArrayPtr<float> targetBuffer) const;

  /// Sets the camera from which to extract the rendering position, direction and field-of-view.
  void SetCamera(const ezCamera* pCamera)
  {
    m_pCamera = pCamera;
  }

  /// Adds an object as an occluder to the scene. Once all occluders have been rasterized, visibility queries can be done.
  void AddObject(const ezRasterizerObject* pObject, const ezTransform& transform)
  {
    auto& inst = m_Instances.ExpandAndGetRef();
    inst.m_pObject = pObject;
    inst.m_Transform = transform;
  }

  /// Checks whether a box would be visible, or is fully occluded by the existing scene geometry.
  ///
  /// Note: This only works after EndScene().
  bool IsVisible(const ezSimdBBox& aabb) const;

  /// Wether any occluder was actually added and also rasterized. If not, no need to do any visibility checks.
  bool HasRasterizedAnyOccluders() const
  {
    return m_bAnyOccludersRasterized;
  }

private:
  void SortObjectsFrontToBack();
  void RasterizeObjects(ezUInt32 uiMaxObjects);
  void UpdateViewProjectionMatrix();
  void ApplyModelViewProjectionMatrix(const ezTransform& modelTransform);
  bool IsVisibleMOC(const ezBoundingBox& bounds, const ezMat4& mModelViewProjection) const;

  /// Switches to a different rasterizer implementation and recreates the rasterizer instance for the current resolution.
  void ApplyImplementation(ezRasterizerImplementation impl);

  /// Creates the rasterizer instance for m_Implementation and the current resolution, reusing an existing instance if it already matches.
  void CreateRasterizer();

  /// Destroys the rasterizer instance, its type is determined by m_Implementation.
  void DestroyRasterizer();

  void OnOcclusionImplementationChanged(const ezCVarEvent& event);

  bool m_bAnyOccludersRasterized = false;
  const ezCamera* m_pCamera = nullptr;
  ezUInt32 m_uiResolutionX = 0;
  ezUInt32 m_uiResolutionY = 0;
  float m_fAspectRation = 1.0f;

  ezRasterizerImplementation m_Implementation = ezRasterizerImplementation::ThirdParty;

  /// Subscription to `Spatial.Occlusion.Implementation`, only set for views created with `Auto`. Zero when not subscribed.
  ezEventSubscriptionID m_uiImplementationSubscription = 0;

  ezUniquePtr<Rasterizer> m_pRasterizer;
  ezUniquePtr<MaskedOcclusionCulling> m_pMOC;

  struct Instance
  {
    ezTransform m_Transform;
    const ezRasterizerObject* m_pObject;
    float m_fSortDistanceSq = 0.0f;
    float m_fSortOriginDistanceSq = 0.0f;
  };

  ezDynamicArray<Instance> m_Instances;

  /// For MOC this is the unmodified engine view-projection. For ThirdParty, row 2 is flipped to produce near depth 0 and far depth 1.
  ezMat4 m_mViewProjection;

  /// Clip space w of the MOC near clip plane, 0 for orthographic cameras.
  float m_fNearClipW = 0.0f;

  /// Used by ReadBackFrame to convert the stored depth to a view distance. Depth row of the unmodified engine projection.
  bool m_bReadbackPerspective = true;
  float m_fReadbackDepthRowZ = 0.0f;
  float m_fReadbackDepthRowW = 0.0f;
  float m_fReadbackNearDistance = 0.0f;
};

class ezRasterizerViewPool
{
public:
  ezRasterizerView* GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio);
  void ReturnRasterizerView(ezRasterizerView* pView);

private:
  struct PoolEntry
  {
    bool m_bInUse = false;
    ezUniquePtr<ezRasterizerView> m_pRasterizerView;
  };

  ezMutex m_Mutex;
  ezDeque<PoolEntry> m_Entries;
};
