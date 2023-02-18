#pragma once

#include <Core/Collection/CollectionResource.h>
#include <Core/CoreDLL.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent
public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezCollectionComponent
public:
  ezCollectionComponent();
  ~ezCollectionComponent();

  void SetCollectionFile(const char* szFile); // [ property ]
  const char* GetCollectionFile() const;      // [ property ]

  void SetCollection(const ezCollectionResourceHandle& hPrefab);
  EZ_ALWAYS_INLINE const ezCollectionResourceHandle& GetCollection() const { return m_hCollection; }

protected:
  /// \brief Triggers the preload on the referenced ezCollectionResource
  void InitiatePreload();

  ezCollectionResourceHandle m_hCollection;
};
