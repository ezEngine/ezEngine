#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/Color16f.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;

struct ezDynamicMeshBufferResourceDescriptor
{
  ezEnum<ezGALPrimitiveTopology> m_Topology = ezGALPrimitiveTopology::Triangles;
  ezEnum<ezGALIndexType> m_IndexType = ezGALIndexType::UInt;
  bool m_bColorStream = false;
  ezUInt32 m_uiMaxPrimitives = 0;
  ezUInt32 m_uiMaxVertices = 0;
};

// Contains the normal, tangent and texcoord0 of a dynamic mesh vertex.
struct EZ_RENDERERCORE_DLL ezDynamicMeshVertexNTT
{
  EZ_DECLARE_POD_TYPE();

  ezVec4U16 m_vEncodedNormal;
  ezVec4U16 m_vEncodedTangent;
  ezVec2 m_vTexCoord;

  EZ_ALWAYS_INLINE void EncodeNormal(const ezVec3& vNormal)
  {
    // store in [0; 1] range
    m_vEncodedNormal.x = ezMath::ColorFloatToShort(vNormal.x * 0.5f + 0.5f);
    m_vEncodedNormal.y = ezMath::ColorFloatToShort(vNormal.y * 0.5f + 0.5f);
    m_vEncodedNormal.z = ezMath::ColorFloatToShort(vNormal.z * 0.5f + 0.5f);
    m_vEncodedNormal.w = 0.0f;

    // this is the same but slower
    // ezMeshBufferUtils::EncodeNormal(vNormal, ezByteArrayPtr(reinterpret_cast<ezUInt8*>(&m_vEncodedNormal), sizeof(ezVec4U16)), ezGALResourceFormat::RGBAUShortNormalized).IgnoreResult();
  }

  EZ_ALWAYS_INLINE void EncodeTangent(const ezVec3& vTangent, float fBitangentSign)
  {
    // store in [0; 1] range
    m_vEncodedTangent.x = ezMath::ColorFloatToShort(vTangent.x * 0.5f + 0.5f);
    m_vEncodedTangent.y = ezMath::ColorFloatToShort(vTangent.y * 0.5f + 0.5f);
    m_vEncodedTangent.z = ezMath::ColorFloatToShort(vTangent.z * 0.5f + 0.5f);
    m_vEncodedTangent.w = ezMath::ColorFloatToShort(fBitangentSign < 0.0f ? 0.0f : 1.0f);

    // this is the same but slower
    ezMeshBufferUtils::EncodeTangent(vTangent, fBitangentSign, ezByteArrayPtr(reinterpret_cast<ezUInt8*>(&m_vEncodedTangent), sizeof(ezVec4U16)), ezGALResourceFormat::RGBAUShortNormalized).IgnoreResult();
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
  EZ_ALWAYS_INLINE ezArrayPtr<const ezGALBufferHandle> GetVertexBuffers() const { return ezMakeArrayPtr(m_hVertexBuffers); }
  EZ_ALWAYS_INLINE ezGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  /// \brief Grants write access to the position data, and flags the data as 'dirty'.
  ezArrayPtr<ezVec3> AccessPositionData(ezUInt32 uiFirstVertex = 0, ezUInt32 uiNumVertices = ezInvalidIndex)
  {
    m_ModifiedPositionDataRange.SetToIncludeRange(uiFirstVertex, uiFirstVertex + ezMath::Min(uiNumVertices, m_PositionData.GetCount() - uiFirstVertex) - 1);
    MarkAsDirty();

    return m_PositionData;
  }

  /// \brief Grants write access to the normal, tangent and texcoord0 data, and flags the data as 'dirty'.
  ezArrayPtr<ezDynamicMeshVertexNTT> AccessNormalTangentTexCoord0Data(ezUInt32 uiFirstVertex = 0, ezUInt32 uiNumVertices = ezInvalidIndex)
  {
    m_ModifiedNTTDataRange.SetToIncludeRange(uiFirstVertex, uiFirstVertex + ezMath::Min(uiNumVertices, m_NTTData.GetCount() - uiFirstVertex) - 1);
    MarkAsDirty();

    return m_NTTData;
  }

  /// \brief Grants write access to the color data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if creation of the color buffer was enabled.
  ezArrayPtr<ezColorLinear16f> AccessColorData(ezUInt32 uiFirstVertex = 0, ezUInt32 uiNumVertices = ezInvalidIndex)
  {
    m_ModifiedColorDataRange.SetToIncludeRange(uiFirstVertex, uiFirstVertex + ezMath::Min(uiNumVertices, m_ColorData.GetCount() - uiFirstVertex) - 1);
    MarkAsDirty();

    return m_ColorData;
  }

  /// \brief Grants write access to the 16 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 16 bit indices.
  ezArrayPtr<ezUInt16> AccessIndex16Data(ezUInt32 uiFirstIndex = 0, ezUInt32 uiNumIndices = ezInvalidIndex)
  {
    constexpr ezUInt32 uiIndexByteSize = sizeof(ezUInt16);
    const ezUInt32 uiMinByte = uiFirstIndex * uiIndexByteSize;
    const ezUInt32 uiMaxByte = uiMinByte + ezMath::Min(uiNumIndices * uiIndexByteSize, m_IndexData.GetCount() - uiMinByte) - 1;
    m_ModifiedIndexDataRange.SetToIncludeRange(uiMinByte, uiMaxByte);
    MarkAsDirty();

    return ezMakeArrayPtr(reinterpret_cast<ezUInt16*>(m_IndexData.GetData()), m_IndexData.GetCount() / uiIndexByteSize);
  }

  /// \brief Grants write access to the 32 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 32 bit indices.
  ezArrayPtr<ezUInt32> AccessIndex32Data(ezUInt32 uiFirstIndex = 0, ezUInt32 uiNumIndices = ezInvalidIndex)
  {
    constexpr ezUInt32 uiIndexByteSize = sizeof(ezUInt32);
    const ezUInt32 uiMinByte = uiFirstIndex * uiIndexByteSize;
    const ezUInt32 uiMaxByte = uiMinByte + ezMath::Min(uiNumIndices * uiIndexByteSize, m_IndexData.GetCount() - uiMinByte) - 1;
    m_ModifiedIndexDataRange.SetToIncludeRange(uiMinByte, uiMaxByte);
    MarkAsDirty();

    return ezMakeArrayPtr(reinterpret_cast<ezUInt32*>(m_IndexData.GetData()), m_IndexData.GetCount() / uiIndexByteSize);
  }

  /// \brief Returns the vertex attributes that describes the data layout of the vertex buffers.
  EZ_ALWAYS_INLINE ezArrayPtr<const ezGALVertexAttribute> GetVertexAttributes() const { return m_VertexAttributes; }

  /// \brief Helper function to create a grid aligned to the XY plane with the given size and number of vertices. The mesh buffer must already have the right number of vertices.
  static void CreateGridXY(ezDynamicMeshBufferResource* pDynamicMeshBuffer, const ezVec2& vSize, const ezVec2U32& vNumVertices, const ezVec2& vTextureScale = ezVec2(1));

  /// \brief Helper function to calculate smooth normals and tangents for a grid mesh created with the function above after its positions have been updated.
  static void CalculateGridNormalAndTangents(ezDynamicMeshBufferResource* pDynamicMeshBuffer, const ezVec2U32& vNumVertices);

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  friend struct ezDynamicMeshBufferManager;
  void MarkAsDirty();
  void UploadChangesForNextFrame();

  ezDynamicMeshBufferResourceDescriptor m_Descriptor;

  ezSmallArray<ezGALVertexAttribute, 8> m_VertexAttributes;

  ezGALBufferHandle m_hVertexBuffers[ezMeshVertexStreamType::Color0 + 1];
  ezGALBufferHandle m_hIndexBuffer;

  ezDynamicArray<ezVec3, ezAlignedAllocatorWrapper> m_PositionData;
  ezDynamicArray<ezDynamicMeshVertexNTT, ezAlignedAllocatorWrapper> m_NTTData;
  ezDynamicArray<ezColorLinear16f, ezAlignedAllocatorWrapper> m_ColorData;

  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_IndexData;

  ezGAL::ModifiedRange m_ModifiedPositionDataRange;
  ezGAL::ModifiedRange m_ModifiedNTTDataRange;
  ezGAL::ModifiedRange m_ModifiedColorDataRange;
  ezGAL::ModifiedRange m_ModifiedIndexDataRange; // In bytes
};
