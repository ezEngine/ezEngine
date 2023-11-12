#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using ezSimpleWindComponentManager = ezComponentManagerSimple<class ezSimpleWindComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_GAMEENGINE_DLL ezSimpleWindComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSimpleWindComponent, ezComponent, ezSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSimpleWindComponent

public:
  ezSimpleWindComponent();
  ~ezSimpleWindComponent();

  ezEnum<ezWindStrength> m_MinWindStrength; // [ property ]
  ezEnum<ezWindStrength> m_MaxWindStrength; // [ property ]

  ezAngle m_Deviation; // [ property ]

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
