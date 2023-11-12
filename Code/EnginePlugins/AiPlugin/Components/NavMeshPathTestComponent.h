#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using ezNavMeshPathTestComponentManager = ezComponentManagerSimple<class ezAiNavMeshPathTestComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_AIPLUGIN_DLL ezAiNavMeshPathTestComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiNavMeshPathTestComponent, ezComponent, ezNavMeshPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  ezAiNavMeshPathTestComponent

public:
  ezAiNavMeshPathTestComponent();
  ~ezAiNavMeshPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(ezGameObjectHandle hObject);

  bool m_bVisualizePathCorridor = true;
  bool m_bVisualizePathLine = true;
  bool m_bVisualizePathState = true;
  ezHashedString m_sNavmeshConfig;
  ezHashedString m_sPathSearchConfig;

protected:
  void Update();

  ezGameObjectHandle m_hPathEnd;
  ezAiNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
