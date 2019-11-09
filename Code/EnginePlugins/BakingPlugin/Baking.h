#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>

class ezWorld;
class ezProgress;

class EZ_BAKINGPLUGIN_DLL ezBaking
{
public:
  struct Scene
  {
    struct MeshObject
    {
      ezSimdTransform m_GlobalTransform;
      ezHashedString m_MeshResourceId;
    };

    ezDynamicArray<MeshObject, ezAlignedAllocatorWrapper> m_MeshObjects;
    ezBoundingBox m_BoundingBox;
  };

  static ezResult ExtractScene(const ezWorld& world, Scene& out_Scene);

  static ezResult BakeScene(const Scene& scene, const ezStringView& sOutputPath, ezProgress& progress);
};
