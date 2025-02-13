#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

//////////////////////////////////////////////////////////////////////////
// ezGameObject
//////////////////////////////////////////////////////////////////////////

void ezGameObject_TryGetComponentOfBaseType(asIScriptGeneric* pGen)
{
  ezGameObject* pObj = (ezGameObject*)pGen->GetObject();
  int typeId = pGen->GetArgTypeId(0);

  if (auto info = pGen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const ezRTTI* pRtti = static_cast<const ezRTTI*>(info->GetUserData(ezAsUserData::RttiPtr)))
    {
      ezComponent* pComponent;
      if (pObj->TryGetComponentOfBaseType(pRtti, pComponent))
      {
        ezComponent** ref = (ezComponent**)pGen->GetArgAddress(0);
        *ref = pComponent;

        pGen->SetReturnByte(1);
        return;
      }
    }
  }

  pGen->SetReturnByte(0);
}

void ezGameObject_CreateComponent(asIScriptGeneric* pGen)
{
  ezGameObject* pObj = (ezGameObject*)pGen->GetObject();
  ezWorld* pWorld = pObj->GetWorld();
  const int typeId = pGen->GetArgTypeId(0);

  if (auto info = pGen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const ezRTTI* pRtti = static_cast<const ezRTTI*>(info->GetUserData(ezAsUserData::RttiPtr)))
    {
      if (auto pCompMan = pWorld->GetOrCreateManagerForComponentType(pRtti))
      {
        ezComponentHandle hComp = pCompMan->CreateComponent(pObj);

        ezComponent* pComponent;
        if (pWorld->TryGetComponent(hComp, pComponent))
        {
          ezComponent** ref = (ezComponent**)pGen->GetArgAddress(0);
          *ref = pComponent;

          pGen->SetReturnByte(1);
          return;
        }
      }
    }
  }

  pGen->SetReturnByte(0);
}

void ezGameObjectHandle_Construct(void* pMemory)
{
  new (pMemory) ezGameObjectHandle();
}

void ezComponentHandle_Construct(void* pMemory)
{
  new (pMemory) ezComponentHandle();
}

void ezGameObject_SendAsMessage(asIScriptGeneric* pGen)
{
  ezGameObject* pObj = (ezGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  ezMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  pObj->SendMessage(msg);
}

void ezGameObject_SendAsMessageConst(asIScriptGeneric* pGen)
{
  const ezGameObject* pObj = (const ezGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  ezMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  pObj->SendMessage(msg);
}

void ezGameObject_SendAsMessageRecursive(asIScriptGeneric* pGen)
{
  ezGameObject* pObj = (ezGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  ezMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  pObj->SendMessageRecursive(msg);
}

void ezGameObject_SendAsMessageRecursiveConst(asIScriptGeneric* pGen)
{
  const ezGameObject* pObj = (const ezGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  ezMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  pObj->SendMessageRecursive(msg);
}

void ezGameObject_PostAsMessage(asIScriptGeneric* pGen)
{
  ezGameObject* pObj = (ezGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  ezTime delay = *((const ezTime*)pGen->GetArgObject(1));
  ezInt32 delivery = (ezInt32)pGen->GetArgDWord(2);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  EZ_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  ezMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostMessage(msg, delay, static_cast<ezObjectMsgQueueType::Enum>(delivery));
}

void ezGameObject_PostAsMessageRecursive(asIScriptGeneric* pGen)
{
  ezGameObject* pObj = (ezGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  ezTime delay = *((const ezTime*)pGen->GetArgObject(1));
  ezInt32 delivery = (ezInt32)pGen->GetArgDWord(2);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  EZ_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  ezMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostMessageRecursive(msg, delay, static_cast<ezObjectMsgQueueType::Enum>(delivery));
}

void ezAngelScriptEngineSingleton::Register_GameObject()
{
  Register_EnumType(ezGetStaticRTTI<ezObjectMsgQueueType>());
  Register_EnumType(ezGetStaticRTTI<ezTransformPreservation>());

  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezGameObjectHandle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezGameObjectHandle_Construct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObjectHandle", "void Invalidate()", asMETHOD(ezGameObjectHandle, Invalidate), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObjectHandle", "bool IsInvalidated() const", asMETHOD(ezGameObjectHandle, IsInvalidated), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObjectHandle", "bool opEquals(ezGameObjectHandle) const", asMETHODPR(ezGameObjectHandle, operator==, (ezGameObjectHandle) const, bool), asCALL_THISCALL));
  }

  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezComponentHandle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezComponentHandle_Construct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("ezComponentHandle", "void Invalidate()", asMETHOD(ezComponentHandle, Invalidate), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezComponentHandle", "bool IsInvalidated() const", asMETHOD(ezComponentHandle, IsInvalidated), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("ezComponentHandle", "bool opEquals(ezComponentHandle) const", asMETHODPR(ezComponentHandle, operator==, (ezComponentHandle) const, bool), asCALL_THISCALL));
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObjectHandle GetHandle() const", asMETHOD(ezGameObject, GetHandle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void MakeDynamic()", asMETHOD(ezGameObject, MakeDynamic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void MakeStatic()", asMETHOD(ezGameObject, MakeStatic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool IsDynamic() const", asMETHOD(ezGameObject, IsDynamic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool IsStatic() const", asMETHOD(ezGameObject, IsStatic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetActiveFlag(bool bActive)", asMETHOD(ezGameObject, SetActiveFlag), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool GetActiveFlag() const", asMETHOD(ezGameObject, GetActiveFlag), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool IsActive() const", asMETHOD(ezGameObject, IsActive), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetName(ezStringView sName)", asMETHODPR(ezGameObject, SetName, (ezStringView), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetName(const ezHashedString& in sName)", asMETHODPR(ezGameObject, SetName, (const ezHashedString&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezStringView GetName() const", asMETHOD(ezGameObject, GetName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool HasName(const ezTempHashedString& in sName) const", asMETHOD(ezGameObject, HasName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalKey(ezStringView sKey)", asMETHODPR(ezGameObject, SetGlobalKey, (ezStringView), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalKey(const ezHashedString& in sKey)", asMETHODPR(ezGameObject, SetGlobalKey, (const ezHashedString&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezStringView GetGlobalKey() const", asMETHOD(ezGameObject, GetGlobalKey), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetParent(const ezGameObjectHandle& in hParent, ezTransformPreservation preserve = ezTransformPreservation::PreserveGlobal)", asMETHOD(ezGameObject, SetParent), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ GetParent() const", asMETHODPR(ezGameObject, GetParent, () const, const ezGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ GetParent()", asMETHODPR(ezGameObject, GetParent, (), ezGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void AddChild(const ezGameObjectHandle& in hChild, ezTransformPreservation preserve = ezTransformPreservation::PreserveGlobal)", asMETHOD(ezGameObject, AddChild), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void DetachChild(const ezGameObjectHandle& in hChild, ezTransformPreservation preserve = ezTransformPreservation::PreserveGlobal)", asMETHOD(ezGameObject, DetachChild), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint32 GetChildCount()", asMETHOD(ezGameObject, GetChildCount), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void GetChildren()", asMETHOD(ezGameObject, GetChildren), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ FindChildByName(const ezTempHashedString& in sName, bool bRecursive = true)", asMETHOD(ezGameObject, FindChildByName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ FindChildByPath(ezStringView sPath)", asMETHOD(ezGameObject, FindChildByPath), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ SearchForChildByNameSequence(string)", asMETHOD(ezGameObject, SearchForChildByNameSequence), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezGameObject@ SearchForChildrenByNameSequence(string)", asMETHOD(ezGameObject, SearchForChildrenByNameSequence), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezWorld@ GetWorld()", asMETHODPR(ezGameObject, GetWorld, (), ezWorld*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "const ezWorld@ GetWorld() const", asMETHODPR(ezGameObject, GetWorld, () const, const ezWorld*), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalPosition(const ezVec3& in)", asMETHODPR(ezGameObject, SetLocalPosition, (ezVec3), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetLocalPosition() const", asMETHOD(ezGameObject, GetLocalPosition), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalRotation(const ezQuat& in)", asMETHODPR(ezGameObject, SetLocalRotation, (ezQuat), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezQuat GetLocalRotation() const", asMETHOD(ezGameObject, GetLocalRotation), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalScaling(const ezVec3& in)", asMETHODPR(ezGameObject, SetLocalScaling, (ezVec3), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetLocalScaling() const", asMETHOD(ezGameObject, GetLocalScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetLocalUniformScaling(float fScale)", asMETHODPR(ezGameObject, SetLocalUniformScaling, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "float GetLocalUniformScaling() const", asMETHOD(ezGameObject, GetLocalUniformScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezTransform GetLocalTransform() const", asMETHOD(ezGameObject, GetLocalTransform), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalPosition(const ezVec3& in)", asMETHODPR(ezGameObject, SetGlobalPosition, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalPosition() const", asMETHOD(ezGameObject, GetGlobalPosition), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalRotation(const ezQuat& in)", asMETHODPR(ezGameObject, SetGlobalRotation, (const ezQuat&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezQuat GetGlobalRotation() const", asMETHODPR(ezGameObject, GetGlobalRotation, () const, ezQuat), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalScaling(const ezVec3& in)", asMETHODPR(ezGameObject, SetGlobalScaling, (const ezVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalScaling() const", asMETHOD(ezGameObject, GetGlobalScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetGlobalTransform(const ezTransform& in)", asMETHODPR(ezGameObject, SetGlobalTransform, (const ezTransform&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezTransform GetGlobalTransform() const", asMETHOD(ezGameObject, GetGlobalTransform), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezTransform GetLastGlobalTransform() const", asMETHOD(ezGameObject, GetLastGlobalTransform), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalDirForwards() const", asMETHOD(ezGameObject, GetGlobalDirForwards), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalDirRight() const", asMETHOD(ezGameObject, GetGlobalDirRight), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetGlobalDirUp() const", asMETHOD(ezGameObject, GetGlobalDirUp), asCALL_THISCALL));

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetLinearVelocity() const", asMETHOD(ezGameObject, GetLinearVelocity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezVec3 GetAngularVelocity() const", asMETHOD(ezGameObject, GetAngularVelocity), asCALL_THISCALL));
#endif

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateGlobalTransform()", asMETHOD(ezGameObject, UpdateGlobalTransform), asCALL_THISCALL));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezBoundingBoxSphere GetLocalBounds() const", asMETHOD(ezGameObject, GetLocalBounds), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "ezBoundingBoxSphere GetGlobalBounds() const", asMETHOD(ezGameObject, GetGlobalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateLocalBounds()", asMETHOD(ezGameObject, UpdateLocalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateGlobalBounds()", asMETHOD(ezGameObject, UpdateGlobalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void UpdateGlobalTransformAndBounds()", asMETHOD(ezGameObject, UpdateGlobalTransformAndBounds), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool TryGetComponentOfBaseType(const ezRTTI@ pType, ezComponent@& out pComponent)", asMETHODPR(ezGameObject, TryGetComponentOfBaseType, (const ezRTTI*, ezComponent*&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool TryGetComponentOfBaseType(?& out pTypedComponent)", asFUNCTION(ezGameObject_TryGetComponentOfBaseType), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool CreateComponent(?& out pTypedComponent)", asFUNCTION(ezGameObject_CreateComponent), asCALL_GENERIC));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void GetComponents()", asMETHOD(ezGameObject, GetComponents), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint16 GetComponentVersion()", asMETHOD(ezGameObject, GetComponentVersion), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessage(ezMessage& inout)", asMETHODPR(ezGameObject, SendMessage, (ezMessage&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessage(ezMessage& inout) const", asMETHODPR(ezGameObject, SendMessage, (ezMessage&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessageRecursive(ezMessage& inout)", asMETHODPR(ezGameObject, SendMessageRecursive, (ezMessage&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendMessageRecursive(ezMessage& inout) const", asMETHODPR(ezGameObject, SendMessageRecursive, (ezMessage&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostMessage(const ezMessage& in, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezGameObject, PostMessage), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostMessageRecursive(const ezMessage& in, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezGameObject, PostMessageRecursive), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendEventMessage(ezMessage& inout, const ezComponent@ pSender)", asMETHODPR(ezGameObject, SendEventMessage, (ezMessage&, const ezComponent*), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool SendEventMessage(ezMessage& inout, const ezComponent@ pSender) const", asMETHODPR(ezGameObject, SendEventMessage, (ezMessage&, const ezComponent*) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostEventMessage(const ezMessage& in msg, const ezComponent@ pSender, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame) const", asMETHOD(ezGameObject, PostEventMessage), asCALL_THISCALL));

  // GetTags
  // SetTags
  // SetTag
  // RemoveTag
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "bool HasTag(const ezTempHashedString& in sTagName) const", asMETHOD(ezGameObject, HasTag), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint16 GetTeamID()", asMETHOD(ezGameObject, GetTeamID), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetTeamID(uint16 id)", asMETHOD(ezGameObject, SetTeamID), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "uint32 GetStableRandomSeed()", asMETHOD(ezGameObject, GetStableRandomSeed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SetStableRandomSeed(uint32 seed)", asMETHOD(ezGameObject, SetStableRandomSeed), asCALL_THISCALL));

  // GetVisibilityState

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SendMessage(ezAngelScriptMessage& inout)", asFUNCTION(ezGameObject_SendAsMessage), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SendMessage(ezAngelScriptMessage& inout) const", asFUNCTION(ezGameObject_SendAsMessageConst), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SendMessageRecursive(ezAngelScriptMessage& inout)", asFUNCTION(ezGameObject_SendAsMessageRecursive), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void SendMessageRecursive(ezAngelScriptMessage& inout) const", asFUNCTION(ezGameObject_SendAsMessageRecursiveConst), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostMessage(const ezAngelScriptMessage& in, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame)", asFUNCTION(ezGameObject_PostAsMessage), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezGameObject", "void PostMessageRecursive(const ezAngelScriptMessage& in, ezTime delay, ezObjectMsgQueueType delivery = ezObjectMsgQueueType::NextFrame)", asFUNCTION(ezGameObject_PostAsMessageRecursive), asCALL_GENERIC));
}

//////////////////////////////////////////////////////////////////////////
// ezWorld
//////////////////////////////////////////////////////////////////////////

static void ezGameObjectDesc_Construct(void* pMemory)
{
  new (pMemory) ezGameObjectDesc();
}

void ezWorld_TryGetComponent(asIScriptGeneric* pGen)
{
  ezWorld* pWorld = (ezWorld*)pGen->GetObject();
  ezComponentHandle* hComponent = (ezComponentHandle*)pGen->GetArgObject(0);
  int typeId = pGen->GetArgTypeId(1);

  if (auto info = pGen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const ezRTTI* pRtti = static_cast<const ezRTTI*>(info->GetUserData(ezAsUserData::RttiPtr)))
    {
      ezComponent* pComponent;
      if (pWorld->TryGetComponent(*hComponent, pComponent))
      {
        ezComponent** ref = (ezComponent**)pGen->GetArgAddress(1);
        *ref = pComponent;

        pGen->SetReturnByte((*ref)->GetDynamicRTTI()->IsDerivedFrom(pRtti) ? 1 : 0);
        return;
      }
    }
  }

  pGen->SetReturnByte(0);
}

void ezAngelScriptEngineSingleton::Register_World()
{
  {
    AS_CHECK(m_pEngine->RegisterObjectType("ezGameObjectDesc", sizeof(ezGameObjectDesc), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<ezGameObjectDesc>()));

    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "bool m_bActiveFlag", asOFFSET(ezGameObjectDesc, m_bActiveFlag)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "bool m_bDynamic", asOFFSET(ezGameObjectDesc, m_bDynamic)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "uint16 m_uiTeamID", asOFFSET(ezGameObjectDesc, m_uiTeamID)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezHashedString m_sName", asOFFSET(ezGameObjectDesc, m_sName)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezGameObjectHandle m_hParent", asOFFSET(ezGameObjectDesc, m_hParent)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezVec3 m_LocalPosition", asOFFSET(ezGameObjectDesc, m_LocalPosition)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezQuat m_LocalRotation", asOFFSET(ezGameObjectDesc, m_LocalRotation)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezVec3 m_LocalScaling", asOFFSET(ezGameObjectDesc, m_LocalScaling)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "float m_LocalUniformScaling", asOFFSET(ezGameObjectDesc, m_LocalUniformScaling)));
    // AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "ezTagSet m_Tags", asOFFSET(ezGameObjectDesc, m_Tags)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("ezGameObjectDesc", "uint32 m_uiStableRandomSeed", asOFFSET(ezGameObjectDesc, m_uiStableRandomSeed)));

    AS_CHECK(m_pEngine->RegisterObjectBehaviour("ezGameObjectDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ezGameObjectDesc_Construct), asCALL_CDECL_OBJFIRST));
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezStringView GetName()", asMETHOD(ezWorld, GetName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezGameObjectHandle CreateObject(const ezGameObjectDesc& in desc)", asMETHODPR(ezWorld, CreateObject, (const ezGameObjectDesc& desc), ezGameObjectHandle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezGameObjectHandle CreateObject(const ezGameObjectDesc& in desc, ezGameObject@& out object)", asMETHODPR(ezWorld, CreateObject, (const ezGameObjectDesc& desc, ezGameObject*&), ezGameObjectHandle), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void DeleteObjectDelayed(const ezGameObjectHandle& in hObject, bool bAlsoDeleteEmptyParents = true)", asMETHOD(ezWorld, DeleteObjectDelayed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool IsValidObject(const ezGameObjectHandle& in hObject)", asMETHOD(ezWorld, IsValidObject), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObject(const ezGameObjectHandle& in, ezGameObject@& out pObject)", asMETHODPR(ezWorld, TryGetObject, (const ezGameObjectHandle&, ezGameObject*&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObject(const ezGameObjectHandle& in, const ezGameObject@& out pObject) const", asMETHODPR(ezWorld, TryGetObject, (const ezGameObjectHandle&, const ezGameObject*&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObjectWithGlobalKey(const ezTempHashedString& in sGlobalKey, ezGameObject@& out pObject)", asMETHODPR(ezWorld, TryGetObjectWithGlobalKey, (const ezTempHashedString& sGlobalKey, ezGameObject*& out_pObject), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetObjectWithGlobalKey(const ezTempHashedString& in sGlobalKey, const ezGameObject@& out pObject)", asMETHODPR(ezWorld, TryGetObjectWithGlobalKey, (const ezTempHashedString& sGlobalKey, const ezGameObject*& out_pObject) const, bool), asCALL_THISCALL));

  // GetOrCreateModule

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool IsValidComponent(ezComponentHandle& in)", asMETHOD(ezWorld, IsValidComponent), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "bool TryGetComponent(const ezComponentHandle& in hComponent, ?& out component)", asFUNCTION(ezWorld_TryGetComponent), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void SendMessage(const ezGameObjectHandle& in hReceiverObject, ezMessage& inout msg)", asMETHODPR(ezWorld, SendMessage, (const ezGameObjectHandle&, ezMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void SendMessageRecursive(const ezGameObjectHandle& in hReceiverObject, ezMessage& inout msg)", asMETHODPR(ezWorld, SendMessageRecursive, (const ezGameObjectHandle&, ezMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void PostMessage(const ezGameObjectHandle& in hReceiverObject, const ezMessage& in msg, ezTime delay, ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHODPR(ezWorld, PostMessage, (const ezGameObjectHandle&, const ezMessage&, ezTime, ezObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void PostMessageRecursive(const ezGameObjectHandle& in hReceiverObject, const ezMessage& in msg, ezTime delay, ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHODPR(ezWorld, PostMessageRecursive, (const ezGameObjectHandle&, const ezMessage&, ezTime, ezObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void SendMessage(const ezComponentHandle& in hReceiverObject, ezMessage& inout msg)", asMETHODPR(ezWorld, SendMessage, (const ezComponentHandle&, ezMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "void PostMessage(const ezComponentHandle& in hReceiverObject, const ezMessage& in msg, ezTime delay, ezObjectMsgQueueType queueType = ezObjectMsgQueueType::NextFrame) const", asMETHODPR(ezWorld, PostMessage, (const ezComponentHandle&, const ezMessage&, ezTime, ezObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezClock@ GetClock()", asMETHODPR(ezWorld, GetClock, (), ezClock&), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "const ezClock@ GetClock() const", asMETHODPR(ezWorld, GetClock, () const, const ezClock&),
    asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("ezWorld", "ezRandom@ GetRandomNumberGenerator()", asMETHOD(ezWorld, GetRandomNumberGenerator), asCALL_THISCALL));
}
