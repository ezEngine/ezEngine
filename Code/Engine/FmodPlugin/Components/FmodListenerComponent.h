#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

typedef ezComponentManagerSimple<class ezFmodListenerComponent, true> ezFmodListenerComponentManager;

class EZ_FMODPLUGIN_DLL ezFmodListenerComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodListenerComponent, ezFmodComponent, ezFmodListenerComponentManager);
  virtual void ezFmodComponentIsAbstract() override {}

public:
  ezFmodListenerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:
  ezUInt8 m_uiListenerIndex;


protected:


  // ************************************* FUNCTIONS *****************************

public:
  virtual ezComponent::Initialization Initialize() override;
  virtual void Deinitialize() override;

protected:
  void Update();
};


