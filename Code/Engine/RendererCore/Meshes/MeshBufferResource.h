#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

using ezMeshBufferResourceHandle = ezTypedResourceHandle<class ezMeshBufferResource>;
class ezGeometry;

struct ezMeshVertexStreamType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Position,
    NormalTangentAndTexCoord0,
    TexCoord1,
    Color0,
    Color1,
    SkinningData,

    Count,

    Default = Position
  };

  static const char* GetName(Enum type);
};

struct EZ_RENDERERCORE_DLL ezMeshVertexStreamConfig
{
  ezUInt16 m_uiTypesMask = 0;
  bool m_bUseHighPrecision = false;

  EZ_ALWAYS_INLINE void AddStream(ezMeshVertexStreamType::Enum type) { m_uiTypesMask |= EZ_BIT(type); }
  EZ_ALWAYS_INLINE bool HasStream(ezMeshVertexStreamType::Enum type) const { return (m_uiTypesMask & EZ_BIT(type)) != 0; }

  EZ_ALWAYS_INLINE bool HasPosition() const { return HasStream(ezMeshVertexStreamType::Position); }
  EZ_ALWAYS_INLINE bool HasNormalTangentAndTexCoord0() const { return HasStream(ezMeshVertexStreamType::NormalTangentAndTexCoord0); }
  EZ_ALWAYS_INLINE bool HasNormal() const { return HasStream(ezMeshVertexStreamType::NormalTangentAndTexCoord0); }
  EZ_ALWAYS_INLINE bool HasTangent() const { return HasStream(ezMeshVertexStreamType::NormalTangentAndTexCoord0); }
  EZ_ALWAYS_INLINE bool HasTexCoord0() const { return HasStream(ezMeshVertexStreamType::NormalTangentAndTexCoord0); }
  EZ_ALWAYS_INLINE bool HasTexCoord1() const { return HasStream(ezMeshVertexStreamType::TexCoord1); }
  EZ_ALWAYS_INLINE bool HasColor0() const { return HasStream(ezMeshVertexStreamType::Color0); }
  EZ_ALWAYS_INLINE bool HasColor1() const { return HasStream(ezMeshVertexStreamType::Color1); }
  EZ_ALWAYS_INLINE bool HasSkinningData() const { return HasStream(ezMeshVertexStreamType::SkinningData); }
  EZ_ALWAYS_INLINE bool HasBoneIndices() const { return HasStream(ezMeshVertexStreamType::SkinningData); }
  EZ_ALWAYS_INLINE bool HasBoneWeights() const { return HasStream(ezMeshVertexStreamType::SkinningData); }

  EZ_ALWAYS_INLINE ezUInt32 GetHighestStreamIndex() const { return ezMath::FirstBitHigh(ezUInt32(m_uiTypesMask)); }

  template <ezUInt16 ArraySize>
  void FillVertexAttributes(ezSmallArray<ezGALVertexAttribute, ArraySize>& out_vertexAttributes)
  {
    for (auto vertexAttribute : GetAllVertexAttributes())
    {
      if ((m_uiTypesMask & EZ_BIT(vertexAttribute.m_uiVertexBufferSlot)) != 0)
      {
        out_vertexAttributes.PushBack(vertexAttribute);
      }
    }
  }

  ezGALResourceFormat::Enum GetPositionFormat() const;
  ezGALResourceFormat::Enum GetNormalFormat() const;
  ezGALResourceFormat::Enum GetTangentFormat() const;
  ezGALResourceFormat::Enum GetTexCoordFormat() const;
  ezGALResourceFormat::Enum GetColorFormat() const;
  ezGALResourceFormat::Enum GetBoneIndicesFormat() const;
  ezGALResourceFormat::Enum GetBoneWeightsFormat() const;

  ezUInt32 GetNormalDataOffset() const;
  ezUInt32 GetTangentDataOffset() const;
  ezUInt32 GetTexCoord0DataOffset() const;
  ezUInt32 GetBoneIndicesDataOffset() const;
  ezUInt32 GetBoneWeightsDataOffset() const;

  ezUInt32 GetStreamElementSize(ezMeshVertexStreamType::Enum type) const;

  EZ_ALWAYS_INLINE ezUInt32 GetPositionElementSize() const { return GetStreamElementSize(ezMeshVertexStreamType::Position); }
  EZ_ALWAYS_INLINE ezUInt32 GetNormalTangentAndTexCoord0ElementSize() const { return GetStreamElementSize(ezMeshVertexStreamType::NormalTangentAndTexCoord0); }
  EZ_ALWAYS_INLINE ezUInt32 GetTexCoord1ElementSize() const { return GetStreamElementSize(ezMeshVertexStreamType::TexCoord1); }
  EZ_ALWAYS_INLINE ezUInt32 GetColor0ElementSize() const { return GetStreamElementSize(ezMeshVertexStreamType::Color0); }
  EZ_ALWAYS_INLINE ezUInt32 GetColor1ElementSize() const { return GetStreamElementSize(ezMeshVertexStreamType::Color1); }
  EZ_ALWAYS_INLINE ezUInt32 GetSkinningDataElementSize() const { return GetStreamElementSize(ezMeshVertexStreamType::SkinningData); }

private:
  ezArrayPtr<ezGALVertexAttribute> GetAllVertexAttributes();
};

struct EZ_RENDERERCORE_DLL ezMeshBufferResourceDescriptor
{
public:
  ezMeshBufferResourceDescriptor();
  ~ezMeshBufferResourceDescriptor();

  void Clear();

  /// \brief Use this function to add vertex streams to the mesh buffer.
  void AddStream(ezMeshVertexStreamType::Enum type, bool bUseHighPrecision = false);

  /// \brief Adds common vertex streams to the mesh buffer.
  ///
  /// The streams are added
  /// * Position
  /// * NormalTangentAndTexCoord0
  void AddCommonStreams(bool bUseHighPrecision = false);

  /// \brief After all streams are added, call this to allocate the data for the streams. If uiNumPrimitives is 0, the mesh buffer will not
  /// use indexed rendering.
  void AllocateStreams(ezUInt32 uiNumVertices, ezGALPrimitiveTopology::Enum topology = ezGALPrimitiveTopology::Triangles, ezUInt32 uiNumPrimitives = 0, bool bZeroFill = false);

  /// \brief Creates streams and fills them with data from the ezGeometry. Only the geometry matching the given topology is used.
  ///  Streams that do not match any of the data inside the ezGeometry directly are skipped.
  void AllocateStreamsFromGeometry(const ezGeometry& geom, ezGALPrimitiveTopology::Enum topology = ezGALPrimitiveTopology::Triangles);


  /// \brief Returns the number of vertex buffer used. Note that some vertex buffers in between might be empty if unused.
  ezUInt32 GetNumVertexBuffers() const;

  /// \brief Gives read access to the allocated vertex data
  ezArrayPtr<const ezUInt8> GetVertexBufferData(ezMeshVertexStreamType::Enum type) const;

  /// \brief Gives read access to the allocated index data
  ezArrayPtr<const ezUInt8> GetIndexBufferData() const;

  /// \brief Allows write access to the allocated vertex data. This can be used for copying data fast into the array.
  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>& GetVertexBufferData(ezMeshVertexStreamType::Enum type);

  /// \brief Allows write access to the allocated index data. This can be used for copying data fast into the array.
  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>& GetIndexBufferData();


  /// \brief Gives access to the position data
  ezArrayPtr<const ezVec3> GetPositionData() const;
  ezArrayPtr<ezVec3> GetPositionData();

  /// \brief Gives access to the normal data. Use ezMeshBufferUtils::EncodeNormal/ezMeshBufferUtils::DecodeNormal to pack/unpack the normal.
  ezArrayPtr<const ezUInt8> GetNormalData(ezUInt32* out_pStride = nullptr) const;
  ezArrayPtr<ezUInt8> GetNormalData(ezUInt32* out_pStride = nullptr);

  /// \brief Gives access to the tangent data. Use ezMeshBufferUtils::EncodeTangent/ezMeshBufferUtils::DecodeTangent to pack/unpack the tangent.
  ezArrayPtr<const ezUInt8> GetTangentData(ezUInt32* out_pStride = nullptr) const;
  ezArrayPtr<ezUInt8> GetTangentData(ezUInt32* out_pStride = nullptr);

  /// \brief Gives access to the tex coord 0 data. Use ezMeshBufferUtils::EncodeTexCoord/ezMeshBufferUtils::DecodeTexCoord to pack/unpack the tex coord.
  ezArrayPtr<const ezUInt8> GetTexCoord0Data(ezUInt32* out_pStride = nullptr) const;
  ezArrayPtr<ezUInt8> GetTexCoord0Data(ezUInt32* out_pStride = nullptr);

  /// \brief Gives access to the tex coord 1 data. Use ezMeshBufferUtils::EncodeTexCoord/ezMeshBufferUtils::DecodeTexCoord to pack/unpack the tex coord.
  ezArrayPtr<const ezUInt8> GetTexCoord1Data(ezUInt32* out_pStride = nullptr) const;
  ezArrayPtr<ezUInt8> GetTexCoord1Data(ezUInt32* out_pStride = nullptr);

  /// \brief Gives access to the color 0 data. Use ezMeshBufferUtils::EncodeFromVec4/ezMeshBufferUtils::DecodeToVec4 to pack/unpack the color.
  ezArrayPtr<const ezUInt8> GetColor0Data(ezUInt32* out_pStride = nullptr) const;
  ezArrayPtr<ezUInt8> GetColor0Data(ezUInt32* out_pStride = nullptr);

  /// \brief Gives access to the color 1 data. Use ezMeshBufferUtils::EncodeFromVec4/ezMeshBufferUtils::DecodeToVec4 to pack/unpack the color.
  ezArrayPtr<const ezUInt8> GetColor1Data(ezUInt32* out_pStride = nullptr) const;
  ezArrayPtr<ezUInt8> GetColor1Data(ezUInt32* out_pStride = nullptr);


  /// \brief Slow, but convenient access to the position of a specific vertex.
  const ezVec3& GetPosition(ezUInt32 uiVertexIndex) const;
  void SetPosition(ezUInt32 uiVertexIndex, const ezVec3& vPos);

  /// \brief Slow, but convenient access to the normal of a specific vertex.
  ezVec3 GetNormal(ezUInt32 uiVertexIndex) const;
  void SetNormal(ezUInt32 uiVertexIndex, const ezVec3& vNormal);

  /// \brief Slow, but convenient access to the tangent of a specific vertex. The w component contains the bi-tangent sign.
  ezVec4 GetTangent(ezUInt32 uiVertexIndex) const;
  void SetTangent(ezUInt32 uiVertexIndex, const ezVec4& vTangent);

  /// \brief Slow, but convenient access to the tex coord 0 of a specific vertex.
  ezVec2 GetTexCoord0(ezUInt32 uiVertexIndex) const;
  void SetTexCoord0(ezUInt32 uiVertexIndex, const ezVec2& vTexCoord);

  /// \brief Slow, but convenient access to the tex coord 1 of a specific vertex.
  ezVec2 GetTexCoord1(ezUInt32 uiVertexIndex) const;
  void SetTexCoord1(ezUInt32 uiVertexIndex, const ezVec2& vTexCoord);

  /// \brief Slow, but convenient access to the color 0 of a specific vertex.
  ezColor GetColor0(ezUInt32 uiVertexIndex) const;
  void SetColor0(ezUInt32 uiVertexIndex, const ezColorLinearUB& color);
  void SetColor0(ezUInt32 uiVertexIndex, const ezColor& color, ezMeshVertexColorConversion::Enum conversion = ezMeshVertexColorConversion::Default);

  /// \brief Slow, but convenient access to the color 1 of a specific vertex.
  ezColor GetColor1(ezUInt32 uiVertexIndex) const;
  void SetColor1(ezUInt32 uiVertexIndex, const ezColorLinearUB& color);
  void SetColor1(ezUInt32 uiVertexIndex, const ezColor& color, ezMeshVertexColorConversion::Enum conversion = ezMeshVertexColorConversion::Default);

  /// \brief Slow, but convenient access to the bone indices of a specific vertex.
  const ezVec4U16& GetBoneIndices(ezUInt32 uiVertexIndex) const;
  void SetBoneIndices(ezUInt32 uiVertexIndex, const ezVec4U16& vIndices);

  /// \brief Slow, but convenient access to the bone weights of a specific vertex.
  ezVec4 GetBoneWeights(ezUInt32 uiVertexIndex) const;
  void SetBoneWeights(ezUInt32 uiVertexIndex, const ezVec4& vWeights);


  /// \brief Writes the vertex index for the given point into the index buffer.
  void SetPointIndices(ezUInt32 uiPoint, ezUInt32 uiVertex0);

  /// \brief Writes the two vertex indices for the given line into the index buffer.
  void SetLineIndices(ezUInt32 uiLine, ezUInt32 uiVertex0, ezUInt32 uiVertex1);

  /// \brief Writes the three vertex indices for the given triangle into the index buffer.
  void SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2);


  /// \brief Allows to read the stream info of the descriptor, which is filled out by AddStream()
  const ezMeshVertexStreamConfig& GetVertexStreamConfig() const { return m_VertexStreamConfig; }

  /// \brief Returns the byte size of all the data for one vertex.
  ezUInt32 GetVertexDataSize() const { return m_uiVertexSize; }

  /// \brief Return the number of vertices, with which AllocateStreams() was called.
  ezUInt32 GetVertexCount() const { return m_uiVertexCount; }

  /// \brief Returns the number of primitives that the array holds.
  ezUInt32 GetPrimitiveCount() const;

  /// \brief Returns whether 16 or 32 Bit indices are to be used.
  bool Uses32BitIndices() const { return m_uiVertexCount > 0xFFFF; }

  /// \brief Returns whether an index buffer is available.
  bool HasIndexBuffer() const { return !m_IndexBufferData.IsEmpty(); }

  /// \brief Calculates the bounds using the data from the position stream
  ezBoundingBoxSphere ComputeBounds() const;

  /// \brief Returns the primitive topology
  ezGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  ezResult RecomputeNormals();

private:
  EZ_ALWAYS_INLINE ezByteArrayPtr GetVertexData(ezMeshVertexStreamType::Enum type, ezUInt32 uiVertexIndex, ezUInt32 uiElementSize, ezUInt32 uiOffset = 0)
  {
    return m_VertexStreamsData[type].GetArrayPtr().GetSubArray(uiVertexIndex * uiElementSize + uiOffset);
  }

  EZ_ALWAYS_INLINE ezConstByteArrayPtr GetVertexData(ezMeshVertexStreamType::Enum type, ezUInt32 uiVertexIndex, ezUInt32 uiElementSize, ezUInt32 uiOffset = 0) const
  {
    return m_VertexStreamsData[type].GetArrayPtr().GetSubArray(uiVertexIndex * uiElementSize + uiOffset);
  }

  ezGALPrimitiveTopology::Enum m_Topology = ezGALPrimitiveTopology::Triangles;
  ezUInt32 m_uiVertexSize = 0;
  ezUInt32 m_uiVertexCount = 0;
  ezMeshVertexStreamConfig m_VertexStreamConfig;
  ezHybridArray<ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>, ezMeshVertexStreamType::Count> m_VertexStreamsData;
  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_IndexBufferData;
};

class EZ_RENDERERCORE_DLL ezMeshBufferResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshBufferResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezMeshBufferResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezMeshBufferResource, ezMeshBufferResourceDescriptor);

public:
  ezMeshBufferResource();
  ~ezMeshBufferResource();

  EZ_ALWAYS_INLINE ezUInt32 GetPrimitiveCount() const { return m_uiPrimitiveCount; }

  EZ_ALWAYS_INLINE ezArrayPtr<const ezGALBufferHandle> GetVertexBuffers() const { return ezMakeArrayPtr(m_hVertexBuffers, m_VertexStreamConfig.GetHighestStreamIndex() + 1); }

  EZ_ALWAYS_INLINE ezGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  EZ_ALWAYS_INLINE ezGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  /// \brief Returns the stream config used by this mesh buffer.
  EZ_ALWAYS_INLINE const ezMeshVertexStreamConfig& GetVertexStreamConfig() const { return m_VertexStreamConfig; }

  /// \brief Returns the vertex attributes that describes the data layout of the vertex buffers.
  EZ_ALWAYS_INLINE ezArrayPtr<const ezGALVertexAttribute> GetVertexAttributes() const { return m_VertexAttributes; }

  /// \brief Returns the bounds of the mesh
  EZ_ALWAYS_INLINE const ezBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezBoundingBoxSphere m_Bounds;
  ezMeshVertexStreamConfig m_VertexStreamConfig;
  ezSmallArray<ezGALVertexAttribute, 8> m_VertexAttributes;
  ezUInt32 m_uiPrimitiveCount = 0;
  ezGALBufferHandle m_hVertexBuffers[ezMeshVertexStreamType::Count];
  ezGALBufferHandle m_hIndexBuffer;
  ezGALPrimitiveTopology::Enum m_Topology = ezGALPrimitiveTopology::Enum::Default;
};
