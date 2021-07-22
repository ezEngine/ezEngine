#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/RopeSimulator.h>
#include <GameEngineDLL.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezFakeRopeComponentManager : public ezComponentManager<class ezFakeRopeComponent, ezBlockStorageType::Compact>
{
public:
  ezFakeRopeComponentManager(ezWorld* pWorld);
  ~ezFakeRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezFakeRopeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezFakeRopeComponent, ezComponent, ezFakeRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezFakeRopeComponent

public:
  ezFakeRopeComponent();
  ~ezFakeRopeComponent();

  ezUInt16 m_uiPieces = 16; // [ property ]

  void SetAnchorAReference(const char* szReference); // [ property ]
  void SetAnchorBReference(const char* szReference); // [ property ]

  void SetAnchorA(ezGameObjectHandle hActor);
  void SetAnchorB(ezGameObjectHandle hActor);

  void SetLength(float val);
  float GetLength() const { return m_fLength; }

  void SetAttachToA(bool val);
  bool GetAttachToA() const;
  void SetAttachToB(bool val);
  bool GetAttachToB() const;

  float m_fLength = 0.0f;
  float m_fDamping = 0.1f;

private:
  void UpdatePoses(bool force);
  ezVec3 GetAnchorPosition(const ezGameObjectHandle& hTarget) const;
  void SetupSimulator();
  void SendCurrentPose();
  void UpdatePreview();

  ezGameObjectHandle m_hAnchorA;
  ezGameObjectHandle m_hAnchorB;

  ezVec3 m_vPreviewRefPos;

  ezRopeSimulator m_RopeSim;

private:
  const char* DummyGetter() const { return nullptr; }
};
