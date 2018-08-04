#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SimpleAnimationComponent.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 2, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_MEMBER_PROPERTY("DegreePerSecond", m_DegreePerSecond)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90))),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSimpleAnimationComponent::ezSimpleAnimationComponent()
{
  m_Rotation.SetRadian(0);
  m_DegreePerSecond = ezAngle::Degree(90);
}

ezSimpleAnimationComponent::~ezSimpleAnimationComponent() {}

void ezSimpleAnimationComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_DegreePerSecond;
}

void ezSimpleAnimationComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_DegreePerSecond;
}

void ezSimpleAnimationComponent::OnActivated()
{
  SUPER::OnActivated();

  // make sure the skinning buffer is deleted
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  {
    ezMat4 rotMat;
    rotMat.SetRotationMatrixZ(m_Rotation);

    m_BoneMatrices.SetCountUninitialized(100);
    m_SkinningMatrices = ezArrayPtr<ezMat4>(m_BoneMatrices);

    for (ezUInt32 i = 0; i < m_BoneMatrices.GetCount(); ++i)
    {
      m_BoneMatrices[i] = rotMat;
    }

    // Create the buffer for the skinning matrices
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_BoneMatrices.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(
        BufferDesc, ezArrayPtr<ezUInt8>(reinterpret_cast<ezUInt8*>(m_BoneMatrices.GetData()), BufferDesc.m_uiTotalSize));
  }
}

void ezSimpleAnimationComponent::Update()
{
  m_Rotation += (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * m_DegreePerSecond;

  ezMat4 rotMat;
  rotMat.SetRotationMatrixZ(m_Rotation);

  for (ezUInt32 i = 0; i < m_BoneMatrices.GetCount(); ++i)
  {
    m_BoneMatrices[i] = rotMat;
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SimpleAnimationComponent);
