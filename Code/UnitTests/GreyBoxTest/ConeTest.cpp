#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Strings/StringBuilder.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

EZ_TESTFRAMEWORK_ENTRY_POINT("GreyBoxTest", "GreyBox Tests")
EZ_CREATE_SIMPLE_TEST_GROUP(GreyBox);

namespace
{
  class ConeBuilder : public ezGreyBoxComponent
  {
  public:
    ConeBuilder()
    {
      SetShape(ezGreyBoxShape::Cone);
      SetSizeNegX(0.5f);
      SetSizePosX(0.5f);
      SetSizeNegY(0.5f);
      SetSizePosY(0.5f);
      SetSizeNegZ(0.5f);
      SetSizePosZ(0.5f);
    }
    void Build(ezGeometry& geometry, bool rough = false) const { BuildGeometry(geometry, GetShape(), rough); }
    ezString Name() const
    {
      ezStringBuilder name;
      GenerateMeshName(name);
      return name;
    }
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(GreyBox, Cone)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Closed cones, frustums and inverted cones have non-degenerate outward triangles")
  {
    for (ezUInt32 sides : {3u, 4u, 32u, 128u})
      for (ezUInt32 segments : {1u, 8u})
        for (float top : {0.0f, 0.5f, 1.0f})
          for (bool smooth : {false, true})
          {
            ConeBuilder builder;
            builder.SetSides(sides);
            builder.SetHeightSegments(segments);
            builder.SetTopRadiusScale(top);
            builder.SetSmoothShading(smooth);
            ezGeometry geometry;
            builder.Build(geometry);
            EZ_TEST_INT(geometry.GetPolygons().GetCount(), sides * segments + (top == 0 ? 1 : 2));
            for (const auto& vertex : geometry.GetVertices())
            {
              EZ_TEST_BOOL(vertex.m_vPosition.IsValid());
              EZ_TEST_BOOL(vertex.m_vNormal.IsNormalized(0.001f));
            }
            geometry.TriangulatePolygons();
            double volume = 0;
            for (const auto& polygon : geometry.GetPolygons())
            {
              const auto a = geometry.GetVertices()[polygon.m_Vertices[0]].m_vPosition;
              const auto b = geometry.GetVertices()[polygon.m_Vertices[1]].m_vPosition;
              const auto c = geometry.GetVertices()[polygon.m_Vertices[2]].m_vPosition;
              EZ_TEST_BOOL((b - a).CrossRH(c - a).GetLengthSquared() > 0);
              volume += a.Dot(b.CrossRH(c)) / 6.0;
            }
            EZ_TEST_BOOL(volume > 0);
          }
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Inverted cone and asymmetric extents")
  {
    ConeBuilder builder;
    builder.SetBaseRadiusScale(0);
    builder.SetTopRadiusScale(1);
    builder.SetSides(4);
    builder.SetHeightSegments(1);
    builder.SetSizeNegX(2);
    builder.SetSizePosX(4);
    ezGeometry geometry;
    builder.Build(geometry);
    EZ_TEST_INT(geometry.GetPolygons().GetCount(), 5);
    ezBoundingBox bounds = ezBoundingBox::MakeInvalid();
    for (const auto& vertex : geometry.GetVertices())
      bounds.ExpandToInclude(vertex.m_vPosition);
    EZ_TEST_BOOL(bounds.m_vMin.IsEqual(ezVec3(-2, -0.5f, -0.5f), 0.00001f));
    EZ_TEST_BOOL(bounds.m_vMax.IsEqual(ezVec3(4, 0.5f, 0.5f), 0.00001f));
    geometry.TriangulatePolygons();
    for (const auto& polygon : geometry.GetPolygons())
    {
      const auto a = geometry.GetVertices()[polygon.m_Vertices[0]].m_vPosition;
      const auto b = geometry.GetVertices()[polygon.m_Vertices[1]].m_vPosition;
      const auto c = geometry.GetVertices()[polygon.m_Vertices[2]].m_vPosition;
      EZ_TEST_BOOL((b - a).CrossRH(c - a).GetLengthSquared() > 0);
    }
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Square pyramid bounds and flat shading")
  {
    ConeBuilder builder;
    builder.SetSides(4);
    builder.SetHeightSegments(1);
    builder.SetSmoothShading(false);
    ezGeometry geometry;
    builder.Build(geometry);
    ezBoundingBox bounds = ezBoundingBox::MakeInvalid();
    for (const auto& vertex : geometry.GetVertices())
      bounds.ExpandToInclude(vertex.m_vPosition);
    EZ_TEST_BOOL(bounds.m_vMin.IsEqual(ezVec3(-0.5f), 0.00001f));
    EZ_TEST_BOOL(bounds.m_vMax.IsEqual(ezVec3(0.5f), 0.00001f));
    for (const auto& polygon : geometry.GetPolygons())
      for (ezUInt32 i : polygon.m_Vertices)
        EZ_TEST_BOOL(geometry.GetVertices()[i].m_vNormal == geometry.GetVertices()[polygon.m_Vertices[0]].m_vNormal);
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Profile, mesh cache and occluder preserve the same surface")
  {
    ConeBuilder builder;
    builder.SetHeightSegments(2);
    builder.SetTopRadiusScale(0.5f);
    for (float curve : {-0.8f, 0.0f, 0.8f})
    {
      builder.SetProfileCurve(curve);
      ezGeometry geometry, rough;
      builder.Build(geometry);
      builder.Build(rough, true);
      EZ_TEST_INT(geometry.GetVertices().GetCount(), rough.GetVertices().GetCount());
      float radius = 0;
      for (const auto& vertex : geometry.GetVertices())
        if (vertex.m_vPosition.z == 0)
          radius = ezMath::Max(radius, vertex.m_vPosition.GetLength());
      EZ_TEST_FLOAT(radius, 0.375f * (1.0f + curve), 0.00001f);
    }
    ezString name = builder.Name();
    builder.SetSides(7);
    EZ_TEST_BOOL(name != builder.Name());
    name = builder.Name();
    builder.SetBaseRadiusScale(0.8f);
    EZ_TEST_BOOL(name != builder.Name());
    name = builder.Name();
    builder.SetTopRadiusScale(0.2f);
    EZ_TEST_BOOL(name != builder.Name());
    name = builder.Name();
    builder.SetHeightSegments(3);
    EZ_TEST_BOOL(name != builder.Name());
    name = builder.Name();
    builder.SetProfileCurve(0.1f);
    EZ_TEST_BOOL(name != builder.Name());
    name = builder.Name();
    builder.SetSmoothShading(false);
    EZ_TEST_BOOL(name != builder.Name());
    builder.SetSides(0);
    EZ_TEST_INT(builder.GetSides(), 3);
    builder.SetHeightSegments(1000);
    EZ_TEST_INT(builder.GetHeightSegments(), 64);
    builder.SetBaseRadiusScale(0);
    builder.SetTopRadiusScale(0);
    ezGeometry fallback;
    builder.Build(fallback);
    EZ_TEST_BOOL(!fallback.GetVertices().IsEmpty());
  }
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "World serialization preserves cone settings")
  {
    ezWorldDesc worldDesc("Cone");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());
    ezGameObject* object = nullptr;
    world.CreateObject(ezGameObjectDesc(), object);
    ezGreyBoxComponent* cone = nullptr;
    ezGreyBoxComponent::CreateComponent(object, cone);
    cone->SetActiveFlag(false);
    cone->SetShape(ezGreyBoxShape::Cone);
    cone->SetBaseRadiusScale(0.8f);
    cone->SetTopRadiusScale(0.25f);
    cone->SetSides(7);
    cone->SetHeightSegments(3);
    cone->SetProfileCurve(-0.4f);
    cone->SetSmoothShading(false);
    ezDefaultMemoryStreamStorage storage;
    ezMemoryStreamWriter output(&storage);
    ezWorldWriter writer;
    writer.WriteWorld(output, world);
    ezMemoryStreamReader input(&storage);
    ezWorldReader reader;
    EZ_TEST_BOOL(reader.ReadWorldDescription(input).Succeeded());
    ezWorldDesc restoredDesc("Restored");
    ezWorld restored(restoredDesc);
    EZ_LOCK(restored.GetWriteMarker());
    reader.InstantiateWorld(restored);
    ezUInt32 count = 0;
    for (auto it = restored.GetObjects(); it.IsValid(); ++it)
    {
      ezGreyBoxComponent* result = nullptr;
      if (!it->TryGetComponentOfBaseType(result))
        continue;
      ++count;
      EZ_TEST_INT(result->GetShape().GetValue(), ezGreyBoxShape::Cone);
      EZ_TEST_FLOAT(result->GetBaseRadiusScale(), 0.8f, 0.00001f);
      EZ_TEST_FLOAT(result->GetTopRadiusScale(), 0.25f, 0.00001f);
      EZ_TEST_INT(result->GetSides(), 7);
      EZ_TEST_INT(result->GetHeightSegments(), 3);
      EZ_TEST_FLOAT(result->GetProfileCurve(), -0.4f, 0.00001f);
      EZ_TEST_BOOL(!result->GetSmoothShading());
    }
    EZ_TEST_INT(count, 1);
  }
}
