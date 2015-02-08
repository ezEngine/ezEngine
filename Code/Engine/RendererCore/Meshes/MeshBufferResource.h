#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

typedef ezResourceHandle<class ezMeshBufferResource> ezMeshBufferResourceHandle;

struct EZ_RENDERERCORE_DLL ezVertexStreamInfo : public ezHashableStruct<ezVertexStreamInfo>
{
  EZ_DECLARE_POD_TYPE();

  ezGALVertexAttributeSemantic::Enum m_Semantic;
  ezGALResourceFormat::Enum m_Format;
  ezUInt16 m_uiOffset;
  ezUInt16 m_uiElementSize;
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

  /// \brief Use this function to add vertex streams to the mesh buffer. The return value is the index of the just added stream.
  ezUInt32 AddStream(ezGALVertexAttributeSemantic::Enum Semantic, ezGALResourceFormat::Enum Format);

  /// \brief After all streams are added, call this to allocate the data for the streams. If uiNumTriangles is 0, the mesh buffer will not use indexed rendering.
  void AllocateStreams(ezUInt32 uiNumVertices, ezUInt32 uiNumTriangles = 0);

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
  template<typename TYPE>
  void SetVertexData(ezUInt32 uiStream, ezUInt32 uiVertexIndex, const TYPE& data)
  {
    reinterpret_cast<TYPE&>(m_VertexStreamData[m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset]) = data;
  }

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

private:

  ezUInt32 m_uiVertexSize;
  ezUInt32 m_uiVertexCount;
  ezVertexDeclarationInfo m_VertexDeclaration;
  ezDynamicArray<ezUInt8> m_VertexStreamData;
  ezDynamicArray<ezUInt8> m_IndexBufferData;
};

class EZ_RENDERERCORE_DLL ezMeshBufferResource : public ezResource<ezMeshBufferResource, ezMeshBufferResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshBufferResource);

public:
  ezMeshBufferResource();
  ~ezMeshBufferResource();

  EZ_FORCE_INLINE ezUInt32 GetPrimitiveCount() const
  {
    return m_uiPrimitiveCount;
  }

  EZ_FORCE_INLINE ezGALBufferHandle GetVertexBuffer() const
  {
    return m_hVertexBuffer;
  }

  EZ_FORCE_INLINE ezGALBufferHandle GetIndexBuffer() const
  {
    return m_hIndexBuffer;
  }

  /// \brief Returns the vertex declaration used by this mesh buffer.
  const ezVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReaderBase* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezMeshBufferResourceDescriptor& descriptor) override;

  ezVertexDeclarationInfo m_VertexDeclaration;
  ezUInt32 m_uiPrimitiveCount;
  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;
};

