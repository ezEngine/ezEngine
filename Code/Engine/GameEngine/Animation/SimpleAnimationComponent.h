#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef ezComponentManagerSimple<class ezSimpleAnimationComponent, ezComponentUpdateType::WhenSimulating> ezSimpleAnimationComponentManager;

class EZ_GAMEENGINE_DLL ezSimpleAnimationComponent : public ezMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSimpleAnimationComponent, ezMeshComponent, ezSimpleAnimationComponentManager);

public:
  ezSimpleAnimationComponent();
  ~ezSimpleAnimationComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;

  void Update();

protected:
  ezAngle m_DegreePerSecond;

  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_BoneMatrices;

  ezAngle m_Rotation;
};

