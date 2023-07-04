#pragma once

#include <FmodPlugin/Components/FmodComponent.h>

class ezFmodListenerComponentManager : public ezComponentManager<class ezFmodListenerComponent, ezBlockStorageType::Compact>
{
public:
  ezFmodListenerComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateListeners(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Represents the position of the sound listener
class EZ_FMODPLUGIN_DLL ezFmodListenerComponent : public ezFmodComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFmodListenerComponent, ezFmodComponent, ezFmodListenerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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
