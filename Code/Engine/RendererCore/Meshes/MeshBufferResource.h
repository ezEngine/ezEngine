#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

typedef ezTypedResourceHandle<class ezMeshBufferResource> ezMeshBufferResourceHandle;
class ezGeometry;

struct EZ_RENDERERCORE_DLL ezVertexStreamInfo : public ezHashableStruct<ezVertexStreamInfo>
{
  EZ_DECLARE_POD_TYPE();

  ezGALVertexAttributeSemantic::Enum m_Semantic;
  ezGALResourceFormat::Enum m_Format;
  ezUInt16 m_uiOffset;      ///< at which byte offset the first element starts
  ezUInt16 m_uiElementSize; ///< the number of bytes for this element type (depends on the format); this is not the stride between elements!
};

struct EZ_RENDERERCORE_DLL ezVertexDeclarationInfo
{
  void ComputeHash();

  ezHybridArray<ezVertexStreamInfo, 8> m_VertexStreams;
  ezUInt32 m_uiHash;
};


struct EZ_RENDERERCORE_DLL ezMeshBufferResourceDescriptor
{
public:
  ezMeshBufferResourceDescriptor();
  ~ezMeshBufferResourceDescriptor();

  void Clear();

  /// \brief Use this function to add vertex streams to the mesh buffer. The return value is the index of the just added stream.
  ezUInt32 AddStream(ezGALVertexAttributeSemantic::Enum Semantic, ezGALResourceFormat::Enum Format);

  /// \brief Add common vertex streams to the mesh buffer. This includes Position, TexCoord0, Normal and Tangent.
  void AddCommonStreams();

  /// \brief After all streams are added, call this to allocate the data for the streams. If uiNumPrimitives is 0, the mesh buffer will not
  /// use indexed rendering.
  void AllocateStreams(ezUInt32 uiNumVertices, ezGALPrimitiveTopology::Enum topology = ezGALPrimitiveTopology::Triangles, ezUInt32 uiNumPrimitives = 0);

  /// \brief Creates streams and fills them with data from the ezGeometry. Only the geometry matching the given topology is used.
  ///  Streams that do not match any of the data inside the ezGeometry directly are skipped.
  void AllocateStreamsFromGeometry(const ezGeometry& geom, ezGALPrimitiveTopology::Enum topology = ezGALPrimitiveTopology::Triangles);

  /// \brief Gives read access to the allocated vertex data
  const ezDynamicArray<ezUInt8>& GetVertexBufferData() const;

  /// \brief Gives read access to the allocated index data
  const ezDynamicArray<ezUInt8>& GetIndexBufferData() const;

  /// \brief Allows write access to the allocated vertex data. This can be used for copying data fast into the array.
  ezDynamicArray<ezUInt8>& GetVertexBufferData();

  /// \brief Allows write access to the allocated index data. This can be used for copying data fast into the array.
  ezDynamicArray<ezUInt8>& GetIndexBufferData();

  /// \brief Slow, but convenient method to write one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  /// data is the piece of data to write to the stream.
  template <typename TYPE>
  void SetVertexData(ezUInt32 uiStream, ezUInt32 uiVertexIndex, const TYPE& data)
  {
    reinterpret_cast<TYPE&>(m_VertexStreamData[m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset]) = data;
  }

  /// \brief Slow, but convenient method to access one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  ezArrayPtr<ezUInt8> GetVertexData(ezUInt32 uiStream, ezUInt32 uiVertexIndex)
  {
    return m_VertexStreamData.GetArrayPtr().GetSubArray(m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset);
  }

  /// \brief Writes the vertex index for the given point into the index buffer.
  void SetPointIndices(ezUInt32 uiPoint, ezUInt32 uiVertex0);

  /// \brief Writes the two vertex indices for the given line into the index buffer.
  void SetLineIndices(ezUInt32 uiLine, ezUInt32 uiVertex0, ezUInt32 uiVertex1);

  /// \brief Writes the three vertex indices for the given triangle into the index buffer.
  void SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2);

  /// \brief Allows to read the stream info of the descriptor, which is filled out by AddStream()
  const ezVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

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

private:
  ezGALPrimitiveTopology::Enum m_Topology;
  ezUInt32 m_uiVertexSize;
  ezUInt32 m_uiVertexCount;
  ezVertexDeclarationInfo m_VertexDeclaration;
  ezDynamicArray<ezUInt8> m_VertexStreamData;
  ezDynamicArray<ezUInt8> m_IndexBufferData;
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

  EZ_ALWAYS_INLINE ezGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }

  EZ_ALWAYS_INLINE ezGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  EZ_ALWAYS_INLINE ezGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  /// \brief Returns the vertex declaration used by this mesh buffer.
  const ezVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Returns the bounds of the mesh
  const ezBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  ezBoundingBoxSphere m_Bounds;
  ezVertexDeclarationInfo m_VertexDeclaration;
  ezUInt32 m_uiPrimitiveCount;
  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;
  ezGALPrimitiveTopology::Enum m_Topology;
};
