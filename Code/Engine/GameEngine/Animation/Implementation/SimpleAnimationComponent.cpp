#include <PCH.h>
#include <GameEngine/Animation/SimpleAnimationComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <RendererFoundation/Device/Device.h>

EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 2, ezComponentMode::Static);
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

ezSimpleAnimationComponent::ezSimpleAnimationComponent()
{
  m_Rotation.SetRadian(0);
  m_DegreePerSecond = ezAngle::Degree(90);
}

ezSimpleAnimationComponent::~ezSimpleAnimationComponent()
{
}

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

void ezSimpleAnimationComponent::Update()
{
  m_Rotation += (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * m_DegreePerSecond;

  m_BoneMatrices.SetCountUninitialized(1);
  m_BoneMatrices[0].SetRotationMatrixZ(m_Rotation);

  m_SkinningMatrices = ezArrayPtr<ezMat4>(m_BoneMatrices);

  if (m_hSkinningTransformsBuffer.IsInvalidated())
  {
    // Create the buffer for the skinning matrices
    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezMat4);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_BoneMatrices.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hSkinningTransformsBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, ezArrayPtr<ezUInt8>(reinterpret_cast<ezUInt8*>(m_BoneMatrices.GetData()), BufferDesc.m_uiTotalSize));
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SimpleAnimationComponent);

