#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

class ezSurfaceResource;
class ezComponent;
class ezJoltDynamicActorComponent;
class ezJoltStaticActorComponent;
class ezJoltTriggerComponent;
class ezJoltCharacterControllerComponent;
class ezJoltShapeComponent;
class ezJoltQueryShapeActorComponent;
class ezJoltRagdollComponent;
class ezJoltRopeComponent;
class ezJoltActorComponent;

class ezJoltUserData
{
public:
  EZ_DECLARE_POD_TYPE();

  ezJoltUserData() = default;
  ~ezJoltUserData() { Invalidate(); }

  EZ_ALWAYS_INLINE void Init(ezJoltDynamicActorComponent* pObject)
  {
    m_Type = Type::DynamicActorComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltStaticActorComponent* pObject)
  {
    m_Type = Type::StaticActorComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltTriggerComponent* pObject)
  {
    m_Type = Type::TriggerComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltCharacterControllerComponent* pObject)
  {
    m_Type = Type::CharacterComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltShapeComponent* pObject)
  {
    m_Type = Type::ShapeComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltQueryShapeActorComponent* pObject)
  {
    m_Type = Type::QueryShapeActorComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezSurfaceResource* pObject)
  {
    m_Type = Type::SurfaceResource;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltRagdollComponent* pObject)
  {
    m_Type = Type::RagdollComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltRopeComponent* pObject)
  {
    m_Type = Type::RopeComponent;
    m_pObject = pObject;
  }

  EZ_FORCE_INLINE void Invalidate()
  {
    m_Type = Type::Invalid;
    m_pObject = nullptr;
  }

  EZ_FORCE_INLINE static ezComponent* GetComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr ||
        pJoltUserData->m_Type == Type::Invalid ||
        pJoltUserData->m_Type == Type::SurfaceResource)
    {
      return nullptr;
    }

    return static_cast<ezComponent*>(pJoltUserData->m_pObject);
  }

  EZ_FORCE_INLINE static ezJoltActorComponent* GetActorComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr &&
        (pJoltUserData->m_Type == Type::DynamicActorComponent ||
          pJoltUserData->m_Type == Type::StaticActorComponent ||
          pJoltUserData->m_Type == Type::QueryShapeActorComponent))
    {
      return static_cast<ezJoltActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezJoltDynamicActorComponent* GetDynamicActorComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::DynamicActorComponent)
    {
      return static_cast<ezJoltDynamicActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezJoltShapeComponent* GetShapeComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::ShapeComponent)
    {
      return static_cast<ezJoltShapeComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezJoltTriggerComponent* GetTriggerComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::TriggerComponent)
    {
      return static_cast<ezJoltTriggerComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static const ezSurfaceResource* GetSurfaceResource(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::SurfaceResource)
    {
      return static_cast<const ezSurfaceResource*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

private:
  enum class Type
  {
    Invalid,
    DynamicActorComponent,
    StaticActorComponent,
    TriggerComponent,
    CharacterComponent,
    ShapeComponent,
    BreakableSheetComponent,
    SurfaceResource,
    QueryShapeActorComponent,
    RagdollComponent,
    RopeComponent,
  };

  Type m_Type = Type::Invalid;
  void* m_pObject = nullptr;
};
