#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct ezMsgComponentInternalTrigger;

// This component manager does literally nothing, meaning the managed components do not need to be update, at all
class SendMsgComponentManager : public ezComponentManager<class SendMsgComponent, ezBlockStorageType::Compact>
{
public:
  SendMsgComponentManager(ezWorld* pWorld);
  ~SendMsgComponentManager() = default;
};

class EZ_SAMPLEGAMEPLUGIN_DLL SendMsgComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(SendMsgComponent, ezComponent, SendMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // SendMsgComponent

public:
  SendMsgComponent();
  ~SendMsgComponent();

private:
  ezDynamicArray<ezString> m_TextArray; // [ property ]

  void OnSendText(ezMsgComponentInternalTrigger& msg); // [ msg handler ]

  ezUInt32 m_uiNextString = 0;
};
