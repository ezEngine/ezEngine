#pragma once

#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiBehaviorGoToPOI : public ezAiBehavior
{
public:
  virtual ezStringView GetName() const override { return "GoToPOI"; }

  ezAiBehaviorGoToPOI();
  ~ezAiBehaviorGoToPOI();

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;

private:
  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiBehaviorWander : public ezAiBehavior
{
public:
  virtual ezStringView GetName() const override { return "Wander"; }

  ezAiBehaviorWander();
  ~ezAiBehaviorWander();

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}

private:
  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiBehaviorGoToCheckpoint : public ezAiBehavior
{
public:
  virtual ezStringView GetName() const override { return "GoToCheckpoint"; }

  ezAiBehaviorGoToCheckpoint();
  ~ezAiBehaviorGoToCheckpoint();

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}

private:
  ezVec3 m_vTargetPosition = ezVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiBehaviorShoot : public ezAiBehavior
{
public:
  virtual ezStringView GetName() const override { return "Shoot"; }

  ezAiBehaviorShoot();
  ~ezAiBehaviorShoot();

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezAiBehaviorQuip : public ezAiBehavior
{
public:
  virtual ezStringView GetName() const override { return "Quip"; }

  ezAiBehaviorQuip();
  ~ezAiBehaviorQuip();

  virtual void FlagNeededPerceptions(ezAiPerceptionManager& ref_PerceptionManager) override;

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override {}

  virtual ezTime GetCooldownDuration() override;
};
