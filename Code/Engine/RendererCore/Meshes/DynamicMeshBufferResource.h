#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;

struct ezDynamicMeshBufferResourceDescriptor
{
  ezGALPrimitiveTopology::Enum m_Topology = ezGALPrimitiveTopology::Triangles;
  ezUInt32 m_uiNumPrimitives = 0;
  ezUInt32 m_uiNumVertices = 0;
  ezUInt32 m_uiNumIndices16 = 0;
  ezUInt32 m_uiNumIndices32 = 0;
};

struct EZ_RENDERERCORE_DLL ezDynamicMeshVertex
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_vPosition;
  ezVec2 m_vTexCoord;
  ezVec3 m_vEncodedNormal;
  ezVec4 m_vEncodedTangent;
  //ezColorLinearUB m_Color;

  void EncodeNormal(const ezVec3& normal);
  void EncodeTangent(const ezVec3& tangent, float bitangentSign);
};

class EZ_RENDERERCORE_DLL ezDynamicMeshBufferResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDynamicMeshBufferResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezDynamicMeshBufferResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezDynamicMeshBufferResource, ezDynamicMeshBufferResourceDescriptor);

public:
  ezDynamicMeshBufferResource();
  ~ezDynamicMeshBufferResource();

  EZ_ALWAYS_INLINE const ezDynamicMeshBufferResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  EZ_ALWAYS_INLINE ezGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }
  EZ_ALWAYS_INLINE ezGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  ezArrayPtr<ezDynamicMeshVertex> AccessVertexData() { return m_VertexData; }
  ezArrayPtr<ezUInt16> AccessIndex16Data() { return m_Index16Data; }
  ezArrayPtr<ezUInt32> AccessIndex32Data() { return m_Index32Data; }

  void SetTopology(ezGALPrimitiveTopology::Enum topology) { m_Descriptor.m_Topology = topology; }
  ezGALPrimitiveTopology::Enum GetTopology() const { return m_Descriptor.m_Topology; }

  void SetPrimitiveCount(ezUInt32 numPrimitives) { m_Descriptor.m_uiNumPrimitives = numPrimitives; }
  ezUInt32 GetPrimitiveCount() const { return m_Descriptor.m_uiNumPrimitives; }

  const ezVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  void UpdateGpuBuffer(ezGALCommandEncoder* pGALCommandEncoder, ezUInt32 uiFirstVertex, ezUInt32 uiNumVertices, ezUInt32 uiFirstIndex, ezUInt32 uiNumIndices);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;
  ezDynamicMeshBufferResourceDescriptor m_Descriptor;

  ezVertexDeclarationInfo m_VertexDeclaration;
  ezDynamicArray<ezDynamicMeshVertex, ezAlignedAllocatorWrapper> m_VertexData;
  ezDynamicArray<ezUInt16, ezAlignedAllocatorWrapper> m_Index16Data;
  ezDynamicArray<ezUInt32, ezAlignedAllocatorWrapper> m_Index32Data;
};
