#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <Core/ResourceManager/ResourceHandle.h>

struct ezSrmManagerEvent;
struct ezMeshBufferResourceDescriptor;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;

//////////////////////////////////////////////////////////////////////////

typedef ezSettingsComponentManager<class ezSrmRenderComponent> ezSrmRenderComponentManager;

class EZ_WINDOWSMIXEDREALITY_DLL ezSrmRenderComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSrmRenderComponent, ezComponent, ezSrmRenderComponentManager);

public:
  ezSrmRenderComponent();
  ~ezSrmRenderComponent();

  //
  // ezComponent Interface
  //

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //
  // ezSrmRenderComponent Interface
  // 
public:

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  void SetMaterial(const ezMaterialResourceHandle& hMaterial);
  EZ_ALWAYS_INLINE const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }


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
  void CreateSurfaceRepresentation(const ezUuid& guid, SrmRenderObject& surface, const ezTransform& transform, const ezMeshBufferResourceDescriptor& mb);

  ezMaterialResourceHandle m_hMaterial;
  ezMap<ezUuid, SrmRenderObject> m_SrmRenderObjects;
};
