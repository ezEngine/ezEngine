#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <AiPlugin/Navigation/Steering.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

EZ_DECLARE_FLAGS(ezUInt32, ezAiNavigationDebugFlags, PrintState, VisPathCorridor, VisPathLine, VisTarget);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiNavigationDebugFlags);

using ezAiNavigationComponentManager = ezComponentManagerSimple<class ezAiNavigationComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_AIPLUGIN_DLL ezAiNavigationComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiNavigationComponent, ezComponent, ezAiNavigationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  ezAiNavMeshPathTestComponent

public:
  ezAiNavigationComponent();
  ~ezAiNavigationComponent();

  enum State
  {
    Idle,
    Moving,
    Falling,
    Fallen,
    Failed,
  };

  void SetDestination(const ezVec3& vGlobalPos, bool bAllowPartialPath);
  void CancelNavigation();

  ezHashedString m_sNavmeshConfig;
  ezHashedString m_sPathSearchConfig;

  float m_fReachedDistance = 1.0f;
  float m_fSpeed = 5.0f;
  float m_fFootRadius = 0.15f;
  ezUInt32 m_uiCollisionLayer = 0;
  float m_fFallHeight = 0.7f;

  /// What aspects of the navigation to visualize.
  ezBitflags<ezAiNavigationDebugFlags> m_DebugFlags; // [ property ]

  State GetState() const { return m_State; }

protected:
  void Update();
  void Steer(ezTransform& transform);
  void PlaceOnGround(ezTransform& transform);

  State m_State = State::Idle;
  ezAiSteering m_Steering;
  ezAiNavigation m_Navigation;
  float m_fFallSpeed = 0.0f;
  bool m_bAllowPartialPath = false;

private:
  const char* DummyGetter() const { return nullptr; }
};
