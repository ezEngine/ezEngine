#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

typedef ezComponentManagerSimple<class ezLineToComponent,
  ezComponentUpdateType::Always /*, ezBlockStorageType::Compact -> does not compile */>
  ezLineToComponentManager;

class EZ_GAMEENGINE_DLL ezLineToComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLineToComponent, ezComponent, ezLineToComponentManager);

public:
  ezLineToComponent();
  ~ezLineToComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  const char* GetLineToTargetGuid() const;
  void SetLineToTargetGuid(const char* szTargetGuid);

  void SetLineToTarget(const ezGameObjectHandle& hTargetObject);
  const ezGameObjectHandle& GetLineToTarget() const { return m_hTargetObject; }

  ezColor m_LineColor;

private:
  ezGameObjectHandle m_hTargetObject;
};
