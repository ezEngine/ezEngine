#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

typedef ezComponentManagerSimple<class ezFmodListenerComponent, ezComponentUpdateType::WhenSimulating> ezFmodListenerComponentManager;

/// \brief Represents the position of the sound listener
class EZ_FMODPLUGIN_DLL ezFmodListenerComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodListenerComponent, ezFmodComponent, ezFmodListenerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezFmodComponent

private:
  virtual void ezFmodComponentIsAbstract() override {}

  //////////////////////////////////////////////////////////////////////////
  // ezFmodListenerComponent

public:
  ezFmodListenerComponent();
  ~ezFmodListenerComponent();

  ezUInt8 m_uiListenerIndex = 0; // [ property ]

protected:
  void Update();
};
