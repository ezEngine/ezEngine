#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/RopeSimulator.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMECOMPONENTS_DLL ezFakeRopeComponentManager : public ezComponentManager<class ezFakeRopeComponent, ezBlockStorageType::FreeList>
{
public:
  ezFakeRopeComponentManager(ezWorld* pWorld);
  ~ezFakeRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMECOMPONENTS_DLL ezFakeRopeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFakeRopeComponent, ezComponent, ezFakeRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezFakeRopeComponent

public:
  ezFakeRopeComponent();
  ~ezFakeRopeComponent();

  ezUInt16 m_uiPieces = 16; // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]
  void SetAnchor1(ezGameObjectHandle hActor);
  void SetAnchor2(ezGameObjectHandle hActor);

  void SetSlack(float fVal);
  float GetSlack() const { return m_fSlack; }

  void SetAttachToAnchor1(bool bVal);
  void SetAttachToAnchor2(bool bVal);
  bool GetAttachToAnchor1() const;
  bool GetAttachToAnchor2() const;

  float m_fSlack = 0.0f;
  float m_fDamping = 0.5f;

private:
  ezResult ConfigureRopeSimulator();
  void SendCurrentPose();
  void SendPreviewPose();
  void RuntimeUpdate();

  ezGameObjectHandle m_hAnchor1;
  ezGameObjectHandle m_hAnchor2;

  ezUInt32 m_uiPreviewHash = 0;

  // if the owner or the anchor object are flagged as 'dynamic', the rope must follow their movement
  // otherwise it can skip some update steps
  bool m_bIsDynamic = true;
  ezUInt8 m_uiCheckEquilibriumCounter = 0;
  ezUInt8 m_uiSleepCounter = 0;
  ezRopeSimulator m_RopeSim;
  float m_fWindInfluence = 0.0f;

private:
  const char* DummyGetter() const { return nullptr; }
};
