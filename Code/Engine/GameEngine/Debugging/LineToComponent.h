#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

typedef ezComponentManagerSimple<class ezLineToComponent,
  ezComponentUpdateType::Always /*, ezBlockStorageType::Compact -> does not compile */>
  ezLineToComponentManager;

/// \brief Draws a line from its own position to the target object position
class EZ_GAMEENGINE_DLL ezLineToComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLineToComponent, ezComponent, ezLineToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLineToComponent

public:
  ezLineToComponent();
  ~ezLineToComponent();

  const char* GetLineToTargetGuid() const;            // [ property ]
  void SetLineToTargetGuid(const char* szTargetGuid); // [ property ]

  void SetLineToTarget(const ezGameObjectHandle& hTargetObject);                // [ property ]
  const ezGameObjectHandle& GetLineToTarget() const { return m_hTargetObject; } // [ property ]

  ezColor m_LineColor; // [ property ]

protected:
  void Update();

  ezGameObjectHandle m_hTargetObject;
};
