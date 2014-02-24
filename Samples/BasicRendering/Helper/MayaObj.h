
#pragma once


#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec2.h>

#include <RendererFoundation/Basics.h>

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

    ~MayaObj();

    static MayaObj* LoadFromFile(const char* szPath, ezGALDevice* pDevice);

    ezGALBufferHandle GetVB() const { return m_hVB; }

    ezGALBufferHandle GetIB() const { return m_hIB; }

    ezUInt32 GetPrimitiveCount() const { return m_uiPrimitiveCount; }

  protected:

    MayaObj(const ezArrayPtr<Vertex>& pVertices, const ezArrayPtr<ezUInt16>& pIndices, ezGALDevice* pDevice);

  private:

    ezGALBufferHandle m_hVB;

    ezGALBufferHandle m_hIB;

    ezGALDevice* m_pDevice;

    ezUInt32 m_uiPrimitiveCount;

  };
}
