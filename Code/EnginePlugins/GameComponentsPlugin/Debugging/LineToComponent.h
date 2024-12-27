#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

using ezLineToComponentManager = ezComponentManagerSimple<class ezLineToComponent, ezComponentUpdateType::Always, ezBlockStorageType::FreeList, ezWorldUpdatePhase::PostTransform>;

/// \brief Draws a line from its own position to the target object position
class EZ_GAMECOMPONENTS_DLL ezLineToComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezLineToComponent, ezComponent, ezLineToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLineToComponent

public:
  ezLineToComponent();
  ~ezLineToComponent();

  const char* GetLineToTargetGuid() const;                                      // [ property ]
  void SetLineToTargetGuid(const char* szTargetGuid);                           // [ property ]

  void SetLineToTarget(const ezGameObjectHandle& hTargetObject);                // [ property ]
  const ezGameObjectHandle& GetLineToTarget() const { return m_hTargetObject; } // [ property ]

  ezColor m_LineColor;                                                          // [ property ]

  void Update();

protected:
  ezGameObjectHandle m_hTargetObject;
};
