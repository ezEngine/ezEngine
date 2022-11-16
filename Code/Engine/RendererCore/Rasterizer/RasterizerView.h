#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/ArrayPtr.h>
#include <RendererCore/RendererCoreDLL.h>

class Rasterizer;
class ezRasterizerObject;
class ezColorLinearUB;
class ezCamera;
class ezSimdBBox;

class EZ_RENDERERCORE_DLL ezRasterizerView final
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRasterizerView);

public:
  ezRasterizerView();
  ~ezRasterizerView();

  /// \brief Changes the resolution of the view. Has to be called at least once before starting to render anything.
  void SetResolution(ezUInt32 width, ezUInt32 height, float fAspectRatio);

  ezUInt32 GetResolutionX() const { return m_uiResolutionX; }
  ezUInt32 GetResolutionY() const { return m_uiResolutionY; }

  /// \brief Prepares the view to rasterize a new scene.
  void BeginScene();

  /// \brief Finishes rasterizing the scene. Visibility queries only work after this.
  void EndScene();

  /// \brief Writes an RGBA8 representation of the depth values to targetBuffer.
  ///
  /// The buffer must be large enough for the chosen resolution.
  void ReadBackFrame(ezArrayPtr<ezColorLinearUB> targetBuffer) const;

  /// \brief Sets the camera from which to extract the rendering position, direction and field-of-view.
  void SetCamera(const ezCamera* pCamera)
  {
    m_pCamera = pCamera;
  }

  /// \brief Adds an object as an occluder to the scene. Once all occluders have been rasterized, visibility queries can be done.
  void AddObject(const ezRasterizerObject* pObject, const ezTransform& transform)
  {
    auto& inst = m_Instances.ExpandAndGetRef();
    inst.m_pObject = pObject;
    inst.m_Transform = transform;
  }

  /// \brief Checks whether a box would be visible, or is fully occluded by the existing scene geometry.
  ///
  /// Note: This only works after EndScene().
  bool IsVisible(const ezSimdBBox& aabb) const;

  /// \brief Wether any occluder was actually added and also rasterized. If not, no need to do any visibility checks.
  bool HasRasterizedAnyOccluders() const
  {
    return m_bAnyOccludersRasterized;
  }

private:
  void SortObjectsFrontToBack();
  void RasterizeObjects(ezUInt32 uiMaxObjects);
  void UpdateViewProjectionMatrix();
  void ApplyModelViewProjectionMatrix(const ezTransform& modelTransform);

  bool m_bAnyOccludersRasterized = false;
  const ezCamera* m_pCamera = nullptr;
  ezUInt32 m_uiResolutionX = 0;
  ezUInt32 m_uiResolutionY = 0;
  float m_fAspectRation = 1.0f;
  ezUniquePtr<Rasterizer> m_pRasterizer;

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
  ezRasterizerView* GetRasterizerView(ezUInt32 width, ezUInt32 height, float fAspectRatio);
  void ReturnRasterizerView(ezRasterizerView* pView);

private:
  struct PoolEntry
  {
    bool m_bInUse = false;
    ezRasterizerView m_RasterizerView;
  };

  ezMutex m_Mutex;
  ezDeque<PoolEntry> m_Entries;
};
