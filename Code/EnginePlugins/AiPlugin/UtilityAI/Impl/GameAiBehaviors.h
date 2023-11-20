#pragma once

#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <Foundation/Math/Vec3.h>

// Behaviors of the 'Prototype' behavior set (see ezAiComponent). Test code, not all of them are in use.

/// Base for behaviors that walk to a position, which is stored in the instance data.
class EZ_AIPLUGIN_DLL ezAiBehaviorWithTargetPosition : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorWithTargetPosition, ezAiBehavior);

public:
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

protected:
  struct InstanceData
  {
    /// ezAiActionNavigateTo reads this through a pointer every frame, so updating it changes the destination.
    ezVec3 m_vTargetPosition = ezVec3::MakeZero();
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Walks to the closest point of interest (default: Command, 0.5). Follows it, if it moves.
class EZ_AIPLUGIN_DLL ezAiBehaviorGoToPOI : public ezAiBehaviorWithTargetPosition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorGoToPOI, ezAiBehaviorWithTargetPosition);

public:
  virtual ezStringView GetName() const override { return "GoToPOI"; }

  ezAiBehaviorGoToPOI();
  ~ezAiBehaviorGoToPOI();

  /// Generators of ezAiPerceptionPOI.
  ezAiInputPin m_POIs; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
  virtual void OnReactivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Walks to a random ezAiPerceptionWander position (default: ActiveIdle, 0.1). Currently not used.
class EZ_AIPLUGIN_DLL ezAiBehaviorWander : public ezAiBehaviorWithTargetPosition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorWander, ezAiBehaviorWithTargetPosition);

public:
  virtual ezStringView GetName() const override { return "Wander"; }

  ezAiBehaviorWander();
  ~ezAiBehaviorWander();

  /// Generators of ezAiPerceptionWander.
  ezAiInputPin m_WanderPoints; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Walks to a random checkpoint (default: ActiveIdle, 0.2).
class EZ_AIPLUGIN_DLL ezAiBehaviorGoToCheckpoint : public ezAiBehaviorWithTargetPosition
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorGoToCheckpoint, ezAiBehaviorWithTargetPosition);

public:
  virtual ezStringView GetName() const override { return "GoToCheckpoint"; }

  ezAiBehaviorGoToCheckpoint();
  ~ezAiBehaviorGoToCheckpoint();

  /// Generators of ezAiPerceptionCheckpoint.
  ezAiInputPin m_Checkpoints; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Turns towards a point of interest 2 to 5m away, aims and shoots, using the 'Aim' and 'Shoot' blackboard entries (default: Combat, 0.1). Currently not used.
class EZ_AIPLUGIN_DLL ezAiBehaviorShoot : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorShoot, ezAiBehavior);

public:
  virtual ezStringView GetName() const override { return "Shoot"; }

  ezAiBehaviorShoot();
  ~ezAiBehaviorShoot();

  /// Generators of ezAiPerceptionPOI.
  ezAiInputPin m_POIs; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// Logs a message whenever any point of interest is detected (default: Command, 0.6). 5 seconds cooldown.
class EZ_AIPLUGIN_DLL ezAiBehaviorQuip : public ezAiBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiBehaviorQuip, ezAiBehavior);

public:
  virtual ezStringView GetName() const override { return "Quip"; }

  ezAiBehaviorQuip();
  ~ezAiBehaviorQuip();

  /// Generators of ezAiPerceptionPOI.
  ezAiInputPin m_POIs; // [ property ]

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) const override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(const ezAiContext& ctx, void* pInstanceData) const override;
  virtual void OnActivateBehavior(const ezAiContext& ctx, void* pInstanceData, const ezAiPerception* pPerception) const override;

  virtual ezTime GetCooldownDuration(const void* pInstanceData) const override;
};
