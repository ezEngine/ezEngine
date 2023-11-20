#pragma once

#include <AiPlugin/UtilityAI/Framework/AiAction.h>
#include <Core/World/GameObject.h>

/// Waits for m_Duration. Canceling finishes it right away.
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

/// Rotates the owner by m_TurnAngle around m_vTurnAxis (global space), with m_TurnAnglesPerSec.
///
/// Sets the global rotation directly, bypassing physics and the navigation component.
/// A negative angle turns the other way. Finishes right away if m_TurnAnglesPerSec is zero.
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

/// Moves the owner by m_vLocalSpaceSlide (relative to its current rotation), with m_fSpeed meters per second.
///
/// Sets the global position directly, bypassing physics and the navigation component. Fails if m_fSpeed is zero.
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

/// Rotates the owner around the Z axis to face m_vTargetPosition or m_hTargetObject, with m_TurnAnglesPerSec.
///
/// Sets the global rotation directly. Finishes once the remaining angle is below m_TargetReachedAngle.
/// Fails if the target object doesn't exist anymore. Finishes right away if m_TurnAnglesPerSec is zero.
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

/// Sets a blackboard entry once and finishes.
///
/// The blackboard is searched with ezBlackboardComponent::FindBlackboard(), so without a blackboard component on the NPC (or its parents)
/// the world's blackboard is used.
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

  /// If true, the value is still set when the action gets canceled. Used at the end of a sequence to restore a value that an earlier action set.
  bool m_bNoCancel = false;
  ezHashedString m_sEntryName;
  ezVariant m_Value;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Waits until a blackboard entry equals m_Value (or doesn't, if m_bEquals is false). There is no timeout.
///
/// A missing entry counts as equal to m_Value.
/// The blackboard is searched with ezBlackboardComponent::FindBlackboard(), so without a blackboard component on the NPC (or its parents)
/// the world's blackboard is used.
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

/// Sets a blackboard entry to m_SetValue, then waits until it equals m_WaitValue (or doesn't, if m_bEqualsWaitValue is false).
///
/// Used to trigger something in another system (typically an animation graph) and wait until that system reports that it is done,
/// by changing the same entry. E.g. set 'Shoot' to 1 and wait until the animation graph resets it to 0.
/// The value is only set in the first Execute(). If the other system never changes the value, this waits forever, unless m_Timeout is set.
/// A missing entry counts as equal to m_WaitValue. Canceling finishes right away, without restoring the entry.
/// The blackboard is searched with ezBlackboardComponent::FindBlackboard(), so without a blackboard component on the NPC (or its parents)
/// the world's blackboard is used.
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

/// Moves the owner in a straight line towards a target by sending ezMsgMoveCharacterController every frame. No path finding.
///
/// Rotates the owner to face the target directly. Height differences are ignored.
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
  float m_fReachedDistSQR = 1.0f; ///< Squared distance (in 2D) at which the target counts as reached.
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Calls ezSpawnComponent::TriggerManualSpawn() on the child object with the given name.
///
/// Waits as long as the spawn component refuses to spawn (minimum spawn delay).
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

  ezTempHashedString m_sChildObjectName; ///< Searched recursively.
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Writes m_sLogMsg to the log. For testing.
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

/// Walks to a target position on the navmesh, using the ezAiNavigationComponent on the owner.
///
/// The target is m_hTargetObject if set, otherwise m_pTargetPosition if set, otherwise m_vTargetPosition.
/// For a moving target a new path is only searched once it moved more than 0.5m.
/// Finishes when the navigation component is idle again (target reached), fails if no path was found or the NPC fell down.
/// Canceling stops the navigation.
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
  float m_fSpeed = 0.0f;               ///< Walk speed in meters per second. Zero means canceled, the action finishes right away.
  float m_fReachedDist = 1.0f;         ///< Distance at which the navigation component considers the target reached.
  bool m_bAllowPartialPath = false;    ///< If true, walks as far as possible when the target can't be reached, instead of failing.

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
/// Gives up (fails) after 100 attempts (one per frame).
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
///
/// The navigation component only turns while it is idle, so this has no effect while the NPC walks (use ezAiActionStopWalking first).
/// Finishes when the target object doesn't exist anymore.
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
  ezAngle m_MaxAngle = ezAngle::MakeFromDegree(20); ///< Finishes once the angle to the target is below this.
  ezTime m_Timeout = ezTime::Seconds(2);            ///< Finishes (not fails) after this time, even if the NPC doesn't face the target.
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Calls ezAiNavigationComponent::StopWalking() and finishes right away, without waiting for the NPC to stop.
///
/// Unlike canceling the navigation, this doesn't stop the NPC abruptly.
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

  float m_fWithinDistance = 0.5f; ///< The NPC decelerates and comes to a stop within this distance.
  bool m_bCanceled = false;
};
