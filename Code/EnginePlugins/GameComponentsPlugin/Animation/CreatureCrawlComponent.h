#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/ComponentManager.h>

class ezPhysicsWorldModuleInterface;

using ezCreatureCrawlComponentManager = ezComponentManagerSimple<class ezCreatureCrawlComponent, ezComponentUpdateType::WhenSimulating>;

struct ezCreatureLeg
{
  ezHashedString m_sLegObject;
  ezUInt8 m_uiStepGroup = 0;

  ezVec3 m_vRestPositionRelative;
  ezGameObjectHandle m_hLegObject;
  ezVec3 m_vCurTargetPosAbs;
  float m_fMoveLegFactor;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMECOMPONENTS_DLL, ezCreatureLeg);

class EZ_GAMECOMPONENTS_DLL ezCreatureCrawlComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCreatureCrawlComponent, ezComponent, ezCreatureCrawlComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezCreatureCrawlComponent

protected:
  void Update();

  void OnSimulationStarted() override;

public:
  ezCreatureCrawlComponent();
  ~ezCreatureCrawlComponent();

  void SetBodyReference(const char* szReference); // [ property ]

  float m_fCastUp = 0.3f;
  float m_fCastDown = 1.0f;
  float m_fStepDistance = 0.4f;
  float m_fMinLegDistance = 0.5f;

protected:
  ezGameObjectHandle m_hBody;             // [ property ]
  ezHybridArray<ezCreatureLeg, 4> m_Legs; // [ property ]

  const ezPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  ezTime m_LastMove;
  ezQuat m_qBodyTilt = ezQuat::MakeIdentity();

private:
  const char* DummyGetter() const { return nullptr; }
};
