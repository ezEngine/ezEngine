#pragma once

#include <PhysXPlugin/PhysXPluginDLL.h>

class ezComponent;
class ezPxDynamicActorComponent;
class ezPxStaticActorComponent;
class ezPxTriggerComponent;
class ezPxCharacterProxyComponent;
class ezPxShapeComponent;
class ezSurfaceResource;
class ezBreakableSheetComponent;

class ezPxUserData
{
public:

  EZ_ALWAYS_INLINE ezPxUserData()
    : m_Type(Invalid)
    , m_pObject(nullptr)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(nullptr_t)
    : m_Type(Invalid)
    , m_pObject(nullptr)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezPxDynamicActorComponent* pObject)
    : m_Type(DynamicActorComponent)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezPxStaticActorComponent* pObject)
    : m_Type(StaticActorComponent)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezPxTriggerComponent* pObject)
    : m_Type(TriggerComponent)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezPxCharacterProxyComponent* pObject)
    : m_Type(CharacterProxyComponent)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezPxShapeComponent* pObject)
    : m_Type(ShapeComponent)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezSurfaceResource* pObject)
    : m_Type(SurfaceResource)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezPxUserData(ezBreakableSheetComponent* pObject)
    : m_Type(BreakableSheet)
    , m_pObject(pObject)
    , m_pAdditionalUserData(nullptr)
  {
  }

  EZ_FORCE_INLINE static ezPxDynamicActorComponent* GetDynamicActorComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == DynamicActorComponent)
    {
      return static_cast<ezPxDynamicActorComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezPxStaticActorComponent* GetStaticActorComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == StaticActorComponent)
    {
      return static_cast<ezPxStaticActorComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezPxTriggerComponent* GetTriggerComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == TriggerComponent)
    {
      return static_cast<ezPxTriggerComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezPxCharacterProxyComponent* GetCharacterProxyComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == CharacterProxyComponent)
    {
      return static_cast<ezPxCharacterProxyComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezPxShapeComponent* GetShapeComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == ShapeComponent)
    {
      return static_cast<ezPxShapeComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezBreakableSheetComponent* GetBreakableSheetComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == BreakableSheet)
    {
      return static_cast<ezBreakableSheetComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezComponent* GetComponent(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr &&
        (pPxUserData->m_Type == DynamicActorComponent ||
         pPxUserData->m_Type == StaticActorComponent ||
         pPxUserData->m_Type == TriggerComponent ||
         pPxUserData->m_Type == CharacterProxyComponent ||
         pPxUserData->m_Type == ShapeComponent ||
         pPxUserData->m_Type == BreakableSheet
        )
      )
    {
      return static_cast<ezComponent*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezSurfaceResource* GetSurfaceResource(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    if (pPxUserData != nullptr && pPxUserData->m_Type == SurfaceResource)
    {
      return static_cast<ezSurfaceResource*>(pPxUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static void* GetAdditionalUserData(void* pUserData)
  {
    ezPxUserData* pPxUserData = static_cast<ezPxUserData*>(pUserData);
    return pPxUserData->m_pAdditionalUserData;
  }

  EZ_FORCE_INLINE void SetAdditionalUserData(void* pAdditionalUserData)
  {
    m_pAdditionalUserData = pAdditionalUserData;
  }


private:
  enum Type
  {
    Invalid,
    DynamicActorComponent,
    StaticActorComponent,
    TriggerComponent,
    CharacterProxyComponent,
    ShapeComponent,
    SurfaceResource,
    BreakableSheet
  };

  Type m_Type;
  void* m_pObject;
  void* m_pAdditionalUserData;
};
