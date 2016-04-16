#include <GameUtils/PCH.h>
#include <GameUtils/Components/CameraComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCameraComponentUsageHint, 1)
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::None),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::MainView),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::Player),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::NPC),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::SecurityCamera),
  EZ_ENUM_CONSTANT(ezCameraComponentUsageHint::SceneThumbnail),
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_COMPONENT_TYPE(ezCameraComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Usage Hint", ezCameraComponentUsageHint, m_UsageHint),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezCameraMode, m_Mode),
    EZ_MEMBER_PROPERTY("Near Plane", m_fNearPlane)->AddAttributes(new ezDefaultValueAttribute(0.25f), new ezClampValueAttribute(0.0f, 1000000.0f)),
    EZ_MEMBER_PROPERTY("Far Plane", m_fFarPlane)->AddAttributes(new ezDefaultValueAttribute(1000.0f), new ezClampValueAttribute(0.0f, 1000000.0f)),
    EZ_MEMBER_PROPERTY("FOV (perspective)", m_fPerspectiveFieldOfView)->AddAttributes(new ezDefaultValueAttribute(60.0f), new ezClampValueAttribute(1.0f, 179.0f)),
    EZ_MEMBER_PROPERTY("Dimensions (ortho)", m_fOrthoDimension)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, 1000000.0f)),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::DarkSlateBlue),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezCameraComponent::ezCameraComponent()
{
  m_fNearPlane = 0.25f;
  m_fFarPlane = 1000.0f;
  m_fPerspectiveFieldOfView = 60.0f;
  m_fOrthoDimension = 10.0f;
}

void ezCameraComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_UsageHint.GetValue();
  s << m_Mode.GetValue();
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;
}

void ezCameraComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  ezCameraComponentUsageHint::StorageType usage;
  s >> usage; m_UsageHint.SetValue(usage);
  ezCameraMode::StorageType cam;
  s >> cam; m_Mode.SetValue(cam);
  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;
}

