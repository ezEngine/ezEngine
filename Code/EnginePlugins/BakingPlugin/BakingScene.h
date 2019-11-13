#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>

class ezView;
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

  ezResult RenderDebugView(const ezView& view, ezProgress& progress) const;

public:
  struct MeshObject
  {
    ezSimdTransform m_GlobalTransform;
    ezHashedString m_MeshResourceId;
  };

  ezArrayPtr<const MeshObject> GetMeshObjects() const { return m_MeshObjects; }
  const ezBoundingBox& GetBoundingBox() const { return m_BoundingBox; }

private:
  friend class ezMemoryUtils;

  ezBakingScene();
  ~ezBakingScene();

  ezDynamicArray<MeshObject, ezAlignedAllocatorWrapper> m_MeshObjects;
  ezBoundingBox m_BoundingBox;

  ezUInt32 m_uiWorldIndex = ezInvalidIndex;
  ezUniquePtr<ezTracerInterface> m_pTracer;
};
