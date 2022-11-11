#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

class ezGeometry;

class EZ_RENDERERCORE_DLL ezRasterizerObject : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRasterizerObject);

public:
  ezRasterizerObject();
  ~ezRasterizerObject();

  void Clear();

  void CreateBox(const ezVec3& vFullExtents, const ezMat4& mTransform);
  void CreateSphere(float fRadius, const ezMat4& mTransform);

  void CreateMesh(const ezGeometry& geometry);

  const Occluder& GetInternalOccluder() const
  {
    return m_Occluder;
  }

private:
  Occluder m_Occluder;
};
