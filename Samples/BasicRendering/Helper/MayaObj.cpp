#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/HashTable.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <CoreUtils/Geometry/OBJLoader.h>

#include <Helper/MayaObj.h>
#include <RendererFoundation/Device/Device.h>
#include <CoreUtils/Geometry/GeomUtils.h>

namespace DontUse
{
  MayaObj* MayaObj::LoadFromFile(const char* szPath, ezGALDevice* pDevice, int i)
  {
    ezDynamicArray<MayaObj::Vertex> Vertices;
    ezDynamicArray<ezUInt16> Indices;

    if (true)
    {
      ezMat4 m;

      ezGeometry geom;
      //geom.AddRectXY(1.0f, 2.0f, ezColorLinearUB(255, 255, 0));

      m.SetRotationMatrixY(ezAngle::Degree(90));
      //geom.AddRectXY(2.0f, 1.0f, ezColorLinearUB(255, 255, 0), m);

      ezColor col(0, 1, 0);

      ezMat4 mTrans;
      mTrans.SetIdentity();
      mTrans.SetRotationMatrixZ(ezAngle::Degree(90));
      //mTrans.SetTranslationMatrix(ezVec3(1, 0, 0));
      //mTrans.SetScalingFactors(ezVec3(0.5f, 1, 0.3f));
      //geom.AddGeodesicSphere(0.5f, i, ezColorLinearUB(0, 255, 0), mTrans);

      //geom.AddBox(ezVec3(1, 2, 3), ezColorLinearUB(0, 255, 0), mTrans);
      //geom.AddPyramid(ezVec3(1.0f, 1.5f, 2.0f), col);
      //geom.AddCylinder(1.0f, 1.0f, 1.1f, true, true, 3 * (i + 1), col, mTrans, 0, ezAngle::Degree(270));
      //geom.AddCone(-1.0f, 2.0f, true, 3 * (i + 1), col, mTrans);
      geom.AddHalfSphere(1.0f, i + 3, i + 1 , true, col, mTrans, 0);
      //geom.AddCylinder(1.0f, 1.0f, 0.5f, false, false, i + 3, col, mTrans);
      //geom.AddCapsule(1.0f, 1.5f, i + 3, i + 1, col, mTrans);
      //geom.AddTorus(1.0f, 1.5f, 16 * (i+1), 3 * (i+1), col);


      ezLog::Info("Polygons: %u, Vertices: %u", geom.GetPolygons().GetCount(), geom.GetVertices().GetCount());

      Vertices.Reserve(geom.GetVertices().GetCount());
      Indices.Reserve(geom.GetPolygons().GetCount() * 6);

      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        MayaObj::Vertex vert;
        vert.pos = geom.GetVertices()[v].m_vPosition;
        vert.norm = geom.GetVertices()[v].m_vNormal;
        vert.tex0.x = geom.GetVertices()[v].m_Color.r;
        vert.tex0.y = geom.GetVertices()[v].m_Color.g;

        Vertices.PushBack(vert);
      }

      for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
      {
        for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
        {
          Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
          Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
          Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
        }
      }
    }
    else
    {
      ezOBJLoader l;
      if (l.LoadOBJ(szPath).Failed())
        return nullptr;

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
    }

    MayaObj* Obj = EZ_DEFAULT_NEW(MayaObj)(Vertices, Indices, pDevice, i);
    return Obj;
  }

  MayaObj::MayaObj(const ezArrayPtr<MayaObj::Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice, int iMesh)
  {
    ezMeshBufferResourceDescriptor desc;
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
    {
      ezStringBuilder s;
      s.Format("MayaMesh%i", iMesh);

      m_hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(s.GetData());

      if (!m_hMeshBuffer.IsValid())
        m_hMeshBuffer = ezResourceManager::CreateResource<ezMeshBufferResource>(s.GetData(), desc);
    }
  }

}