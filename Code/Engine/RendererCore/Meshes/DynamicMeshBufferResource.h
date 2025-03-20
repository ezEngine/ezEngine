#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/BufferPool.h>

using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;

struct ezDynamicMeshBufferResourceDescriptor
{
  ezGALPrimitiveTopology::Enum m_Topology = ezGALPrimitiveTopology::Triangles;
  ezGALIndexType::Enum m_IndexType = ezGALIndexType::UInt;
  ezUInt32 m_uiMaxPrimitives = 0;
  ezUInt32 m_uiMaxVertices = 0;
  bool m_bColorStream = false;
};

struct EZ_RENDERERCORE_DLL ezDynamicMeshVertex
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_vPosition;
  ezVec2 m_vTexCoord;
  ezVec3 m_vEncodedNormal;
  ezVec4 m_vEncodedTangent;
  // ezColorLinearUB m_Color;

  EZ_ALWAYS_INLINE void EncodeNormal(const ezVec3& vNormal)
  {
    // store in [0; 1] range
    m_vEncodedNormal = vNormal * 0.5f + ezVec3(0.5f);

    // this is the same
    // ezMeshBufferUtils::EncodeNormal(normal, ezByteArrayPtr(reinterpret_cast<ezUInt8*>(&m_vEncodedNormal), sizeof(ezVec3)), ezMeshNormalPrecision::_32Bit).IgnoreResult();
  }

  EZ_ALWAYS_INLINE void EncodeTangent(const ezVec3& vTangent, float fBitangentSign)
  {
    // store in [0; 1] range
    m_vEncodedTangent.x = vTangent.x * 0.5f + 0.5f;
    m_vEncodedTangent.y = vTangent.y * 0.5f + 0.5f;
    m_vEncodedTangent.z = vTangent.z * 0.5f + 0.5f;
    m_vEncodedTangent.w = fBitangentSign < 0.0f ? 0.0f : 1.0f;

    // this is the same
    // ezMeshBufferUtils::EncodeTangent(tangent, bitangentSign, ezByteArrayPtr(reinterpret_cast<ezUInt8*>(&m_vEncodedTangent), sizeof(ezVec4)), ezMeshNormalPrecision::_32Bit).IgnoreResult();
  }
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
  EZ_ALWAYS_INLINE ezGALBufferHandle GetVertexBuffer() const { return m_VertexBuffer.IsInitialized() ? m_VertexBuffer.GetCurrentBuffer() : ezGALBufferHandle(); }
  EZ_ALWAYS_INLINE ezGALBufferHandle GetIndexBuffer() const { return m_IndexBuffer.IsInitialized() ? m_IndexBuffer.GetCurrentBuffer() : ezGALBufferHandle(); }
  EZ_ALWAYS_INLINE ezGALBufferHandle GetColorBuffer() const { return m_ColorBuffer.IsInitialized() ? m_ColorBuffer.GetCurrentBuffer() : ezGALBufferHandle(); }

  /// \brief Grants write access to the vertex data, and flags the data as 'dirty'.
  ezArrayPtr<ezDynamicMeshVertex> AccessVertexData()
  {
    m_bAccessedVB = true;
    return m_VertexData;
  }

  /// \brief Grants write access to the 16 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 16 bit indices.
  ezArrayPtr<ezUInt16> AccessIndex16Data()
  {
    m_bAccessedIB = true;
    return m_Index16Data;
  }

  /// \brief Grants write access to the 32 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 32 bit indices.
  ezArrayPtr<ezUInt32> AccessIndex32Data()
  {
    m_bAccessedIB = true;
    return m_Index32Data;
  }

  /// \brief Grants write access to the color data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if creation of the color buffer was enabled.
  ezArrayPtr<ezColorLinearUB> AccessColorData()
  {
    m_bAccessedCB = true;
    return m_ColorData;
  }

  /// \brief Returns the vertex attributes that describes the data layout of the vertex buffers.
  EZ_ALWAYS_INLINE ezArrayPtr<const ezGALVertexAttribute> GetVertexAttributes() const { return m_VertexAttributes; }

  /// \brief Uploads the current vertex and index data to the GPU.
  ///
  /// If all values are set to default, the entire data is uploaded.
  /// If \a uiNumVertices or \a uiNumIndices is set to the max value, all vertices or indices (after their start offset) are uploaded.
  ///
  /// In all other cases, the number of elements to upload must be within valid bounds.
  ///
  /// This function can be used to only upload a subset of the modified data.
  ///
  /// Note that this function doesn't do anything, if the vertex or index data wasn't recently accessed through AccessVertexData(), AccessIndex16Data() or AccessIndex32Data(). So if you want to upload multiple pieces of the data to the GPU, you have to call these functions in between to flag the uploaded data as out-of-date.
  void UpdateGpuBuffer(ezGALCommandEncoder* pGALCommandEncoder, ezUInt32 uiFirstVertex = 0, ezUInt32 uiNumVertices = ezMath::MaxValue<ezUInt32>(), ezUInt32 uiFirstIndex = 0, ezUInt32 uiNumIndices = ezMath::MaxValue<ezUInt32>(), bool bCreateNewBuffer = true);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bAccessedVB = false;
  bool m_bAccessedIB = false;
  bool m_bAccessedCB = false;

  ezGALBufferPool m_VertexBuffer;
  ezGALBufferPool m_IndexBuffer;
  ezGALBufferPool m_ColorBuffer;
  ezDynamicMeshBufferResourceDescriptor m_Descriptor;

  ezSmallArray<ezGALVertexAttribute, 8> m_VertexAttributes;
  ezDynamicArray<ezDynamicMeshVertex, ezAlignedAllocatorWrapper> m_VertexData;
  ezDynamicArray<ezUInt16, ezAlignedAllocatorWrapper> m_Index16Data;
  ezDynamicArray<ezUInt32, ezAlignedAllocatorWrapper> m_Index32Data;
  ezDynamicArray<ezColorLinearUB, ezAlignedAllocatorWrapper> m_ColorData;
};
