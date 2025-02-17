#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleRenderComponent.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(SampleRenderComponentMask, 1)
  EZ_BITFLAGS_CONSTANT(SampleRenderComponentMask::Box),
  EZ_BITFLAGS_CONSTANT(SampleRenderComponentMask::Sphere),
  EZ_BITFLAGS_CONSTANT(SampleRenderComponentMask::Cross),
  EZ_BITFLAGS_CONSTANT(SampleRenderComponentMask::Quad)
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_COMPONENT_TYPE(SampleRenderComponent, 1 /* version */, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColor::White)),
    EZ_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    EZ_BITFLAGS_MEMBER_PROPERTY("Render", SampleRenderComponentMask, m_RenderTypes)->AddAttributes(new ezDefaultValueAttribute(SampleRenderComponentMask::Box)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Game"), // Component menu group
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
// clang-format on

SampleRenderComponent::SampleRenderComponent() = default;
SampleRenderComponent::~SampleRenderComponent() = default;

void SampleRenderComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (OWNTYPE::GetStaticRTTI()->GetTypeVersion() == 1)
  {
    // this automatically serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual serialization
    ezReflectionSerializer::WriteObjectToBinary(s, GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization, for example:
    // s << m_fSize;
    // s << m_Color;
    // s << m_hTexture;
    // s << m_RenderTypes;
  }
}

void SampleRenderComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    // this automatically de-serializes all properties
    // if you need more control, increase the component 'version' at the top of this file
    // and then use the code path below for manual de-serialization
    ezReflectionSerializer::ReadObjectPropertiesFromBinary(s, *GetDynamicRTTI(), this);
  }
  else
  {
    // do custom de-serialization, for example:
    // s >> m_fSize;
    // s >> m_Color;
    // s >> m_hTexture;
    // s >> m_RenderTypes;
  }
}

void SampleRenderComponent::OnSetColor(ezMsgSetColor& msg)
{
  m_Color = msg.m_Color;
}

void SampleRenderComponent::SetRandomColor()
{
  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  m_Color.r = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.g = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
  m_Color.b = static_cast<float>(rng.DoubleMinMax(0.2f, 1.0f));
}

void SampleRenderComponent::Update()
{
  const ezTransform ownerTransform = GetOwner()->GetGlobalTransform();

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Box))
  {
    ezBoundingBox bbox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3(m_fSize));

    ezDebugRenderer::DrawLineBox(GetWorld(), bbox, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Cross))
  {
    ezDebugRenderer::DrawCross(GetWorld(), ezVec3::MakeZero(), m_fSize, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Sphere))
  {
    ezBoundingSphere sphere = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fSize);
    ezDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, ownerTransform);
  }

  if (m_RenderTypes.IsSet(SampleRenderComponentMask::Quad) && m_hTexture.IsValid())
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
}
