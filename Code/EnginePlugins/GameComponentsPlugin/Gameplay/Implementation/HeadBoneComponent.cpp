#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameComponentsPlugin/Gameplay/HeadBoneComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezHeadBoneComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("VerticalRotation", m_MaxVerticalRotation)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(80)), new ezClampValueAttribute(ezAngle::MakeFromDegree(0.0f), ezAngle::MakeFromDegree(89.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetVerticalRotation, In, "Radians"),
    EZ_SCRIPT_FUNCTION_PROPERTY(ChangeVerticalRotation, In, "Radians"),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHeadBoneComponent::ezHeadBoneComponent() = default;
ezHeadBoneComponent::~ezHeadBoneComponent() = default;

void ezHeadBoneComponent::Update()
{
  m_NewVerticalRotation = ezMath::Clamp(m_NewVerticalRotation, -m_MaxVerticalRotation, m_MaxVerticalRotation);

  ezQuat qOld, qNew;
  qOld = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), m_CurVerticalRotation);
  qNew = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), m_NewVerticalRotation);

  const ezQuat qChange = qNew * qOld.GetInverse();

  const ezQuat qFinalNew = qChange * GetOwner()->GetLocalRotation();

  GetOwner()->SetLocalRotation(qFinalNew);

  m_CurVerticalRotation = m_NewVerticalRotation;
}

void ezHeadBoneComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  // Version 1
  s << m_MaxVerticalRotation;
  s << m_CurVerticalRotation;
}

void ezHeadBoneComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  // Version 1
  s >> m_MaxVerticalRotation;
  s >> m_CurVerticalRotation;
}

void ezHeadBoneComponent::SetVerticalRotation(float fRadians)
{
  m_NewVerticalRotation = ezAngle::MakeFromRadian(fRadians);
}

void ezHeadBoneComponent::ChangeVerticalRotation(float fRadians)
{
  m_NewVerticalRotation += ezAngle::MakeFromRadian(fRadians);
}


EZ_STATICLINK_FILE(GameComponentsPlugin, GameComponentsPlugin_Gameplay_Implementation_HeadBoneComponent);
