#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 3, ezComponentMode::Dynamic);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
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

ezSimpleAnimationComponent::ezSimpleAnimationComponent() {}

ezSimpleAnimationComponent::~ezSimpleAnimationComponent() {}

void ezSimpleAnimationComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  ezAngle m_DegreePerSecond;
  s << m_DegreePerSecond;
  s << m_hAnimationClip;
}

void ezSimpleAnimationComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  ezAngle m_DegreePerSecond;
  s >> m_DegreePerSecond;
  s >> m_hAnimationClip;
}

void ezSimpleAnimationComponent::OnActivated()
{
  SUPER::OnActivated();

  // make sure the skinning buffer is deleted
  EZ_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");

  {
    m_BoneMatrices.SetCountUninitialized(100);
    m_SkinningMatrices = ezArrayPtr<ezMat4>(m_BoneMatrices);

    for (ezUInt32 i = 0; i < m_BoneMatrices.GetCount(); ++i)
    {
      m_BoneMatrices[i].SetIdentity();
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

void ezSimpleAnimationComponent::SetAnimationClip(const ezAnimationClipResourceHandle& hResource)
{
  m_hAnimationClip = hResource;
}

void ezSimpleAnimationComponent::SetAnimationClipFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* ezSimpleAnimationComponent::GetAnimationClipFile() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}

void ezSimpleAnimationComponent::Update()
{
  if (!m_hAnimationClip.IsValid())
    return;

  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip);

  const ezTime animDuration = pAnimClip->GetDescriptor().GetDuration();

  if (animDuration < ezTime::Zero())
    return;

  m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

  // loop the animation
  while (m_AnimationTime >= animDuration)
  {
    m_AnimationTime -= animDuration;
  }

  double fAnimLerp = 0;
  const ezUInt32 uiFirstFrame = pAnimClip->GetDescriptor().GetFrameAt(m_AnimationTime, fAnimLerp);

  const ezMat4* pAnimBoneTransforms = pAnimClip->GetDescriptor().GetFirstBones(uiFirstFrame);

  for (ezUInt32 i = 0; i < m_BoneMatrices.GetCount(); ++i)
  {
    m_BoneMatrices[i] = pAnimBoneTransforms[i];
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SimpleAnimationComponent);
