#pragma once

#include <Foundation/Types/RangeView.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <Foundation/Types/SharedPtr.h>

typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReactionFactory_Prefab : public ezParticleEventReactionFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReactionFactory_Prefab, ezParticleEventReactionFactory);

public:
  ezParticleEventReactionFactory_Prefab();

  virtual const ezRTTI* GetEventReactionType() const override;
  virtual void CopyReactionProperties(ezParticleEventReaction* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sPrefab;
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  //const ezRangeView<const char*, ezUInt32> GetParameters() const;
  //void SetParameter(const char* szKey, const ezVariant& value);
  //void RemoveParameter(const char* szKey);
  //bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  //ezSharedPtr<ezParticlePrefabParameters> m_Parameters;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction_Prefab : public ezParticleEventReaction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction_Prefab, ezParticleEventReaction);

public:
  ezParticleEventReaction_Prefab();
  ~ezParticleEventReaction_Prefab();

  ezPrefabResourceHandle m_hPrefab;
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;

  //ezSharedPtr<ezParticlePrefabParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const ezParticleEvent& e) override;
};
