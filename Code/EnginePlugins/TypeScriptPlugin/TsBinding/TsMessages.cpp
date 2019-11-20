#include <TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void ezTypeScriptBinding::GenerateMessagesFile(const char* szFile)
{
  ezStringBuilder sFileContent;

  sFileContent =
    R"(// AUTO-GENERATED FILE

import __Message = require("./Message")
export import Message = __Message.Message;

import __Vec2 = require("./Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("./Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("./Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

import __Transform = require("./Transform")
export import Transform = __Transform.Transform;

import __Color = require("./Color")
export import Color = __Color.Color;

import __Time = require("./Time")
export import Time = __Time.Time;

import __Angle = require("./Angle")
export import Angle = __Angle.Angle;

)";

  GenerateAllMessagesCode(sFileContent);

  ezFileWriter file;
  if (file.Open(szFile).Failed())
  {
    ezLog::Error("Failed to open file '{}'", szFile);
    return;
  }

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount());
}

static void CreateMessageTypeList(ezSet<const ezRTTI*>& found, ezDynamicArray<const ezRTTI*>& sorted, const ezRTTI* pRtti)
{
  if (found.Contains(pRtti))
    return;

  if (!pRtti->IsDerivedFrom<ezMessage>())
    return;

  if (pRtti == ezGetStaticRTTI<ezMessage>())
    return;

  found.Insert(pRtti);
  CreateMessageTypeList(found, sorted, pRtti->GetParentType());

  sorted.PushBack(pRtti);
}

void ezTypeScriptBinding::GenerateAllMessagesCode(ezStringBuilder& out_Code)
{
  ezSet<const ezRTTI*> found;
  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(100);

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    CreateMessageTypeList(found, sorted, pRtti);
  }

  for (auto pRtti : sorted)
  {
    GenerateMessageCode(out_Code, pRtti);
  }
}

void ezTypeScriptBinding::GenerateMessageCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sType, sParentType;
  GetTsName(pRtti, sType);

  GetTsName(pRtti->GetParentType(), sParentType);

  out_Code.AppendFormat("export class {0} extends {1}\n", sType, sParentType);
  out_Code.Append("{\n");
  out_Code.AppendFormat("  public static GetTypeNameHash(): number { return {}; }\n", pRtti->GetTypeNameHash());
  out_Code.AppendFormat("  constructor() { super(); this.TypeNameHash = {}; }\n", pRtti->GetTypeNameHash());
  GenerateMessagePropertiesCode(out_Code, pRtti);
  out_Code.Append("}\n\n");
}

void ezTypeScriptBinding::GenerateMessagePropertiesCode(ezStringBuilder& out_Code, const ezRTTI* pRtti)
{
  ezStringBuilder sProp;
  ezStringBuilder sDefault;

  for (ezAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const char* szTypeName = TsType(pMember->GetSpecificType());
    if (szTypeName == nullptr)
      continue;

    const ezVariant def = ezReflectionUtils::GetDefaultValue(pMember);

    if (def.CanConvertTo<ezString>())
    {
      sDefault = def.ConvertTo<ezString>();
    }

    if (!sDefault.IsEmpty())
    {
      // TODO: make this prettier
      if (def.GetType() == ezVariant::Type::Color)
      {
        ezColor c = def.Get<ezColor>();
        sDefault.Format("new Color({}, {}, {}, {})", c.r, c.g, c.b, c.a);
      }

      sProp.Format("  {0}: {1} = {2};\n", pMember->GetPropertyName(), szTypeName, sDefault);
    }
    else
    {
      sProp.Format("  {0}: {1};\n", pMember->GetPropertyName(), szTypeName);
    }

    out_Code.Append(sProp.GetView());
  }
}

void ezTypeScriptBinding::InjectMessageImportExport(const char* szFile, const char* szMessageFile)
{
  ezSet<const ezRTTI*> found;
  ezDynamicArray<const ezRTTI*> sorted;
  sorted.Reserve(100);

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    CreateMessageTypeList(found, sorted, pRtti);
  }

  ezStringBuilder sImportExport, sTypeName;

  sImportExport.Format(R"(

// AUTO-GENERATED
import __AllMessages = require("{}")
)",
    szMessageFile);

  for (const ezRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0}  = __AllMessages.{0};\n",
      sTypeName);
  }


  ezStringBuilder sFinal;

  {
    ezFileReader fileIn;
    fileIn.Open(szFile);

    sFinal.ReadAll(fileIn);

    sFinal.Append("\n\n");
    sFinal.Append(sImportExport.GetView());
    sFinal.Append("\n");
  }

  {
    ezFileWriter fileOut;
    fileOut.Open(szFile);
    fileOut.WriteBytes(sFinal.GetData(), sFinal.GetElementCount());
  }
}

static ezUniquePtr<ezMessage> CreateMessage(ezUInt32 uiTypeHash, const ezRTTI*& pRtti)
{
  static ezHashTable<ezUInt32, const ezRTTI*, ezHashHelper<ezUInt32>, ezStaticAllocatorWrapper> MessageTypes;

  if (!MessageTypes.TryGetValue(uiTypeHash, pRtti))
  {
    MessageTypes[uiTypeHash] = nullptr;

    for (pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (pRtti->GetTypeNameHash() == uiTypeHash)
      {
        MessageTypes[uiTypeHash] = pRtti;
        break;
      }
    }
  }

  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  return pRtti->GetAllocator()->Allocate<ezMessage>();
}

ezUniquePtr<ezMessage> ezTypeScriptBinding::MessageFromParameter(duk_context* pDuk, ezInt32 iObjIdx, ezTime delay)
{
  ezDuktapeHelper duk(pDuk);

  ezUInt32 uiTypeNameHash = duk.GetUIntValue(iObjIdx);

  const ezRTTI* pRtti = nullptr;
  ezUniquePtr<ezMessage> pMsg = CreateMessage(uiTypeNameHash, pRtti);

  if (pMsg != nullptr)
  {
    ezHybridArray<ezAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);

    for (ezAbstractProperty* pProp : properties)
    {
      if (pProp->GetCategory() != ezPropertyCategory::Member)
        continue;

      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

      const ezVariant value = ezTypeScriptBinding::GetVariantProperty(duk, pProp->GetPropertyName(), iObjIdx + 1, pMember->GetSpecificType()->GetVariantType());
      ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
    }
  }
  else

  {
    ezUInt32 uiMsgStashIdx = 0xFFFFFFFF;

    m_StashedMsgDelivery.SetCount(c_uiMaxMsgStash);
    const ezTime tNow = m_pWorld->GetClock().GetAccumulatedTime();

    for (ezUInt32 i = 0; i < c_uiMaxMsgStash; ++i)
    {
      m_uiNextStashMsgIdx++;

      if (m_uiNextStashMsgIdx >= c_uiLastStashMsgIdx)
        m_uiNextStashMsgIdx = c_uiFirstStashMsgIdx;

      if (m_StashedMsgDelivery[m_uiNextStashMsgIdx - c_uiFirstStashMsgIdx] < tNow)
        goto found;
    }

    ezLog::Error("Too many posted messages with large delay (> {}). DukTape stash is full.", c_uiMaxMsgStash);
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, nullptr, 0);

  found:

    m_StashedMsgDelivery[m_uiNextStashMsgIdx - c_uiFirstStashMsgIdx] = tNow + delay + ezTime::Milliseconds(50);

    {
      duk_dup(duk, iObjIdx + 1);                       // [ object ]
      StoreReferenceInStash(duk, m_uiNextStashMsgIdx); // [ object ]
      duk.PopStack();                                  // [ ]
    }

    pMsg = ezGetStaticRTTI<ezMsgTypeScriptMsgProxy>()->GetAllocator()->Allocate<ezMsgTypeScriptMsgProxy>();
    ezMsgTypeScriptMsgProxy* pTypedMsg = static_cast<ezMsgTypeScriptMsgProxy*>(pMsg.Borrow());
    pTypedMsg->m_uiTypeNameHash = uiTypeNameHash;
    pTypedMsg->m_uiStashIndex = m_uiNextStashMsgIdx;
  }

  EZ_DUK_VERIFY_STACK(duk, 0);
  return pMsg;
}

void ezTypeScriptBinding::DukPutMessage(duk_context* pDuk, const ezMessage& msg)
{
  ezDuktapeHelper duk(pDuk);

  const ezRTTI* pRtti = msg.GetDynamicRTTI();
  ezStringBuilder sMsgName = pRtti->GetTypeName();
  sMsgName.TrimWordStart("ez");

  duk.PushGlobalObject();                           // [ global ]
  duk.PushLocalObject("__AllMessages");             // [ global __AllMessages ]
  duk_get_prop_string(duk, -1, sMsgName.GetData()); // [ global __AllMessages msgname ]
  duk_new(duk, 0);                                  // [ global __AllMessages msg ]
  duk_remove(duk, -2);                              // [ global msg ]
  duk_remove(duk, -2);                              // [ msg ]


  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const ezRTTI* pType = pMember->GetSpecificType();
    if (pType->GetVariantType() == ezVariant::Type::Invalid)
      continue;

    const ezVariant val = ezReflectionUtils::GetMemberPropertyValue(pMember, &msg);

    SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
  }

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void ezTypeScriptBinding::RegisterMessageHandlersForComponentType(const char* szComponent, const ezUuid& componentType)
{
  ezDuktapeHelper duk(m_Duk);

  m_CurrentTsMsgHandlerRegistrator = componentType;

  const ezStringBuilder sCompModule("__", szComponent);

  duk.PushGlobalObject();                           // [ global ]
  if (duk.PushLocalObject(sCompModule).Succeeded()) // [ global __CompModule ]
  {
    if (duk.PushLocalObject(szComponent).Succeeded()) // [ global __CompModule obj ]
    {
      if (duk.PrepareObjectFunctionCall("RegisterMessageHandlers").Succeeded()) // [ global __CompModule obj func ]
      {
        duk.CallPreparedFunction(); // [ global __CompModule obj result ]
        duk.PopStack();             // [ global __CompModule obj ]
      }

      duk.PopStack(); // [ global __CompModule ]
    }

    duk.PopStack(); // [ global ]
  }

  duk.PopStack(); // [ ]

  m_CurrentTsMsgHandlerRegistrator.SetInvalid();

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

int ezTypeScriptBinding::__CPP_Binding_RegisterMessageHandler(duk_context* pDuk)
{
  ezTypeScriptBinding* tsb = ezTypeScriptBinding::RetrieveBinding(pDuk);

  EZ_ASSERT_DEV(tsb->m_CurrentTsMsgHandlerRegistrator.IsValid(), "'ez.TypescriptComponent.RegisterMessageHandler' may only be called from 'static RegisterMessageHandlers()'");

  ezDuktapeFunction duk(pDuk);

  ezUInt32 uiMsgTypeHash = duk.GetUIntValue(0);
  const char* szMsgHandler = duk.GetStringValue(1);

  const ezRTTI* pMsgType = ezRTTI::FindTypeByNameHash(uiMsgTypeHash);

  // this happens for pure TypeScript messages
  //if (pMsgType == nullptr)
  //{
  //  ezLog::Error("Message with type name hash '{}' does not exist.", uiMsgTypeHash);
  //  return duk.ReturnVoid();
  //}

  auto& tsc = tsb->m_TsComponentTypes[tsb->m_CurrentTsMsgHandlerRegistrator];
  auto& mh = tsc.m_MessageHandlers.ExpandAndGetRef();

  mh.m_sHandlerFunc = szMsgHandler;
  mh.m_pMessageType = pMsgType;
  mh.m_uiMessageTypeNameHash = uiMsgTypeHash;

  EZ_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

bool ezTypeScriptBinding::HasMessageHandler(const TsComponentTypeInfo& typeInfo, const ezRTTI* pMsgRtti) const
{
  if (!typeInfo.IsValid())
    return false;

  for (auto& mh : typeInfo.Value().m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      return true;
    }
  }

  return false;
}

bool ezTypeScriptBinding::DeliverMessage(const TsComponentTypeInfo& typeInfo, ezTypeScriptComponent* pComponent, ezMessage& msg)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  if (tsc.m_MessageHandlers.IsEmpty())
    return false;

  const ezRTTI* pMsgRtti = msg.GetDynamicRTTI();

  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      ezDuktapeHelper duk(m_Duk);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        ezTypeScriptBinding::DukPutMessage(duk, msg); // [ comp func comp msg ]
        duk.PushCustom();                             // [ comp func comp msg ]
        duk.CallPreparedMethod();                     // [ comp result ]
        duk.PopStack(2);                              // [ ]

        EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
      }
      else
      {
        // TODO: better error handling

        ezLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, pMsgRtti->GetTypeName());

        // mh.m_pMessageType = nullptr;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        EZ_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
      }

      EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
    }
  }

  return false;
}

bool ezTypeScriptBinding::DeliverTsMessage(const TsComponentTypeInfo& typeInfo, ezTypeScriptComponent* pComponent, const ezMsgTypeScriptMsgProxy& msg)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_uiMessageTypeNameHash == msg.m_uiTypeNameHash)
    {
      ezDuktapeHelper duk(m_Duk);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        DukPushStashObject(duk, msg.m_uiStashIndex); // [ comp func comp msg ]
        duk.PushCustom();                            // [ comp func comp msg ]
        duk.CallPreparedMethod();                    // [ comp result ]
        duk.PopStack(2);                             // [ ]

        EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
      }
      else
      {
        // TODO: better error handling

        ezLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, msg.m_uiTypeNameHash);

        // mh.m_uiMessageTypeNameHash = 0;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        EZ_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
      }

      EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
    }
  }

  return false;
}
