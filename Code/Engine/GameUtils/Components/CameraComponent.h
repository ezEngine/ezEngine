#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <CoreUtils/Graphics/Camera.h>

/// \brief Usage hint of a camera component. Does not necessarily have any effect.
struct EZ_GAMEUTILS_DLL ezCameraComponentUsageHint
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    None,
    MainView,
    Player,
    NPC,
    SecurityCamera,
    SceneThumbnail,

    ENUM_COUNT,

    Default = None,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEUTILS_DLL, ezCameraComponentUsageHint);



typedef ezComponentManager<class ezCameraComponent> ezCameraComponentManager;

class EZ_GAMEUTILS_DLL ezCameraComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCameraComponent, ezComponent, ezCameraComponentManager);

public:
  ezCameraComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  ezEnum<ezCameraComponentUsageHint> m_UsageHint;
  ezEnum<ezCameraMode> m_Mode;
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fPerspectiveFieldOfView;
  float m_fOrthoDimension;

private:


};
