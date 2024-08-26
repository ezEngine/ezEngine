#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

class ezLineToComponentManager : public ezComponentManager<class ezLineToComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<class ezLineToComponent, ezBlockStorageType::FreeList>;

public:
  ezLineToComponentManager(ezWorld* pWorld);

protected:
  void Initialize() override;
  void Update(const ezWorldModule::UpdateContext& context);
};

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

protected:
  void Update();

  ezGameObjectHandle m_hTargetObject;
};
