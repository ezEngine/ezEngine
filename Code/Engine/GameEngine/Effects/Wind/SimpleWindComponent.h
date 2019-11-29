#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

typedef ezComponentManagerSimple<class ezSimpleWindComponent, ezComponentUpdateType::WhenSimulating> ezSimpleWindComponentManager;

class EZ_GAMEENGINE_DLL ezSimpleWindComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSimpleWindComponent, ezComponent, ezSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSimpleWindComponent

public:
  ezSimpleWindComponent();
  ~ezSimpleWindComponent();

  float m_fWindStrengthMin = 0; // [ property ]
  float m_fWindStrengthMax = 0; // [ property ]
  ezAngle m_Deviation;          // [ property ]

protected:
  void Update();
  void ComputeNextState();

  float m_fLastStrength = 0;
  float m_fNextStrength = 0;
  ezVec3 m_vLastDirection;
  ezVec3 m_vNextDirection;
  ezTime m_LastChange;
  ezTime m_NextChange;
};
