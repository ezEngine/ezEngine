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

import __Vec3 = require("./Vec3")
export import Vec3 = __Vec3.Vec3;

import __Quat = require("./Quat")
export import Quat = __Quat.Quat;

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
      if (def.GetType() == ezVariantType::Color)
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

    ezStringBuilder sSrc;
    sSrc.ReadAll(fileIn);

    //if (const char* szAutoGen = sSrc.FindSubString("// AUTO-GENERATED"))
    //{
    //  sFinal.SetSubString_FromTo(sSrc.GetData(), szAutoGen);
    //  sFinal.Trim(" \t\n\r");
    //}
    //else
    {
      sFinal = sSrc;
      sFinal.Append("\n\n");
    }

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

ezUniquePtr<ezMessage> ezTypeScriptBinding::MessageFromParameter(duk_context* pDuk, ezInt32 iObjIdx)
{
  ezDuktapeHelper duk(pDuk, 0);

  ezUInt32 uiTypeNameHash = duk.GetUIntValue(iObjIdx);

  const ezRTTI* pRtti = nullptr;
  ezUniquePtr<ezMessage> pMsg = CreateMessage(uiTypeNameHash, pRtti);


  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const ezVariant::Type::Enum type = pMember->GetSpecificType()->GetVariantType();
    switch (type)
    {
      case ezVariant::Type::Invalid:
        break;

      case ezVariant::Type::Bool:
      {
        bool value = duk.GetBoolProperty(pMember->GetPropertyName(), false, iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::String:
      {
        ezString value = duk.GetStringProperty(pMember->GetPropertyName(), "", iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::StringView:
      {
        ezStringView value = duk.GetStringProperty(pMember->GetPropertyName(), "", iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Int8:
      case ezVariant::Type::Int16:
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
      {
        ezInt32 value = duk.GetIntProperty(pMember->GetPropertyName(), 0, iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::UInt8:
      case ezVariant::Type::UInt16:
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
      {
        ezUInt32 value = duk.GetUIntProperty(pMember->GetPropertyName(), 0, iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Float:
      {
        const float value = duk.GetFloatProperty(pMember->GetPropertyName(), 0, iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Double:
      {
        const double value = duk.GetNumberProperty(pMember->GetPropertyName(), 0, iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Vector3:
      {
        ezVec3 value = ezTypeScriptBinding::GetVec3Property(duk, pMember->GetPropertyName(), iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Quaternion:
      {
        ezQuat value = ezTypeScriptBinding::GetQuatProperty(duk, pMember->GetPropertyName(), iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Color:
      {
        ezColor value = ezTypeScriptBinding::GetColorProperty(duk, pMember->GetPropertyName(), iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::ColorGamma:
      {
        ezColorGammaUB value = ezTypeScriptBinding::GetColorProperty(duk, pMember->GetPropertyName(), iObjIdx + 1);
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Time:
      {
        const ezTime value = ezTime::Seconds(duk.GetNumberProperty(pMember->GetPropertyName(), 0, iObjIdx + 1));
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Angle:
      {
        const ezAngle value = ezAngle::Radian(duk.GetFloatProperty(pMember->GetPropertyName(), 0, iObjIdx + 1));
        ezReflectionUtils::SetMemberPropertyValue(pMember, pMsg.Borrow(), value);
        break;
      }

      case ezVariant::Type::Vector2:
      case ezVariant::Type::Matrix3:
      case ezVariant::Type::Matrix4:
      case ezVariant::Type::Uuid:
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  return pMsg;
}

void ezTypeScriptBinding::DukPutMessage(duk_context* pDuk, const ezMessage& msg)
{
  ezDuktapeHelper duk(pDuk, +1);

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
    if (pType->GetVariantType() == ezVariantType::Invalid)
      continue;

    const ezVariant val = ezReflectionUtils::GetMemberPropertyValue(pMember, &msg);

    const ezVariant::Type::Enum type = pMember->GetSpecificType()->GetVariantType();
    switch (type)
    {
      case ezVariant::Type::Bool:
      {
        duk.SetBoolProperty(pMember->GetPropertyName(), val.Get<bool>());
        break;
      }

      case ezVariant::Type::String:
      case ezVariant::Type::StringView:
      {
        duk.SetStringProperty(pMember->GetPropertyName(), val.Get<ezString>());
        break;
      }

      case ezVariant::Type::Int8:
      case ezVariant::Type::Int16:
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
      case ezVariant::Type::UInt8:
      case ezVariant::Type::UInt16:
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
      {
        duk.SetNumberProperty(pMember->GetPropertyName(), val.ConvertTo<double>());
        break;
      }

      case ezVariant::Type::Vector3:
      {
        const ezVec3 v = val.Get<ezVec3>();

        duk.PushLocalObject(pMember->GetPropertyName(), -1);
        duk.SetNumberProperty("x", v.x, -1);
        duk.SetNumberProperty("y", v.y, -1);
        duk.SetNumberProperty("z", v.z, -1);
        duk.PopStack();
        break;
      }

      case ezVariant::Type::Color:
      case ezVariant::Type::ColorGamma:
      {
        const ezColor c = val.ConvertTo<ezColor>();

        duk.PushLocalObject(pMember->GetPropertyName(), -1);
        duk.SetNumberProperty("r", c.r, -1);
        duk.SetNumberProperty("g", c.g, -1);
        duk.SetNumberProperty("b", c.b, -1);
        duk.SetNumberProperty("a", c.a, -1);
        duk.PopStack();
        break;
      }

      case ezVariant::Type::Time:
      {
        duk.SetNumberProperty(pMember->GetPropertyName(), val.Get<ezTime>().GetSeconds());
        break;
      }

      case ezVariant::Type::Angle:
      {
        duk.SetNumberProperty(pMember->GetPropertyName(), val.Get<ezAngle>().GetRadian());
        break;
      }

      case ezVariant::Type::Quaternion:
      case ezVariant::Type::Vector2:
      case ezVariant::Type::Matrix3:
      case ezVariant::Type::Matrix4:
      case ezVariant::Type::Uuid:
      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void ezTypeScriptBinding::RegisterMessageHandlersForComponentType(const char* szComponent)
{
  ezDuktapeHelper duk(m_Duk, 0);

  m_sCurrentTsMsgHandlerRegistrator = szComponent;

  duk.PushGlobalObject();                         // [ global ]
  if (duk.PushLocalObject(szComponent).Succeeded()) // [ global obj ]
  {
    if (duk.PrepareObjectFunctionCall("RegisterMessageHandlers").Succeeded()) // [ global obj func ]
    {
      duk.CallPreparedFunction(); // [ global obj result ]
      duk.PopStack();             // [ global obj ]
    }

    duk.PopStack(); // [ global ]
  }

  duk.PopStack(); // [ ]

  m_sCurrentTsMsgHandlerRegistrator.Clear();
}

int ezTypeScriptBinding::__CPP_Binding_RegisterMessageHandler(duk_context* pDuk)
{
  ezTypeScriptBinding* tsb = ezTypeScriptBinding::RetrieveBinding(pDuk);

  EZ_ASSERT_DEV(!tsb->m_sCurrentTsMsgHandlerRegistrator.IsEmpty(), "'ez.TypescriptComponent.RegisterMessageHandler' may only be called from 'static RegisterMessageHandlers()'");

  ezDuktapeFunction duk(pDuk, 0);

  ezUInt32 uiMsgTypeHash = duk.GetUIntValue(0);
  const char* szMsgHandler = duk.GetStringValue(1);

  const ezRTTI* pMsgType = ezRTTI::FindTypeByNameHash(uiMsgTypeHash);

  if (pMsgType == nullptr)
  {
    ezLog::Error("Message with type name hash '{}' does not exist.", uiMsgTypeHash);
    return duk.ReturnVoid();
  }

  auto& tsc = tsb->m_TsComponentTypes[tsb->m_sCurrentTsMsgHandlerRegistrator];
  auto& mh = tsc.m_MessageHandlers.ExpandAndGetRef();

  mh.m_sHandlerFunc = szMsgHandler;
  mh.m_pMessageType = pMsgType;

  return duk.ReturnVoid();
}

bool ezTypeScriptBinding::DeliverMessage(const TsComponentTypeInfo& typeInfo, ezTypeScriptComponent* pComponent, ezMessage& msg)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  if (tsc.m_MessageHandlers.IsEmpty())
    return false;

  const ezRTTI* pMsgRtti = msg.GetDynamicRTTI();

  // TODO: make this more efficient
  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      ezDuktapeHelper duk(m_Duk, 0);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        ezTypeScriptBinding::DukPutMessage(duk, msg); // [ comp func comp msg ]
        duk.PushCustom();                             // [ comp func comp msg ]
        duk.CallPreparedMethod();                     // [ comp result ]
        duk.PopStack(2);                              // [ ]

        return true;
      }
      else
      {
        // TODO: better error handling

        ezLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, pMsgRtti->GetTypeName());

        // mh.m_pMessageType = nullptr;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        return false;
      }


      return true;
    }
  }

  return false;
}
