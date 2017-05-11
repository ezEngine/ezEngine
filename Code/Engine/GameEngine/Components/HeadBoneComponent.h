#pragma once

#include <GameEngine/Basics.h>
#include <GameEngine/Components/TransformComponent.h>
#include <Core/Messages/ScriptFunctionMessage.h>

typedef ezComponentManagerSimple<class ezHeadBoneComponent, ezComponentUpdateType::WhenSimulating> ezHeadBoneComponentManager;

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezHeadBoneComponent_SetVerticalRotationMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezHeadBoneComponent_SetVerticalRotationMsg, ezScriptFunctionMessage);

  double m_Angle; // in radians
};

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezHeadBoneComponent_ChangeVerticalRotationMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezHeadBoneComponent_ChangeVerticalRotationMsg, ezScriptFunctionMessage);

  double m_Angle; // in radians
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezHeadBoneComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezHeadBoneComponent, ezComponent, ezHeadBoneComponentManager);

public:
  ezHeadBoneComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetVerticalRotation(ezHeadBoneComponent_SetVerticalRotationMsg& msg);
  void ChangeVerticalRotation(ezHeadBoneComponent_ChangeVerticalRotationMsg& msg);

public:
  ezAngle m_MaxVerticalRotation;

private:
  ezAngle m_CurVerticalRotation;
  ezAngle m_NewVerticalRotation;
};


