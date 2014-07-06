#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Graphics/OBJLoader.h>

#include <Helper/MayaObj.h>
#include <RendererFoundation/Device/Device.h>

namespace DontUse
{
  MayaObj* MayaObj::LoadFromFile(const char* szPath, ezGALDevice* pDevice)
  {
    ezOBJLoader l;
    if (l.LoadOBJ(szPath).Failed())
      return nullptr;

    ezDynamicArray<MayaObj::Vertex> Vertices;
    ezDynamicArray<ezUInt16> Indices;

    Vertices.Reserve(l.m_Faces.GetCount() * 3);
    Indices.Reserve(l.m_Faces.GetCount() * 3);

    for (ezUInt32 f = 0; f < l.m_Faces.GetCount(); ++f)
    {
      ezUInt32 uiFirstVertex = Vertices.GetCount();

      for (ezUInt32 v = 0; v < l.m_Faces[f].m_Vertices.GetCount(); ++v)
      {
        MayaObj::Vertex vert;
        vert.pos = l.m_Positions[l.m_Faces[f].m_Vertices[v].m_uiPositionID];
        vert.tex0 = l.m_TexCoords[l.m_Faces[f].m_Vertices[v].m_uiTexCoordID].GetAsVec2();
        vert.norm = l.m_Normals[l.m_Faces[f].m_Vertices[v].m_uiNormalID];

        Vertices.PushBack(vert);
      }

      for (ezUInt32 v = 2; v < l.m_Faces[f].m_Vertices.GetCount(); ++v)
      {
        Indices.PushBack(uiFirstVertex);
        Indices.PushBack(uiFirstVertex + v - 1);
        Indices.PushBack(uiFirstVertex + v);
      }
    }

    MayaObj* Obj = EZ_DEFAULT_NEW(MayaObj)(Vertices, Indices, pDevice);
    return Obj;

    return nullptr;
  }

  MayaObj::MayaObj(const ezArrayPtr<MayaObj::Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice)
  {
    ezMeshBufferResourceDescriptor desc;
    desc.m_pDevice = pDevice;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);

    desc.AllocateStreams(pVertices.GetCount(), pIndices.GetCount() / 3);

    // you can do this more efficiently, by just accessing the vertex array directly, this is just more convenient
    for (ezUInt32 v = 0; v < pVertices.GetCount(); ++v)
    {
      desc.SetVertexData<ezVec3>(0, v, pVertices[v].pos);
      desc.SetVertexData<ezVec3>(1, v, pVertices[v].norm);
      desc.SetVertexData<ezVec2>(2, v, pVertices[v].tex0);
    }

    for (ezUInt32 t = 0; t < pIndices.GetCount(); t += 3)
    {
      desc.SetTriangleIndices(t / 3, pIndices[t], pIndices[t + 1], pIndices[t + 2]);
    }

    // there should be a function to generate a unique ID
    // or we should just move this into CreateResource, not sure yet
    m_hMeshBuffer = ezResourceManager::GetResourceHandle<ezMeshBufferResource>("MayaMesh");

    ezResourceManager::CreateResource(m_hMeshBuffer, desc);
  }

}