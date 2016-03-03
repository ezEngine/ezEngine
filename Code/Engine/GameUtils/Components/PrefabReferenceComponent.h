#pragma once

#include <GameUtils/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameUtils/Prefabs/PrefabResource.h>

typedef ezComponentManager<class ezPrefabReferenceComponent> ezPrefabReferenceComponentManager;

class EZ_GAMEUTILS_DLL ezPrefabReferenceComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPrefabReferenceComponent, ezComponent, ezPrefabReferenceComponentManager);

public:
  ezPrefabReferenceComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetPrefabFile(const char* szFile);
  const char* GetPrefabFile() const;

  void SetPrefab(const ezPrefabResourceHandle& hPrefab);
  EZ_FORCE_INLINE const ezPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

protected:

  // ************************************* FUNCTIONS *****************************

  virtual Initialization Initialize() override;

private:
  ezPrefabResourceHandle m_hPrefab;
};
