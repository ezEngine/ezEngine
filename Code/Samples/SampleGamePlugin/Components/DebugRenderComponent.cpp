#include <SampleGamePluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/DebugRenderComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(DebugRenderComponentMask, 1)
  EZ_BITFLAGS_CONSTANT(DebugRenderComponentMask::Box),
  EZ_BITFLAGS_CONSTANT(DebugRenderComponentMask::Sphere),
  EZ_BITFLAGS_CONSTANT(DebugRenderComponentMask::Cross),
  EZ_BITFLAGS_CONSTANT(DebugRenderComponentMask::Quad)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(DebugRenderComponent, 2 /* version for serialization */, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    EZ_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_BITFLAGS_MEMBER_PROPERTY("Render", DebugRenderComponentMask, m_RenderTypes)->AddAttributes(new ezDefaultValueAttribute(DebugRenderComponentMask::Box)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGamePlugin"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

DebugRenderComponent::DebugRenderComponent() = default;
DebugRenderComponent::~DebugRenderComponent() = default;

void DebugRenderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fSize;
  s << m_Color;
  s << m_hTexture;
  s << m_RenderTypes;
}

void DebugRenderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fSize;
  s >> m_Color;
  s >> m_hTexture;
  s >> m_RenderTypes;
}

void DebugRenderComponent::SetTexture(const ezTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const ezTexture2DResourceHandle& DebugRenderComponent::GetTexture() const
{
  return m_hTexture;
}

void DebugRenderComponent::SetTextureFile(const char* szFile)
{
  ezTexture2DResourceHandle hTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(szFile);
  }

  SetTexture(hTexture);
}

const char* DebugRenderComponent::GetTextureFile(void) const
{
  if (m_hTexture.IsValid())
    return m_hTexture.GetResourceID();

  return nullptr;
}

void DebugRenderComponent::Update()
{
  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Box))
  {
    ezBoundingBox bbox;
    bbox.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3(m_fSize));

    ezDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Cross))
  {
    ezDebugRenderer::DrawCross(GetWorld(), ezVec3::ZeroVector(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Sphere))
  {
    // BEGIN-DOCS-CODE-SNIPPET: debugrender-sphere
    ezBoundingSphere sphere;
    sphere.SetElements(ezVec3::ZeroVector(), m_fSize);
    ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
    // END-DOCS-CODE-SNIPPET
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Quad) && m_hTexture.IsValid())
  {
    ezHybridArray<ezDebugRenderer::TexturedTriangle, 16> triangles;

    {
      auto& t0 = triangles.ExpandAndGetRef();

      t0.m_position[0].Set(0, -m_fSize, +m_fSize);
      t0.m_position[1].Set(0, +m_fSize, -m_fSize);
      t0.m_position[2].Set(0, -m_fSize, -m_fSize);

      t0.m_texcoord[0].Set(0.0f, 0.0f);
      t0.m_texcoord[1].Set(1.0f, 1.0f);
      t0.m_texcoord[2].Set(0.0f, 1.0f);
    }

    {
      auto& t1 = triangles.ExpandAndGetRef();

      t1.m_position[0].Set(0, -m_fSize, +m_fSize);
      t1.m_position[1].Set(0, +m_fSize, +m_fSize);
      t1.m_position[2].Set(0, +m_fSize, -m_fSize);

      t1.m_texcoord[0].Set(0.0f, 0.0f);
      t1.m_texcoord[1].Set(1.0f, 0.0f);
      t1.m_texcoord[2].Set(1.0f, 1.0f);
    }

    // move the triangles into our object space
    for (auto& tri : triangles)
    {
      tri.m_position[0] = ownerTransform.TransformPosition(tri.m_position[0]);
      tri.m_position[1] = ownerTransform.TransformPosition(tri.m_position[1]);
      tri.m_position[2] = ownerTransform.TransformPosition(tri.m_position[2]);
    }

    ezDebugRenderer::DrawTexturedTriangles(GetWorld(), triangles, m_Color, m_hTexture);
  }
}
