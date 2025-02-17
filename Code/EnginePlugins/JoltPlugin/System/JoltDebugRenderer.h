#pragma once

#ifdef JPH_DEBUG_RENDERER

#  include <RendererCore/Debug/DebugRenderer.h>

class ezJoltDebugRenderer : public JPH::DebugRenderer
{
public:
  ezDynamicArray<ezDebugRendererLine> m_Lines;
  ezDynamicArray<ezDebugRendererTriangle> m_Triangles;

  struct TriangleBatch : public JPH::RefTargetVirtual
  {
    ezDynamicArray<ezDebugRendererTriangle> m_Triangles;

    int m_iRefCount = 0;

    virtual void AddRef() override;

    virtual void Release() override;
  };

  ezJoltDebugRenderer();

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

  virtual void DrawTriangle(JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override;

  virtual Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* pInTriangles, int iInTriangleCount) override;

  virtual Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* pInVertices, int iInVertexCount, const JPH::uint32* pInIndices, int iInIndexCount) override;

  virtual void DrawGeometry(JPH::Mat44Arg modelMatrix, const JPH::AABox& worldSpaceBounds, float fInLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& geometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On, EDrawMode inDrawMode = EDrawMode::Solid) override;

  virtual void DrawText3D(JPH::Vec3Arg inPosition, const JPH::string_view& string, JPH::ColorArg inColor = JPH::Color::sWhite, float fInHeight = 0.5f) override {}
};


#endif
