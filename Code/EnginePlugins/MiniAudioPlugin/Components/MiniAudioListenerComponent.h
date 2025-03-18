#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <MiniAudioPlugin/MiniAudioPluginDLL.h>

class ezMiniAudioListenerComponentManager : public ezComponentManager<class ezMiniAudioListenerComponent, ezBlockStorageType::Compact>
{
public:
  ezMiniAudioListenerComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateListeners(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Represents the position of the sound listener
class EZ_MINIAUDIOPLUGIN_DLL ezMiniAudioListenerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMiniAudioListenerComponent, ezComponent, ezMiniAudioListenerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezMiniAudioListenerComponent

public:
  ezMiniAudioListenerComponent();
  ~ezMiniAudioListenerComponent();

protected:
  void Update();
};
