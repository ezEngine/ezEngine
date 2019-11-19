#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Core/Graphics/AmbientCubeBasis.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>

class ezWorld;
class ezProgress;
class ezTracerInterface;

class EZ_BAKINGPLUGIN_DLL ezBakingScene
{
public:
  static ezBakingScene* GetOrCreate(const ezWorld& world);
  static ezBakingScene* Get(const ezWorld& world);

  ezResult Extract();

  ezResult Bake(const ezStringView& sOutputPath, ezProgress& progress);

  ezResult RenderDebugView(const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels,
    ezProgress& progress) const;

public:
  struct MeshObject
  {
    ezSimdTransform m_GlobalTransform;
    ezHashedString m_MeshResourceId;
  };

  ezArrayPtr<const MeshObject> GetMeshObjects() const { return m_MeshObjects; }
  const ezBoundingBox& GetBoundingBox() const { return m_BoundingBox; }

  ezArrayPtr<const ezVec3> GetProbePositions() const { return m_ProbePositions; }
  ezArrayPtr<const ezAmbientCube<ezUInt8>> GetSkyVisibility() const { return m_SkyVisibility; }

  bool IsBaked() const { return m_bIsBaked; }

private:
  friend class ezMemoryUtils;

  ezBakingScene();
  ~ezBakingScene();

  ezDynamicArray<MeshObject, ezAlignedAllocatorWrapper> m_MeshObjects;
  ezBoundingBox m_BoundingBox;

  // temp
  ezDynamicArray<ezVec3> m_ProbePositions;
  ezDynamicArray<ezAmbientCube<ezUInt8>> m_SkyVisibility;

  ezUInt32 m_uiWorldIndex = ezInvalidIndex;
  ezUniquePtr<ezTracerInterface> m_pTracer;

  bool m_bIsBaked = false;
};
