#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/ResourceManager/Resource.h>
#include <Foundation/Reflection/Reflection.h>

typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;
typedef ezTypedResourceHandle<class ezPrefabResource> ezPrefabResourceHandle;


struct ezSurfaceInteractionAlignment
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezSurfaceInteractionAlignment);


struct EZ_GAMEENGINE_DLL ezSurfaceInteraction
{
  void SetPrefab(const char* szPrefab);
  const char* GetPrefab() const;

  ezString m_sInteractionType;

  ezPrefabResourceHandle m_hPrefab;
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;
  ezAngle m_Deviation;
  float m_fImpulseThreshold = 0.0f;
  float m_fImpulseScale = 1.0f;
};





EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezSurfaceInteraction);

struct EZ_GAMEENGINE_DLL ezSurfaceResourceDescriptor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceResourceDescriptor, ezReflectedClass);
public:

  void Load(ezStreamReader& stream);
  void Save(ezStreamWriter& stream) const;

  void SetBaseSurfaceFile(const char* szFile);
  const char* GetBaseSurfaceFile() const;

  void SetCollisionInteraction(const char* szFile);
  const char* GetCollisionInteraction() const;

  ezSurfaceResourceHandle m_hBaseSurface;
  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
  ezHashedString m_sOnCollideInteraction;

  ezHybridArray<ezSurfaceInteraction, 16> m_Interactions;
};

