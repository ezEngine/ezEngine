#pragma once

#include <BakingPlugin/Declarations.h>
#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/Utils/WorldGeoExtractionUtil.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>

class ezWorld;
class ezProgress;
class ezTracerInterface;

class EZ_BAKINGPLUGIN_DLL ezBakingScene
{
public:
  ezResult Extract();

  ezResult Bake(const ezStringView& sOutputPath, ezProgress& progress);

  ezResult RenderDebugView(const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels,
    ezProgress& progress) const;

public:
  const ezWorldGeoExtractionUtil::MeshObjectList& GetMeshObjects() const { return m_MeshObjects; }
  const ezBoundingBox& GetBoundingBox() const { return m_BoundingBox; }

  bool IsBaked() const { return m_bIsBaked; }

private:
  friend class ezBaking;
  friend class ezMemoryUtils;

  ezBakingScene();
  ~ezBakingScene();

  ezBakingSettings m_Settings;
  ezDynamicArray<ezBakingInternal::Volume, ezAlignedAllocatorWrapper> m_Volumes;
  ezWorldGeoExtractionUtil::MeshObjectList m_MeshObjects;
  ezBoundingBox m_BoundingBox;

  ezUInt32 m_uiWorldIndex = ezInvalidIndex;
  ezUniquePtr<ezTracerInterface> m_pTracer;

  bool m_bIsBaked = false;
};

class EZ_BAKINGPLUGIN_DLL ezBaking : public ezBakingInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezBaking, ezBakingInterface);

public:
  ezBaking();

  void Startup();
  void Shutdown();

  ezBakingScene* GetOrCreateScene(const ezWorld& world);
  ezBakingScene* GetScene(const ezWorld& world);
  const ezBakingScene* GetScene(const ezWorld& world) const;

  // ezBakingInterface
  virtual ezResult RenderDebugView(const ezWorld& world, const ezMat4& InverseViewProjection, ezUInt32 uiWidth, ezUInt32 uiHeight, ezDynamicArray<ezColorGammaUB>& out_Pixels, ezProgress& progress) const override;
};
