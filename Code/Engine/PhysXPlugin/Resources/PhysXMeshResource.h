#pragma once

#include <PhysXPlugin/Basics.h>
#include <Core/ResourceManager/Resource.h>

typedef ezResourceHandle<class ezPhysXMeshResource> ezPhysXMeshResourceHandle;

struct EZ_PHYSXPLUGIN_DLL ezPhysXMeshResourceDescriptor
{
public:

  ezPhysXMeshResourceDescriptor();

  ///// \brief After all streams are added, call this to allocate the data for the streams. If uiNumTriangles is 0, the mesh buffer will not use indexed rendering.
  //void Allocate(ezUInt32 uiNumVertices, ezUInt32 uiNumTriangles = 0);
  //{
  //  reinterpret_cast<TYPE&>(m_VertexStreamData[m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset]) = data;
  //}

  ///// \brief Writes the three vertex indices for the given triangle into the index buffer.
  //void SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2);

  /// \brief Return the number of vertices, with which AllocateStreams() was called.
  ezUInt32 GetVertexCount() const { return m_uiVertexCount; }

  ///// \brief Returns the number of primitives that the array holds.
  //ezUInt32 GetPrimitiveCount() const;

  /// \brief Returns whether 16 or 32 Bit indices are to be used.
  bool Uses32BitIndices() const { return m_uiVertexCount > 0xFFFF; }

private:

  ezUInt32 m_uiVertexCount;
  //ezVertexDeclarationInfo m_VertexDeclaration;
  //ezDynamicArray<ezUInt8> m_VertexStreamData;
  //ezDynamicArray<ezUInt8> m_IndexBufferData;
};

class EZ_PHYSXPLUGIN_DLL ezPhysXMeshResource : public ezResource<ezPhysXMeshResource, ezPhysXMeshResourceDescriptor>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXMeshResource, ezResourceBase);

public:
  ezPhysXMeshResource();
  ~ezPhysXMeshResource();


private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual ezResourceLoadDesc CreateResource(const ezPhysXMeshResourceDescriptor& descriptor) override;

private:
  PxTriangleMesh* m_pPxMesh;
};

