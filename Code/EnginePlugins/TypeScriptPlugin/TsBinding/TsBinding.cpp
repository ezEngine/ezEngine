#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static ezHashTable<duk_context*, ezTypeScriptBinding*> s_DukToBinding;

ezSet<const ezRTTI*> ezTypeScriptBinding::s_RequiredEnums;
ezSet<const ezRTTI*> ezTypeScriptBinding::s_RequiredFlags;

static int __CPP_Time_Get(duk_context* pDuk)
{
  ezDuktapeFunction duk(pDuk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      return duk.ReturnFloat(ezTime::Now().AsFloatInSeconds());

    case 1:
    {
      ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);
      return duk.ReturnFloat(pWorld->GetClock().GetAccumulatedTime().AsFloatInSeconds());
    }

    case 2:
    {
      ezWorld* pWorld = ezTypeScriptBinding::RetrieveWorld(pDuk);
      return duk.ReturnFloat(pWorld->GetClock().GetTimeDiff().AsFloatInSeconds());
    }
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return duk.ReturnFloat(0.0f);
}

ezTypeScriptBinding::ezTypeScriptBinding()
  : m_Duk("Typescript Binding")
{
  s_DukToBinding[m_Duk] = this;
}

ezTypeScriptBinding::~ezTypeScriptBinding()
{
  if (!m_CVars.IsEmpty())
  {
    m_CVars.Clear();

    ezCVar::ListOfCVarsChanged("invalid");
  }

  s_DukToBinding.Remove(m_Duk);
}

ezResult ezTypeScriptBinding::Init_Time()
{
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetRealTime", __CPP_Time_Get, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetGameTime", __CPP_Time_Get, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetGameTimeDiff", __CPP_Time_Get, 0, 2);

  return EZ_SUCCESS;
}

ezTypeScriptBinding* ezTypeScriptBinding::RetrieveBinding(duk_context* pDuk)
{
  ezTypeScriptBinding* pBinding = nullptr;
  s_DukToBinding.TryGetValue(pDuk, pBinding);
  return pBinding;
}

ezResult ezTypeScriptBinding::Initialize(ezWorld& ref_world)
{
  if (m_bInitialized)
    return EZ_SUCCESS;

  EZ_LOG_BLOCK("Initialize TypeScript Binding");
  EZ_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_hScriptCompendium = ezResourceManager::LoadResource<ezScriptCompendiumResource>(":project/AssetCache/Common/Scripts.ezScriptCompendium");

  m_Duk.EnableModuleSupport(&ezTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("ezTypeScriptBinding", this);

  m_Duk.RegisterGlobalFunction("__CPP_Binding_RegisterMessageHandler", &ezTypeScriptBinding::__CPP_Binding_RegisterMessageHandler, 2);

  StoreWorld(&ref_world);

  SetupRttiFunctionBindings();
  SetupRttiPropertyBindings();

  EZ_SUCCEED_OR_RETURN(Init_RequireModules());
  EZ_SUCCEED_OR_RETURN(Init_Log());
  EZ_SUCCEED_OR_RETURN(Init_Utils());
  EZ_SUCCEED_OR_RETURN(Init_Time());
  EZ_SUCCEED_OR_RETURN(Init_GameObject());
  EZ_SUCCEED_OR_RETURN(Init_FunctionBinding());
  EZ_SUCCEED_OR_RETURN(Init_PropertyBinding());
  EZ_SUCCEED_OR_RETURN(Init_Component());
  EZ_SUCCEED_OR_RETURN(Init_World());
  EZ_SUCCEED_OR_RETURN(Init_Clock());
  EZ_SUCCEED_OR_RETURN(Init_Debug());
  EZ_SUCCEED_OR_RETURN(Init_Random());
  EZ_SUCCEED_OR_RETURN(Init_Physics());

  m_bInitialized = true;
  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::LoadComponent(const ezUuid& typeGuid, TsComponentTypeInfo& out_typeInfo)
{
  if (!m_bInitialized || !typeGuid.IsValid())
  {
    return EZ_FAILURE;
  }

  // check if this component type has been loaded before
  {
    auto itLoaded = m_LoadedComponents.Find(typeGuid);

    if (itLoaded.IsValid())
    {
      out_typeInfo = m_TsComponentTypes.Find(typeGuid);
      return itLoaded.Value() ? EZ_SUCCESS : EZ_FAILURE;
    }
  }

  EZ_PROFILE_SCOPE("Load Script Component");

  bool& bLoaded = m_LoadedComponents[typeGuid];
  bLoaded = false;

  ezResourceLock<ezScriptCompendiumResource> pCompendium(m_hScriptCompendium, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    return EZ_FAILURE;
  }

  auto itType = pCompendium->GetDescriptor().m_AssetGuidToInfo.Find(typeGuid);
  if (!itType.IsValid())
  {
    return EZ_FAILURE;
  }

  const ezString& sComponentPath = itType.Value().m_sComponentFilePath;
  const ezString& sComponentName = itType.Value().m_sComponentTypeName;

  const ezStringBuilder sCompModule("__", sComponentName);

  m_Duk.PushGlobalObject();

  ezStringBuilder req;
  req.Format("var {} = require(\"./{}\");", sCompModule, itType.Value().m_sComponentFilePath);
  if (m_Duk.ExecuteString(req).Failed())
  {
    ezLog::Error("Could not load component");
    return EZ_FAILURE;
  }

  m_Duk.PopStack();

  m_TsComponentTypes[typeGuid].m_sComponentTypeName = sComponentName;
  RegisterMessageHandlersForComponentType(sComponentName, typeGuid);

  bLoaded = true;

  out_typeInfo = m_TsComponentTypes.FindOrAdd(typeGuid, nullptr);

  return EZ_SUCCESS;
}

ezResult ezTypeScriptBinding::FindScriptComponentInfo(const char* szComponentType, TsComponentTypeInfo& out_typeInfo)
{
  for (auto it : m_TsComponentTypes)
  {
    if (it.Value().m_sComponentTypeName == szComponentType)
    {
      out_typeInfo = it;
      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezTypeScriptBinding::Update()
{
  ExecuteConsoleFuncs();
}

void ezTypeScriptBinding::CleanupStash(ezUInt32 uiNumIterations)
{
  if (!m_LastCleanupObj.IsValid())
    m_LastCleanupObj = m_GameObjectToStashIdx.GetIterator();

  ezDuktapeHelper duk(m_Duk); // [ ]
  duk.PushGlobalStash();      // [ stash ]

  for (ezUInt32 i = 0; i < uiNumIterations && m_LastCleanupObj.IsValid(); ++i)
  {
    ezGameObject* pGO;
    if (!m_pWorld->TryGetObject(m_LastCleanupObj.Key(), pGO))
    {
      duk.PushNull();                                        // [ stash null ]
      duk_put_prop_index(duk, -2, m_LastCleanupObj.Value()); // [ stash ]

      ReleaseStashObjIndex(m_LastCleanupObj.Value());
      m_LastCleanupObj = m_GameObjectToStashIdx.Remove(m_LastCleanupObj);
    }
    else
    {
      ++m_LastCleanupObj;
    }
  }

  if (!m_LastCleanupComp.IsValid())
    m_LastCleanupComp = m_ComponentToStashIdx.GetIterator();

  for (ezUInt32 i = 0; i < uiNumIterations && m_LastCleanupComp.IsValid(); ++i)
  {
    ezComponent* pGO;
    if (!m_pWorld->TryGetComponent(m_LastCleanupComp.Key(), pGO))
    {
      duk.PushNull();                                         // [ stash null ]
      duk_put_prop_index(duk, -2, m_LastCleanupComp.Value()); // [ stash ]

      ReleaseStashObjIndex(m_LastCleanupComp.Value());
      m_LastCleanupComp = m_ComponentToStashIdx.Remove(m_LastCleanupComp);
    }
    else
    {
      ++m_LastCleanupComp;
    }
  }

  duk.PopStack(); // [ ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

ezUInt32 ezTypeScriptBinding::AcquireStashObjIndex()
{
  ezUInt32 idx;

  if (!m_FreeStashObjIdx.IsEmpty())
  {
    idx = m_FreeStashObjIdx.PeekBack();
    m_FreeStashObjIdx.PopBack();
  }
  else
  {
    idx = m_uiNextStashObjIdx;
    ++m_uiNextStashObjIdx;
  }

  return idx;
}

void ezTypeScriptBinding::ReleaseStashObjIndex(ezUInt32 uiIdx)
{
  m_FreeStashObjIdx.PushBack(uiIdx);
}

void ezTypeScriptBinding::StoreReferenceInStash(duk_context* pDuk, ezUInt32 uiStashIdx)
{
  ezDuktapeHelper duk(pDuk);               // [ object ]
  duk.PushGlobalStash();                   // [ object stash ]
  duk_dup(duk, -2);                        // [ object stash object ]
  duk_put_prop_index(duk, -2, uiStashIdx); // [ object stash ]
  duk.PopStack();                          // [ object ]

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

bool ezTypeScriptBinding::DukPushStashObject(duk_context* pDuk, ezUInt32 uiStashIdx)
{
  ezDuktapeHelper duk(pDuk);

  duk.PushGlobalStash();          // [ stash ]
  duk_push_uint(duk, uiStashIdx); // [ stash idx ]

  if (!duk_get_prop(duk, -2)) // [ stash obj/undef ]
  {
    duk_pop_2(duk);     // [ ]
    duk_push_null(duk); // [ null ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, false, +1);
  }
  else // [ stash obj ]
  {
    duk_replace(duk, -2); // [ obj ]
    EZ_DUK_RETURN_AND_VERIFY_STACK(duk, true, +1);
  }
}

void ezTypeScriptBinding::SyncTsObjectEzTsObject(duk_context* pDuk, const ezRTTI* pRtti, void* pObject, ezInt32 iObjIdx)
{
  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const ezVariant value = ezTypeScriptBinding::GetVariantProperty(pDuk, pProp->GetPropertyName(), iObjIdx, pMember->GetSpecificType());

    if (value.IsValid())
    {
      ezReflectionUtils::SetMemberPropertyValue(pMember, pObject, value);
    }
  }
}

void ezTypeScriptBinding::SyncEzObjectToTsObject(duk_context* pDuk, const ezRTTI* pRtti, const void* pObject, ezInt32 iObjIdx)
{
  ezDuktapeHelper duk(pDuk);

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pProp);

    const ezRTTI* pType = pMember->GetSpecificType();

    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
    {
      const ezVariant val = ezReflectionUtils::GetMemberPropertyValue(pMember, pObject);

      SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
    }
    else
    {
      if (pType->GetVariantType() == ezVariant::Type::Invalid)
        continue;

      const ezVariant val = ezReflectionUtils::GetMemberPropertyValue(pMember, pObject);

      SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
    }
  }

  EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void ezTypeScriptBinding::GenerateConstructorString(ezStringBuilder& out_String, const ezVariant& value)
{
  out_String.Clear();

  const ezVariant::Type::Enum type = value.GetType();

  switch (type)
  {
    case ezVariant::Type::Invalid:
      break;

    case ezVariant::Type::Bool:
    case ezVariant::Type::Int8:
    case ezVariant::Type::UInt8:
    case ezVariant::Type::Int16:
    case ezVariant::Type::UInt16:
    case ezVariant::Type::Int32:
    case ezVariant::Type::UInt32:
    case ezVariant::Type::Int64:
    case ezVariant::Type::UInt64:
    case ezVariant::Type::Float:
    case ezVariant::Type::Double:
    case ezVariant::Type::String:
    case ezVariant::Type::StringView:
    {
      out_String = value.ConvertTo<ezString>();
      break;
    }

    case ezVariant::Type::Color:
    case ezVariant::Type::ColorGamma:
    {
      const ezColor c = value.ConvertTo<ezColor>();
      out_String.Format("new Color({}, {}, {}, {})", c.r, c.g, c.b, c.a);
      break;
    }

    case ezVariant::Type::Vector2:
    {
      const ezVec2 v = value.Get<ezVec2>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case ezVariant::Type::Vector3:
    {
      const ezVec3 v = value.Get<ezVec3>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case ezVariant::Type::Vector4:
    {
      const ezVec4 v = value.Get<ezVec4>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case ezVariant::Type::Vector2I:
    {
      const ezVec2I32 v = value.Get<ezVec2I32>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case ezVariant::Type::Vector3I:
    {
      const ezVec3I32 v = value.Get<ezVec3I32>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case ezVariant::Type::Vector4I:
    {
      const ezVec4I32 v = value.Get<ezVec4I32>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case ezVariant::Type::Vector2U:
    {
      const ezVec2U32 v = value.Get<ezVec2U32>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case ezVariant::Type::Vector3U:
    {
      const ezVec3U32 v = value.Get<ezVec3U32>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case ezVariant::Type::Vector4U:
    {
      const ezVec4U32 v = value.Get<ezVec4U32>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case ezVariant::Type::Quaternion:
    {
      const ezQuat q = value.Get<ezQuat>();
      out_String.Format("new Quat({}, {}, {}, {})", q.x, q.y, q.z, q.w);
      break;
    }

    case ezVariant::Type::Matrix3:
    {
      out_String = "new Mat3()";
      break;
    }

    case ezVariant::Type::Matrix4:
    {
      out_String = "new Mat4()";
      break;
    }

    case ezVariant::Type::Transform:
    {
      out_String = "new Transform()";
      break;
    }

    case ezVariant::Type::Time:
      out_String.Format("{0}", value.Get<ezTime>().GetSeconds());
      break;

    case ezVariant::Type::Angle:
      out_String.Format("{0}", value.Get<ezAngle>().GetRadian());
      break;

    case ezVariant::Type::Uuid:
    case ezVariant::Type::DataBuffer:
    case ezVariant::Type::VariantArray:
    case ezVariant::Type::VariantDictionary:
    case ezVariant::Type::TypedPointer:
    case ezVariant::Type::TypedObject:
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}
