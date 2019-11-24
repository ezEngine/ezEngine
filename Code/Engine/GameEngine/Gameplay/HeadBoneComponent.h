#pragma once

#include <GameEngine/Animation/TransformComponent.h>
#include <GameEngine/GameEngineDLL.h>

typedef ezComponentManagerSimple<class ezHeadBoneComponent, ezComponentUpdateType::WhenSimulating> ezHeadBoneComponentManager;

class EZ_GAMEENGINE_DLL ezHeadBoneComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeadBoneComponent, ezComponent, ezHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezHeadBoneComponent

public:
  ezHeadBoneComponent();
  ~ezHeadBoneComponent();


  void SetVerticalRotation(float radians);    // [ scriptable ]
  void ChangeVerticalRotation(float radians); // [ scriptable ]

  ezAngle m_NewVerticalRotation;                       // [ property ]
  ezAngle m_MaxVerticalRotation = ezAngle::Degree(80); // [ property ]

protected:
  void Update();

  ezAngle m_CurVerticalRotation;
};
