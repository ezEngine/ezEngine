#include <PCH.h>
#include <GameEngine/Components/HeadBoneComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezHeadBoneComponent_SetVerticalRotationMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHeadBoneComponent_SetVerticalRotationMsg, 1, ezRTTIDefaultAllocator<ezHeadBoneComponent_SetVerticalRotationMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Angle", m_Angle),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezHeadBoneComponent_ChangeVerticalRotationMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHeadBoneComponent_ChangeVerticalRotationMsg, 1, ezRTTIDefaultAllocator<ezHeadBoneComponent_ChangeVerticalRotationMsg>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Angle", m_Angle),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezHeadBoneComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(80)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(89.0f))),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezHeadBoneComponent_SetVerticalRotationMsg, SetVerticalRotation),
    EZ_MESSAGE_HANDLER(ezHeadBoneComponent_ChangeVerticalRotationMsg, ChangeVerticalRotation),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Transform"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezHeadBoneComponent::ezHeadBoneComponent()
{
  m_MaxVerticalRotation = ezAngle::Degree(80);
  m_CurVerticalRotation.SetRadian(0);
  m_NewVerticalRotation.SetRadian(0);
}

void ezHeadBoneComponent::Update()
{
  m_NewVerticalRotation = ezMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);
    //GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + GetOwner()->GetLocalRotation() * vAxis * fDistanceDiff);

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
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void ezHeadBoneComponent::SetVerticalRotation(ezHeadBoneComponent_SetVerticalRotationMsg& msg)
{
  m_NewVerticalRotation = ezAngle::Radian((float)msg.m_Angle);
}

void ezHeadBoneComponent::ChangeVerticalRotation(ezHeadBoneComponent_ChangeVerticalRotationMsg& msg)
{
  m_NewVerticalRotation += ezAngle::Radian((float)msg.m_Angle);
}
