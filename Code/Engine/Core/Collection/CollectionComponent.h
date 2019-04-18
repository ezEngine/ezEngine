#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Core/Collection/CollectionResource.h>

typedef ezComponentManager<class ezCollectionComponent, ezBlockStorageType::Compact> ezCollectionComponentManager;

/// \brief An ezCollectionComponent references an ezCollectionResource and triggers resource preloading when needed
///
/// Placing an ezCollectionComponent in a scene or a model makes it possible to tell the engine to preload certain resources
/// that are likely to be needed soon.
///
/// If a deactivated ezCollectionComponent is part of the scene, it will not trigger a preload, but will do so once
/// the component is activated.
class EZ_CORE_DLL ezCollectionComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezCollectionComponent, ezComponent, ezCollectionComponentManager);

public:
  ezCollectionComponent();
  ~ezCollectionComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent interface
public:

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:

  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCollectionComponent interface
public:

  void SetCollectionFile(const char* szFile);
  const char* GetCollectionFile() const;

  void SetCollection(const ezCollectionResourceHandle& hPrefab);
  EZ_ALWAYS_INLINE const ezCollectionResourceHandle& GetCollection() const { return m_hCollection; }

protected:

  /// Triggers the preload on the referenced ezCollectionResource
  void InitiatePreload();

  ezCollectionResourceHandle m_hCollection;
};

