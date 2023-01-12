#pragma once

#ifdef JPH_DEBUG_RENDERER

#  include <RendererCore/Debug/DebugRenderer.h>

class ezJoltDebugRenderer : public JPH::DebugRenderer
{
public:
  ezDynamicArray<ezDebugRenderer::Line> m_Lines;
  ezDynamicArray<ezDebugRenderer::Triangle> m_Triangles;

  struct TriangleBatch : public JPH::RefTargetVirtual
  {
    ezDynamicArray<ezDebugRenderer::Triangle> m_Triangles;

    int m_iRefCount = 0;

    virtual void AddRef() override;

    virtual void Release() override;
  };

  ezJoltDebugRenderer();

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

  virtual void DrawTriangle(JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor) override;

  virtual Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount) override;

  virtual Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override;

  virtual void DrawGeometry(JPH::Mat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On, EDrawMode inDrawMode = EDrawMode::Solid) override;

  virtual void DrawText3D(JPH::Vec3Arg inPosition, const JPH::string_view& inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override {}
};


#endif
