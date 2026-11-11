#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

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
  MaskedOcclusionCullingAVX512, ///< Intel's Masked Occlusion Culling library (native SIMD), AVX-512 instruction set.
  GenericMOC                    ///< Intel's MOC algorithm ported to ezSimdVec4f (portable).
};

/// The SIMD variants of the native MaskedOcclusionCulling implementations, ordered from the lowest to the highest instruction set.
inline constexpr ezRasterizerImplementation ezMOCInstructionSets[] = {
  ezRasterizerImplementation::MaskedOcclusionCullingSSE2,
  ezRasterizerImplementation::MaskedOcclusionCullingSSE41,
  ezRasterizerImplementation::MaskedOcclusionCullingAVX2,
  ezRasterizerImplementation::MaskedOcclusionCullingAVX512};

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
  return ezMOCInstructionSetIndex(impl) >= 0 || impl == ezRasterizerImplementation::GenericMOC;
}

class EZ_RENDERERCORE_DLL ezRasterizerView final
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRasterizerView);

public:
  /// \param bUseOptimized If true and the platform supports it, uses the AVX2-optimized rasterizer. Otherwise uses GenericMOC when available.
  explicit ezRasterizerView(bool bUseOptimized = true);

  /// On platforms without the native x86 rasterizers, requests fall back to GenericMOC when MOC support is enabled.
  explicit ezRasterizerView(ezRasterizerImplementation impl);
  ~ezRasterizerView();

  /// Changes the resolution of the view. Has to be called at least once before starting to render anything.
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

  /// Writes an RGBA8 representation of the depth values to targetBuffer.
  ///
  /// The buffer must be large enough for the chosen resolution.
  void ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const;

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

  bool m_bAnyOccludersRasterized = false;
  const ezCamera* m_pCamera = nullptr;
  ezUInt32 m_uiResolutionX = 0;
  ezUInt32 m_uiResolutionY = 0;
  float m_fAspectRation = 1.0f;

  ezRasterizerImplementation m_Implementation = ezRasterizerImplementation::ThirdParty;

  ezUniquePtr<Rasterizer> m_pRasterizer;
  MaskedOcclusionCulling* m_pMOC = nullptr;

  struct Instance
  {
    ezTransform m_Transform;
    const ezRasterizerObject* m_pObject;
  };

  ezDeque<Instance> m_Instances;
  ezMat4 m_mViewProjection;
};

class ezRasterizerViewPool
{
public:
  ezRasterizerView* GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio, bool bUseOptimized = true);
  ezRasterizerView* GetRasterizerView(ezUInt32 uiWidth, ezUInt32 uiHeight, float fAspectRatio, ezRasterizerImplementation impl);
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
