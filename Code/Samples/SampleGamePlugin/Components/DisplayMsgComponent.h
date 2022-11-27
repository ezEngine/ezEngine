#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

struct ezMsgSetText;
struct ezMsgSetColor;

// BEGIN-DOCS-CODE-SNIPPET: component-manager-simple
using DisplayMsgComponentManager = ezComponentManagerSimple<class DisplayMsgComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;
// END-DOCS-CODE-SNIPPET

class EZ_SAMPLEGAMEPLUGIN_DLL DisplayMsgComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(DisplayMsgComponent, ezComponent, DisplayMsgComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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
