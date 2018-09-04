#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Components/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetVerticalHeadBoneRotation);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetVerticalHeadBoneRotation, 1, ezRTTIDefaultAllocator<ezMsgSetVerticalHeadBoneRotation>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Angle", m_Angle),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgChangeVerticalHeadBoneRotation);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgChangeVerticalHeadBoneRotation, 1, ezRTTIDefaultAllocator<ezMsgChangeVerticalHeadBoneRotation>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Angle", m_Angle),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezHeadBoneComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(80)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(89.0f))),
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSetVerticalHeadBoneRotation, OnSetVerticalRotation),
    EZ_MESSAGE_HANDLER(ezMsgChangeVerticalHeadBoneRotation, OnChangeVerticalRotation),
  }
  EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Transform"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHeadBoneComponent::ezHeadBoneComponent()
{
  m_MaxVerticalRotation = ezAngle::Degree(80);
  m_CurVerticalRotation.SetRadian(0);
  m_NewVerticalRotation.SetRadian(0);
}

void ezHeadBoneComponent::Update()
{
  m_NewVerticalRotation = ezMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);
  // GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + GetOwner()->GetLocalRotation() * vAxis * fDistanceDiff);

  ezQuat qOld, qNew;
  qOld.SetFromAxisAndAngle(ezVec3(0, 1, 0), m_CurVerticalRotation);
  qNew.SetFromAxisAndAngle(ezVec3(0, 1, 0), m_NewVerticalRotation);

  const ezQuat qChange = qNew * -qOld;

  const ezQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void ezHeadBoneComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void ezHeadBoneComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void ezHeadBoneComponent::OnSetVerticalRotation(ezMsgSetVerticalHeadBoneRotation& msg)
{
  m_NewVerticalRotation = ezAngle::Radian((float)msg.m_Angle);
}

void ezHeadBoneComponent::OnChangeVerticalRotation(ezMsgChangeVerticalHeadBoneRotation& msg)
{
  m_NewVerticalRotation += ezAngle::Radian((float)msg.m_Angle);
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_HeadBoneComponent);
