#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;
using ezAnimGraphResourceHandle = ezTypedResourceHandle<class ezAnimGraphResource>;

using ezAnimationControllerComponentManager = ezComponentManagerSimple<class ezAnimationControllerComponent, ezComponentUpdateType::WhenSimulating, ezBlockStorageType::FreeList>;

class EZ_GAMEENGINE_DLL ezAnimationControllerComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAnimationControllerComponent, ezComponent, ezAnimationControllerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAnimationControllerComponent

public:
  ezAnimationControllerComponent();
  ~ezAnimationControllerComponent();

  void SetAnimationControllerFile(const char* szFile); // [ property ]
  const char* GetAnimationControllerFile() const;      // [ property ]

protected:
  void Update();

  ezAnimGraphResourceHandle m_hAnimationController;
  ezAnimGraph m_AnimationGraph;
};
