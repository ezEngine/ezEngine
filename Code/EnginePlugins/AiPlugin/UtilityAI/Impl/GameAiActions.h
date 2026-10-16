#pragma once

#include <AiPlugin/UtilityAI/Framework/AiAction.h>
#include <Core/World/GameObject.h>

class EZ_AIPLUGIN_DLL ezAiActionWait : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionWait);

public:
  ezAiActionWait();
  ~ezAiActionWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezTime m_Duration;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionLerpRotation : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionLerpRotation);

public:
  ezAiActionLerpRotation();
  ~ezAiActionLerpRotation();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezVec3 m_vTurnAxis = ezVec3::MakeAxisZ();
  ezAngle m_TurnAngle;
  ezAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionLerpPosition : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionLerpPosition);

public:
  ezAiActionLerpPosition();
  ~ezAiActionLerpPosition();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  float m_fSpeed = 0.0f;
  ezVec3 m_vLocalSpaceSlide = ezVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionLerpRotationTowards : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionLerpRotationTowards);

public:
  ezAiActionLerpRotationTowards();
  ~ezAiActionLerpRotationTowards();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
  ezGameObjectHandle m_hTargetObject;
  ezAngle m_TargetReachedAngle = ezAngle::MakeFromDegree(5);
  ezAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// class EZ_AIPLUGIN_DLL ezAiActionFollowPath : public ezAiAction
//{
//   EZ_DECLARE_AICMD(ezAiActionFollowPath);
//
// public:
//   ezAiActionFollowPath();
//   ~ezAiActionFollowPath();
//
//   virtual void Reset() override;
//   virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
//   virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
//   virtual void Cancel(ezGameObject& owner) override;
//
//   ezGameObjectHandle m_hPath;
//   float m_fSpeed = 0.0f;
// };

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionBlackboardSetEntry : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionBlackboardSetEntry);

public:
  ezAiActionBlackboardSetEntry();
  ~ezAiActionBlackboardSetEntry();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  bool m_bNoCancel = false;
  ezHashedString m_sEntryName;
  ezVariant m_Value;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionBlackboardWait : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionBlackboardWait);

public:
  ezAiActionBlackboardWait();
  ~ezAiActionBlackboardWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezTempHashedString m_sEntryName;
  ezVariant m_Value;
  bool m_bEquals = true;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionBlackboardSetAndWait : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionBlackboardSetAndWait);

public:
  ezAiActionBlackboardSetAndWait();
  ~ezAiActionBlackboardSetAndWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezHashedString m_sEntryName;
  ezVariant m_SetValue;
  ezVariant m_WaitValue;
  bool m_bEqualsWaitValue = true;
  ezTime m_Timeout; ///< If non-zero, the action finishes after this time, even if the value wasn't reached.
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionCCMoveTo : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionCCMoveTo);

public:
  ezAiActionCCMoveTo();
  ~ezAiActionCCMoveTo();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
  ezGameObjectHandle m_hTargetObject;
  float m_fSpeed = 0.0f;
  float m_fReachedDistSQR = 1.0f;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionSpawn : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionSpawn);

public:
  ezAiActionSpawn();
  ~ezAiActionSpawn();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezTempHashedString m_sChildObjectName;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionQuip : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionQuip);

public:
  ezAiActionQuip();
  ~ezAiActionQuip();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezString m_sLogMsg;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiActionNavigateTo : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionNavigateTo);

public:
  ezAiActionNavigateTo();
  ~ezAiActionNavigateTo();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezVec3* m_pTargetPosition = nullptr; ///< If set, overrides m_vTargetPosition and is read every frame. Must stay valid as long as the action exists.
  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
  ezGameObjectHandle m_hTargetObject;  ///< If set, overrides the target position and follows the object.
  float m_fSpeed = 0.0f;
  float m_fReachedDist = 1.0f;
  bool m_bAllowPartialPath = false;

protected:
  ezAiActionResult ExecuteNavigation(ezGameObject& owner, const ezVec3& vTargetPos, ezLogInterface* pLog);

  bool m_bStarted = false;
  ezVec3 m_vLastDestination = ezVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Picks a random point on the navmesh around m_vCenter and navigates there.
///
/// Retries to find a point for a while, since that fails as long as the navmesh isn't available around the center.
class EZ_AIPLUGIN_DLL ezAiActionNavigateToRandomPoint : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionNavigateToRandomPoint);

public:
  ezAiActionNavigateToRandomPoint();
  ~ezAiActionNavigateToRandomPoint();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezVec3 m_vCenter = ezVec3::MakeZero();
  float m_fRadius = 10.0f;
  float m_fMaxDistToCenter = 0.0f; ///< If non-zero, points further away from the center are rejected.
  float m_fSpeed = 0.0f;
  float m_fReachedDist = 1.0f;
  bool m_bAllowPartialPath = false;

private:
  ezAiActionNavigateTo m_Navigate;
  bool m_bHasPoint = false;
  ezUInt32 m_uiAttempts = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Uses ezAiNavigationComponent::TurnTowards() until the owner faces the target.
class EZ_AIPLUGIN_DLL ezAiActionTurnTowards : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionTurnTowards);

public:
  ezAiActionTurnTowards();
  ~ezAiActionTurnTowards();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
  ezGameObjectHandle m_hTargetObject; ///< If set, overrides m_vTargetPosition.
  ezAngle m_MaxAngle = ezAngle::MakeFromDegree(20);
  ezTime m_Timeout = ezTime::Seconds(2);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Calls ezAiNavigationComponent::StopWalking().
class EZ_AIPLUGIN_DLL ezAiActionStopWalking : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionStopWalking);

public:
  ezAiActionStopWalking();
  ~ezAiActionStopWalking();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) override;
  virtual void Cancel(ezGameObject& owner) override;

  float m_fWithinDistance = 0.5f;
  bool m_bCanceled = false;
};
