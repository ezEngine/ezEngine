#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef ezTypedResourceHandle<class ezAnimationClipResource> ezAnimationClipResourceHandle;

typedef ezComponentManagerSimple<class ezSimpleAnimationComponent, ezComponentUpdateType::WhenSimulating> ezSimpleAnimationComponentManager;

class EZ_GAMEENGINE_DLL ezSimpleAnimationComponent : public ezMeshComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSimpleAnimationComponent, ezMeshComponent, ezSimpleAnimationComponentManager);

public:
  ezSimpleAnimationComponent();
  ~ezSimpleAnimationComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface
  //
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // 

  //////////////////////////////////////////////////////////////////////////
  // Properties
  //

  void SetAnimationClip(const ezAnimationClipResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezAnimationClipResourceHandle& GetAnimationClip() const { return m_hAnimationClip; }

  void SetAnimationClipFile(const char* szFile);
  const char* GetAnimationClipFile() const;


  void Update();

protected:
  ezAnimationClipResourceHandle m_hAnimationClip;

  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_BoneMatrices;

  ezTime m_AnimationTime;
};

