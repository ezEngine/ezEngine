#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

typedef ezComponentManagerSimple<class ezFmodListenerComponent, ezComponentUpdateType::WhenSimulating> ezFmodListenerComponentManager;

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


  // ************************************* FUNCTIONS *****************************


protected:
  void Update();
};


