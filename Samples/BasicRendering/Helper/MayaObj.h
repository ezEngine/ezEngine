
#pragma once


#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec2.h>

#include <RendererFoundation/Basics.h>

#include <RendererCore/Meshes/MeshBufferResource.h>

namespace DontUse
{
  class MayaObj
  {
  public:

    struct Vertex
    {
      ezVec3 pos;
      ezVec3 norm;
      ezVec2 tex0;
    };

    static MayaObj* LoadFromFile(const char* szPath, ezGALDevice* pDevice, int i);

    ezMeshBufferResourceHandle m_hMeshBuffer;

  protected:

    MayaObj(const ezArrayPtr<Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice, int iMesh);


  };
}
