#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Module/ParticleModule.h>

class ezParticleEffectInstance;
class ezParticleEventReaction;

/// \brief Base class for all particle event reactions
class EZ_PARTICLEPLUGIN_DLL ezParticleEventReactionFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReactionFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetEventReactionType() const = 0;
  virtual void CopyReactionProperties(ezParticleEventReaction* pObject) const = 0;

  ezParticleEventReaction* CreateEventReaction(ezParticleEffectInstance* pOwner) const;

  virtual void Save(ezStreamWriter& stream) const;
  virtual void Load(ezStreamReader& stream);

  ezString m_sEventType;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction, ezReflectedClass);

  friend class ezParticleEventReactionFactory;
  friend class ezParticleEffectInstance;

protected:
  ezParticleEventReaction();
  ~ezParticleEventReaction();

  void Reset(ezParticleEffectInstance* pOwner);

  virtual void AfterPropertiesConfigured(bool bFirstTime);
  virtual void ProcessEventQueue(const ezParticleEventQueue* pQueue) = 0;

  ezTempHashedString m_sEventName;
  ezParticleEffectInstance* m_pOwnerEffect = nullptr;
};
