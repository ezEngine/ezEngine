#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

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
class ezJoltClothSheetComponent;
class ezJoltBreakableSlabComponent;

class ezJoltUserData
{
public:
  EZ_DECLARE_POD_TYPE();

  enum class Type
  {
    Invalid,
    DynamicActorComponent,
    StaticActorComponent,
    TriggerComponent,
    CharacterComponent,
    ShapeComponent,
    BreakableSlabComponent,
    QueryShapeActorComponent,
    RagdollComponent,
    RopeComponent,
    ClothSheetComponent,
  };

  ezJoltUserData() = default;
  ~ezJoltUserData() = default;

  EZ_ALWAYS_INLINE void Init(ezJoltDynamicActorComponent* pObject, ezBitflags<ezOnJoltContact> contactFlags)
  {
    m_Type = Type::DynamicActorComponent;
    m_pObject = pObject;
    m_OnContact = contactFlags;
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

  EZ_ALWAYS_INLINE void Init(ezJoltClothSheetComponent* pObject)
  {
    m_Type = Type::ClothSheetComponent;
    m_pObject = pObject;
  }

  EZ_ALWAYS_INLINE void Init(ezJoltBreakableSlabComponent* pObject, ezBitflags<ezOnJoltContact> contactFlags)
  {
    m_Type = Type::BreakableSlabComponent;
    m_pObject = pObject;
    m_OnContact = contactFlags;
  }

  EZ_FORCE_INLINE void Invalidate()
  {
    m_Type = Type::Invalid;
    m_pObject = nullptr;
  }

  EZ_FORCE_INLINE static Type GetType(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr)
      return Type::Invalid;

    return pJoltUserData->m_Type;
  }

  EZ_FORCE_INLINE void* GetObject() const
  {
    return m_pObject;
  }

  EZ_FORCE_INLINE static ezComponent* GetComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr || pJoltUserData->m_Type == Type::Invalid)
    {
      return nullptr;
    }

    return static_cast<ezComponent*>(pJoltUserData->m_pObject);
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

  EZ_FORCE_INLINE static ezJoltTriggerComponent* GetTriggerComponent(const void* pUserData)
  {
    const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::TriggerComponent)
    {
      return static_cast<ezJoltTriggerComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  EZ_FORCE_INLINE static ezBitflags<ezOnJoltContact> GetContactFlags(const void* pUserData)
  {
    if (const ezJoltUserData* pJoltUserData = static_cast<const ezJoltUserData*>(pUserData))
    {
      return pJoltUserData->m_OnContact;
    }

    return ezOnJoltContact::None;
  }

  EZ_FORCE_INLINE ezBitflags<ezOnJoltContact> GetContactFlags() const
  {
    return m_OnContact;
  }

private:
  Type m_Type = Type::Invalid;
  void* m_pObject = nullptr;
  ezBitflags<ezOnJoltContact> m_OnContact;
};
