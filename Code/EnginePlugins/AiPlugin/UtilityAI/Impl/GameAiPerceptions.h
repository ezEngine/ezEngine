#pragma once

#include <AiPlugin/UtilityAI/Framework/AiPerception.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>

/// Point of interest: the position of an object that a sensor detected (see ezAiPerceptionGenPOI). Prototype behavior set.
class EZ_AIPLUGIN_DLL ezAiPerceptionPOI : public ezAiPerception
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionPOI, ezAiPerception);

public:
  ezAiPerceptionPOI() = default;
  ~ezAiPerceptionPOI() = default;

  EZ_AI_PERCEPTION_CLONE(ezAiPerceptionPOI);

  virtual bool GetDebugPosition(ezVec3& out_vPosition) const override
  {
    out_vPosition = m_vGlobalPosition;
    return true;
  }

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};


/// A position near the NPC to wander to. Prototype behavior set.
class EZ_AIPLUGIN_DLL ezAiPerceptionWander : public ezAiPerception
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionWander, ezAiPerception);

public:
  ezAiPerceptionWander() = default;
  ~ezAiPerceptionWander() = default;

  EZ_AI_PERCEPTION_CLONE(ezAiPerceptionWander);

  virtual bool GetDebugPosition(ezVec3& out_vPosition) const override
  {
    out_vPosition = m_vGlobalPosition;
    return true;
  }

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};

/// Position of an object in the 'Checkpoint' spatial category. Prototype behavior set.
class EZ_AIPLUGIN_DLL ezAiPerceptionCheckpoint : public ezAiPerception
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionCheckpoint, ezAiPerception);

public:
  ezAiPerceptionCheckpoint() = default;
  ~ezAiPerceptionCheckpoint() = default;

  EZ_AI_PERCEPTION_CLONE(ezAiPerceptionCheckpoint);

  virtual bool GetDebugPosition(ezVec3& out_vPosition) const override
  {
    out_vPosition = m_vGlobalPosition;
    return true;
  }

  ezVec3 m_vGlobalPosition = ezVec3::MakeZero();
};

/// A (potentially hostile) object that was seen or is remembered. See ezAiPerceptionGenTarget.
class EZ_AIPLUGIN_DLL ezAiPerceptionTarget : public ezAiPerception
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionTarget, ezAiPerception);

public:
  ezAiPerceptionTarget() = default;
  ~ezAiPerceptionTarget() = default;

  EZ_AI_PERCEPTION_CLONE(ezAiPerceptionTarget);

  virtual bool GetDebugPosition(ezVec3& out_vPosition) const override
  {
    out_vPosition = m_vGlobalPosition;
    return true;
  }

  virtual void GetDebugDesc(ezStringBuilder& out_sText) const override;

  /// For scripts, which get candidates as ezAiPerception. Returns null if pPerception is null or not a target. Not const, because reflected functions can't return const pointers.
  static ezAiPerceptionTarget* Cast(const ezAiPerception* pPerception);

  // script accessors
  ezGameObjectHandle GetTarget() const { return m_hTarget; }
  ezVec3 GetGlobalPosition() const { return m_vGlobalPosition; }
  float GetDistance() const { return m_fDistance; }
  bool IsVisible() const { return m_bVisible; }
  ezTime GetLastSeen() const { return m_LastSeen; }

  ezGameObjectHandle m_hTarget;
  ezVec3 m_vGlobalPosition = ezVec3::MakeZero(); ///< Current position if visible, otherwise the last known one.
  float m_fDistance = 0.0f;                      ///< Distance from the owner to m_vGlobalPosition at the time of the perception update.
  bool m_bVisible = false;                       ///< Whether a sensor currently detects the target.
  ezTime m_LastSeen;                             ///< Time of the last update in which a sensor detected the target. Together with m_hTarget it identifies the information, e.g. to know whether it was already investigated.
};

/// An ezAiMarkerComponent that a sensor detects right now, e.g. the noise of a shot. See ezAiPerceptionGenHint.
///
/// Not remembered: once the marker is gone, so is the perception. A behavior that reacts to it has to store what it needs.
class EZ_AIPLUGIN_DLL ezAiPerceptionHint : public ezAiPerception
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiPerceptionHint, ezAiPerception);

public:
  ezAiPerceptionHint() = default;
  ~ezAiPerceptionHint() = default;

  EZ_AI_PERCEPTION_CLONE(ezAiPerceptionHint);

  /// The source position, since that is where behaviors usually go.
  virtual bool GetDebugPosition(ezVec3& out_vPosition) const override
  {
    out_vPosition = m_vSourcePosition;
    return true;
  }

  virtual void GetDebugDesc(ezStringBuilder& out_sText) const override;

  /// For scripts, which get candidates as ezAiPerception. Returns null if pPerception is null or not a hint. Not const, because reflected functions can't return const pointers.
  static ezAiPerceptionHint* Cast(const ezAiPerception* pPerception);

  // script accessors
  ezGameObjectHandle GetMarker() const { return m_hMarker; }
  ezGameObjectHandle GetSource() const { return m_hSource; }
  ezVec3 GetGlobalPosition() const { return m_vGlobalPosition; }
  ezVec3 GetSourcePosition() const { return m_vSourcePosition; }
  float GetDistance() const { return m_fDistance; }
  float GetPriority() const { return m_fPriority; }

  ezGameObjectHandle m_hMarker;
  ezGameObjectHandle m_hSource;                  ///< See ezAiMarkerComponent::GetSourceObject().
  ezVec3 m_vGlobalPosition = ezVec3::MakeZero(); ///< Position of the marker.
  ezVec3 m_vSourcePosition = ezVec3::MakeZero(); ///< Position of the source, e.g. the shooter, while the marker may be at the impact.
  float m_fDistance = 0.0f;                      ///< Distance from the owner to m_vGlobalPosition.
  float m_fPriority = 0.0f;
};
