#pragma once

#include <KrautPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Containers/StaticArray.h>

typedef ezTypedResourceHandle<class ezKrautTreeResource> ezKrautTreeResourceHandle;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

struct EZ_KRAUTPLUGIN_DLL ezKrautTreeResourceDescriptor
{
  void Save(ezStreamWriter& stream) const;
  ezResult Load(ezStreamReader& stream);

  ezBoundingBoxSphere m_Bounds;

  struct VertexData
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    ezVec3 m_vTexCoord; // U,V and Q
    ezVec3 m_vNormal;
    ezVec3 m_vTangent;
    ezColorGammaUB m_VariationColor;
  };

  struct TriangleData
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiVertexIndex[3];
  };

  struct SubMeshData
  {
    ezUInt16 m_uiFirstTriangle = 0;
    ezUInt16 m_uiNumTriangles = 0;
    //ezMaterialResourceHandle m_hMaterial;
  };

  struct LodData
  {
    float m_fMinLodDistance;
    float m_fMaxLodDistance;

    ezDynamicArray<VertexData> m_Vertices;
    ezDynamicArray<TriangleData> m_Triangles;
    ezDynamicArray<SubMeshData> m_SubMeshes;
  };


  ezStaticArray<LodData, 5> m_Lods;
};

class EZ_KRAUTPLUGIN_DLL ezKrautTreeResource : public ezResource<ezKrautTreeResource, ezKrautTreeResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeResource, ezResourceBase);

public:
  ezKrautTreeResource();

  /// \brief Returns the bounds of this tree.
  const ezBoundingBoxSphere& GetBounds() const { return m_Descriptor.m_Bounds; }

  ezKrautTreeResourceDescriptor m_Descriptor;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezKrautTreeResourceDescriptor& descriptor) override;

};

