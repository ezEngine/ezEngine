#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct ezMsgSetText;
struct ezMsgSetColor;

typedef ezComponentManagerSimple<class DisplayMsgComponent, ezComponentUpdateType::Always> DisplayMsgComponentManager;

class EZ_SAMPLEGAMEPLUGIN_DLL DisplayMsgComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DisplayMsgComponent, ezComponent, DisplayMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // DisplayMsgComponent

public:
  DisplayMsgComponent();
  ~DisplayMsgComponent();

private:
  void Update();

  void OnSetText(ezMsgSetText& msg);   // [ msg handler ]
  void OnSetColor(ezMsgSetColor& msg); // [ msg handler ]

  ezString m_sCurrentText;
  ezColor m_TextColor = ezColor::Yellow;
};
