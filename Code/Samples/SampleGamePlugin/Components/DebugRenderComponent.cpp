#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
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

// BEGIN-DOCS-CODE-SNIPPET: component-reflection-block
EZ_BEGIN_COMPONENT_TYPE(DebugRenderComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    EZ_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    EZ_BITFLAGS_MEMBER_PROPERTY("Render", DebugRenderComponentMask, m_RenderTypes)->AddAttributes(new ezDefaultValueAttribute(DebugRenderComponentMask::Box)),

    // BEGIN-DOCS-CODE-SNIPPET: customdata-property
    EZ_RESOURCE_MEMBER_PROPERTY("CustomData", m_hCustomData)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_CustomData", "SampleCustomData")),
    // END-DOCS-CODE-SNIPPET
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGamePlugin"), // Component menu group
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnSetColor)
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetRandomColor)
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// END-DOCS-CODE-SNIPPET
// clang-format on

DebugRenderComponent::DebugRenderComponent() = default;
DebugRenderComponent::~DebugRenderComponent() = default;

void DebugRenderComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fSize;
  s << m_Color;
  s << m_hTexture;
  s << m_RenderTypes;
  s << m_hCustomData;
}

void DebugRenderComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fSize;
  s >> m_Color;
  s >> m_hTexture;
  s >> m_RenderTypes;
  s >> m_hCustomData;
}

void DebugRenderComponent::OnSetColor(ezMsgSetColor& ref_msg)
{
  m_Color = ref_msg.m_Color;
}

void DebugRenderComponent::SetRandomColor()
{
  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  m_Color.r = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.g = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.b = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
}

void DebugRenderComponent::Update()
{
  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Box))
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3(m_fSize));

    ezDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Cross))
  {
    ezDebugRenderer::DrawCross(GetWorld(), ezVec3::MakeZero(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Sphere))
  {
    // BEGIN-DOCS-CODE-SNIPPET: debugrender-sphere
    ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fSize);
    ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
    // END-DOCS-CODE-SNIPPET
  }

  if (m_RenderTypes.IsSet(DebugRenderComponentMask::Quad) && m_hTexture.IsValid())
  {
    ezHybridArray<ezDebugRendererTexturedTriangle, 16> triangles;

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

  // accessing custom data resources
  if (m_hCustomData.IsValid())
  {
    // BEGIN-DOCS-CODE-SNIPPET: customdata-access
    ezResourceLock<SampleCustomDataResource> pCustomDataResource(m_hCustomData, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pCustomDataResource.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      const SampleCustomData* pCustomData = pCustomDataResource->GetData();

      ezDebugRenderer::Draw3DText(GetWorld(), ezFmt(pCustomData->m_sText), GetOwner()->GetGlobalPosition(), pCustomData->m_Color, pCustomData->m_iSize);
    }
    // END-DOCS-CODE-SNIPPET
  }
}
