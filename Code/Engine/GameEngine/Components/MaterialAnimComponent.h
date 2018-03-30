#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/Resources/PropertyAnimResource.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Material/MaterialResource.h>

typedef ezComponentManagerSimple<class ezMaterialAnimComponent, ezComponentUpdateType::WhenSimulating> ezMaterialAnimComponentManager;

class EZ_GAMEENGINE_DLL ezMaterialAnimComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezMaterialAnimComponent, ezComponent, ezMaterialAnimComponentManager);

public:
  ezMaterialAnimComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetPropertyAnimFile(const char* szFile);
  const char* GetPropertyAnimFile() const;

  void SetPropertyAnim(const ezPropertyAnimResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezPropertyAnimResourceHandle& GetPropertyAnim() const { return m_hPropertyAnim; }

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  void SetMaterial(const ezMaterialResourceHandle& hResource);
  EZ_ALWAYS_INLINE const ezMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }

protected:

  ezTime m_CurAnimTime;

  ezMaterialResourceHandle m_hMaterial;
  ezPropertyAnimResourceHandle m_hPropertyAnim;
};

