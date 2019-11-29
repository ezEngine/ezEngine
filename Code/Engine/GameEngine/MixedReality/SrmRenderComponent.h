#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <GameEngine/GameEngineDLL.h>

struct ezSrmManagerEvent;
struct ezMeshBufferResourceDescriptor;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

//////////////////////////////////////////////////////////////////////////

typedef ezSettingsComponentManager<class ezSrmRenderComponent> ezSrmRenderComponentManager;

class EZ_GAMEENGINE_DLL ezSrmRenderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSrmRenderComponent, ezComponent, ezSrmRenderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezSrmRenderComponent

public:
  ezSrmRenderComponent();
  ~ezSrmRenderComponent();

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  void SetMaterial(const ezMaterialResourceHandle& hMaterial);
  const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }


protected:
  struct SrmRenderObject
  {
    ezGameObjectHandle m_hGameObject;
    ezComponentHandle m_hMeshComponent;
    ezInt64 m_iLastUpdate = 0;
  };

  void SurfaceReconstructionManagerEventHandler(const ezSrmManagerEvent& e);
  void RemoveSrmRenderObject(const ezUuid& guid);
  void UpdateSurfaceRepresentation(const ezUuid& guid);
  void CreateSurfaceRepresentation(const ezUuid& guid, SrmRenderObject& surface, const ezTransform& transform, ezMeshBufferResourceDescriptor&& mb);

  ezMaterialResourceHandle m_hMaterial;
  ezMap<ezUuid, SrmRenderObject> m_SrmRenderObjects;
};
