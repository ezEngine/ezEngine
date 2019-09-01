#pragma once

#include <GameEngine/Animation/TransformComponent.h>
#include <GameEngine/GameEngineDLL.h>

typedef ezComponentManagerSimple<class ezHeadBoneComponent, ezComponentUpdateType::WhenSimulating> ezHeadBoneComponentManager;

class EZ_GAMEENGINE_DLL ezHeadBoneComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeadBoneComponent, ezComponent, ezHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezHeadBoneComponent interface

public:
  ezHeadBoneComponent();

  void Update();

  void SetVerticalRotation(float radians);    // [scriptable]
  void ChangeVerticalRotation(float radians); // [scriptable]

  ezAngle m_MaxVerticalRotation; // [property]

private:
  ezAngle m_CurVerticalRotation;
  ezAngle m_NewVerticalRotation;
};
