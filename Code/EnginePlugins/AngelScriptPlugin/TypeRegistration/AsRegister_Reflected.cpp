#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>

static void CastToBase(asIScriptGeneric* pGen)
{
  int derivedTypeId = pGen->GetObjectTypeId();
  auto derivedTypeInfo = pGen->GetEngine()->GetTypeInfoById(derivedTypeId);
  const ezRTTI* pDerivedRtti = (const ezRTTI*)derivedTypeInfo->GetUserData(ezAsUserData::RttiPtr);

  const ezRTTI* pBaseRtti = (const ezRTTI*)pGen->GetAuxiliary();

  if (pDerivedRtti != nullptr && pBaseRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      pGen->SetReturnObject(pGen->GetObject());
      return;
    }
  }

  pGen->SetReturnObject(nullptr);
}

static void CastToDerived(asIScriptGeneric* pGen)
{
  int baseTypeId = pGen->GetObjectTypeId();
  auto baseTypeInfo = pGen->GetEngine()->GetTypeInfoById(baseTypeId);
  const ezRTTI* pBaseRtti = (const ezRTTI*)baseTypeInfo->GetUserData(ezAsUserData::RttiPtr);

  const ezRTTI* pDerivedRtti = (const ezRTTI*)pGen->GetAuxiliary();

  if (pBaseRtti != nullptr && pDerivedRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      pGen->SetReturnObject(pGen->GetObject());
      return;
    }
  }

  pGen->SetReturnObject(nullptr);
}

struct RefInstance
{
  ezUInt32 m_uiRefCount = 1;
  const ezRTTI* m_pRtti = nullptr;
};

static ezMutex s_RefCountMutex;
static ezMap<void*, RefInstance> s_RefCounts;

static void* ezRtti_Create(const ezRTTI* pRtti)
{
  auto inst = pRtti->GetAllocator()->Allocate<ezReflectedClass>();

  EZ_LOCK(s_RefCountMutex);
  auto& ref = s_RefCounts[inst.m_pInstance];
  ref.m_pRtti = pRtti;

  return inst.m_pInstance;
}

static void ezRtti_AddRef(void* pInstance)
{
  EZ_LOCK(s_RefCountMutex);
  ++s_RefCounts[pInstance].m_uiRefCount;
}

static void ezRtti_Release(void* pInstance)
{
  EZ_LOCK(s_RefCountMutex);
  auto it = s_RefCounts.Find(pInstance);
  RefInstance& ri = it.Value();
  if (--ri.m_uiRefCount == 0)
  {
    ri.m_pRtti->GetAllocator()->Deallocate(pInstance);
    s_RefCounts.Remove(it);
  }
}


void ezAngelScriptEngineSingleton::Register_ReflectedType(const ezRTTI* pBaseType, bool bCreatable)
{
  EZ_LOG_BLOCK("Register_ReflectedType", pBaseType->GetTypeName());

  // first register the type
  ezRTTI::ForEachDerivedType(pBaseType, [&](const ezRTTI* pRtti)
    {
      ezStringBuilder typeName = pRtti->GetTypeName();
      auto pTypeInfo = m_pEngine->GetTypeInfoByName(typeName);

      m_WhitelistedRefTypes.Insert(typeName);

      if (pTypeInfo == nullptr)
      {
        if (bCreatable)
        {
          const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF);
          AS_CHECK(typeId);
          pTypeInfo = m_pEngine->GetTypeInfoById(typeId);

          if (pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate())
          {
            const ezStringBuilder sFactoryOp(typeName, "@ f()");
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_FACTORY, sFactoryOp, asFUNCTION(ezRtti_Create), asCALL_CDECL_OBJLAST, (void*)pRtti);
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asFUNCTION(ezRtti_AddRef), asCALL_CDECL_OBJLAST, (void*)pRtti);
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asFUNCTION(ezRtti_Release), asCALL_CDECL_OBJLAST, (void*)pRtti);
          }
        }
        else
        {

          const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF | asOBJ_NOCOUNT);
          AS_CHECK(typeId);

          pTypeInfo = m_pEngine->GetTypeInfoById(typeId);
        }
      }

      pTypeInfo->SetUserData((void*)pRtti, ezAsUserData::RttiPtr);

      AddForbiddenType(typeName);

      RegisterTypeFunctions(typeName, pRtti, false);
      RegisterTypeProperties(typeName, pRtti, false);

      //
    },
    ezRTTI::ForEachOptions::None);

  // then register the type hierarchy
  ezRTTI::ForEachDerivedType(pBaseType, [&](const ezRTTI* pRtti)
    {
      if (pRtti == pBaseType)
        return;

      const ezStringBuilder typeName = pRtti->GetTypeName();

      const ezRTTI* pParentRtti = pRtti->GetParentType();

      ezStringBuilder parentName, castOp;

      while (pParentRtti)
      {
        parentName = pParentRtti->GetTypeName();
        castOp.Set(parentName, "@ opImplCast()");

        AS_CHECK(m_pEngine->RegisterObjectMethod(typeName, castOp, asFUNCTION(CastToBase), asCALL_GENERIC, (void*)pParentRtti));

        castOp.Set(typeName, "@ opCast()");
        AS_CHECK(m_pEngine->RegisterObjectMethod(parentName, castOp, asFUNCTION(CastToDerived), asCALL_GENERIC, (void*)pRtti));

        if (pParentRtti == pBaseType)
          break;

        pParentRtti = pParentRtti->GetParentType();
      }
      //
    },
    ezRTTI::ForEachOptions::None);
}
