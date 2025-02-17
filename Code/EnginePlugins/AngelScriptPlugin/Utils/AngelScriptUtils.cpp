#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/World.h>
#include <Foundation/Types/Variant.h>

const char* ezAngelScriptUtils::GetAsTypeName(asIScriptEngine* pEngine, int iAsTypeID)
{
  switch (iAsTypeID)
  {
    case asTYPEID_BOOL:
      return "bool";
    case asTYPEID_INT8:
      return "int8";
    case asTYPEID_INT16:
      return "int16";
    case asTYPEID_INT32:
      return "int32";
    case asTYPEID_INT64:
      return "int64";
    case asTYPEID_UINT8:
      return "uint8";
    case asTYPEID_UINT16:
      return "uint16";
    case asTYPEID_UINT32:
      return "uint32";
    case asTYPEID_UINT64:
      return "uint64";
    case asTYPEID_FLOAT:
      return "float";
    case asTYPEID_DOUBLE:
      return "double";

    default:
      if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
      {
        return pInfo->GetName();
      }

      return nullptr;
  }
}


ezString ezAngelScriptUtils::GetNiceFunctionDeclaration(const asIScriptFunction* pFunc, bool bIncludeObjectName, bool bIncludeNamespace)
{
  ezStringBuilder tmp;

  tmp = pFunc->GetDeclaration(bIncludeObjectName, bIncludeNamespace, true);

  tmp.ReplaceAll(" :: ", "::");
  tmp.ReplaceAll(" (", "(");
  tmp.ReplaceAll("( ", "(");
  tmp.ReplaceAll(" )", ")");
  tmp.ReplaceAll(") ", ")");
  tmp.ReplaceAll(" ,", ",");
  tmp.ReplaceAll(")const", ") const");

  return tmp;
}

const ezRTTI* ezAngelScriptUtils::MapToRTTI(int iAsTypeID, asIScriptEngine* pEngine)
{
  if (iAsTypeID == asTYPEID_BOOL)
    return ezGetStaticRTTI<bool>();

  if (iAsTypeID == asTYPEID_INT8)
    return ezGetStaticRTTI<ezInt8>();

  if (iAsTypeID == asTYPEID_INT16)
    return ezGetStaticRTTI<ezInt16>();

  if (iAsTypeID == asTYPEID_INT32)
    return ezGetStaticRTTI<ezInt32>();

  if (iAsTypeID == asTYPEID_INT64)
    return ezGetStaticRTTI<ezInt64>();

  if (iAsTypeID == asTYPEID_UINT8)
    return ezGetStaticRTTI<ezUInt8>();

  if (iAsTypeID == asTYPEID_UINT16)
    return ezGetStaticRTTI<ezUInt16>();

  if (iAsTypeID == asTYPEID_UINT32)
    return ezGetStaticRTTI<ezUInt32>();

  if (iAsTypeID == asTYPEID_UINT64)
    return ezGetStaticRTTI<ezUInt64>();

  if (iAsTypeID == asTYPEID_FLOAT)
    return ezGetStaticRTTI<float>();

  if (iAsTypeID == asTYPEID_DOUBLE)
    return ezGetStaticRTTI<double>();

  if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
  {
    return (const ezRTTI*)pInfo->GetUserData(ezAsUserData::RttiPtr);
  }

  return nullptr;
}

ezResult ezAngelScriptUtils::WriteToAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, const ezVariant& value)
{
  void* pMemDst = pMemoryLocation;

  switch (iAsTypeID)
  {
    case asTYPEID_BOOL:
      *static_cast<ezInt8*>(pMemDst) = value.ConvertTo<bool>() ? 1 : 0;
      return EZ_SUCCESS;
    case asTYPEID_INT8:
      *static_cast<ezInt8*>(pMemDst) = value.ConvertTo<ezInt8>();
      return EZ_SUCCESS;
    case asTYPEID_INT16:
      *static_cast<ezInt16*>(pMemDst) = value.ConvertTo<ezInt16>();
      return EZ_SUCCESS;
    case asTYPEID_INT32:
      *static_cast<ezInt32*>(pMemDst) = value.ConvertTo<ezInt32>();
      return EZ_SUCCESS;
    case asTYPEID_INT64:
      *static_cast<ezInt64*>(pMemDst) = value.ConvertTo<ezInt64>();
      return EZ_SUCCESS;
    case asTYPEID_UINT8:
      *static_cast<ezUInt8*>(pMemDst) = value.ConvertTo<ezUInt8>();
      return EZ_SUCCESS;
    case asTYPEID_UINT16:
      *static_cast<ezUInt16*>(pMemDst) = value.ConvertTo<ezUInt16>();
      return EZ_SUCCESS;
    case asTYPEID_UINT32:
      *static_cast<ezUInt32*>(pMemDst) = value.ConvertTo<ezUInt32>();
      return EZ_SUCCESS;
    case asTYPEID_UINT64:
      *static_cast<ezUInt64*>(pMemDst) = value.ConvertTo<ezUInt64>();
      return EZ_SUCCESS;
    case asTYPEID_FLOAT:
      *static_cast<float*>(pMemDst) = value.ConvertTo<float>();
      return EZ_SUCCESS;
    case asTYPEID_DOUBLE:
      *static_cast<double*>(pMemDst) = value.ConvertTo<double>();
      return EZ_SUCCESS;
  }

  if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
  {
    const ezRTTI* pRtti = (const ezRTTI*)pInfo->GetUserData(ezAsUserData::RttiPtr);

    if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
    {
      *static_cast<ezUInt32*>(pMemDst) = value.ConvertTo<ezUInt32>();
      return EZ_SUCCESS;
    }

    if (pRtti == ezGetStaticRTTI<ezAngle>())
    {
      *static_cast<ezAngle*>(pMemDst) = value.ConvertTo<ezAngle>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezTime>())
    {
      *static_cast<ezTime*>(pMemDst) = value.ConvertTo<ezTime>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezColor>())
    {
      *static_cast<ezColor*>(pMemDst) = value.ConvertTo<ezColor>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezColorGammaUB>())
    {
      *static_cast<ezColorGammaUB*>(pMemDst) = value.ConvertTo<ezColorGammaUB>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec2>())
    {
      *static_cast<ezVec2*>(pMemDst) = value.ConvertTo<ezVec2>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec3>())
    {
      *static_cast<ezVec3*>(pMemDst) = value.ConvertTo<ezVec3>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec4>())
    {
      *static_cast<ezVec4*>(pMemDst) = value.ConvertTo<ezVec4>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezQuat>())
    {
      *static_cast<ezQuat*>(pMemDst) = value.ConvertTo<ezQuat>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezTransform>())
    {
      *static_cast<ezTransform*>(pMemDst) = value.ConvertTo<ezTransform>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezString>())
    {
      *static_cast<ezString*>(pMemDst) = value.ConvertTo<ezString>();
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezHashedString>())
    {
      static_cast<ezHashedString*>(pMemDst)->Assign(value.ConvertTo<ezString>());
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezGameObjectHandle>())
    {
      *static_cast<ezGameObjectHandle*>(pMemDst) = *((const ezGameObjectHandle*)value.GetData());
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezComponentHandle>())
    {
      *static_cast<ezComponentHandle*>(pMemDst) = *((const ezComponentHandle*)value.GetData());
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezWorld>())
    {
      *static_cast<ezWorld**>(pMemDst) = ((ezWorld*)value.GetData());
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezGameObject>())
    {
      *static_cast<ezGameObject**>(pMemDst) = ((ezGameObject*)value.GetData());
      return EZ_SUCCESS;
    }
  }

  // currently unsupported type for exposed parameter
  return EZ_FAILURE;
}

ezResult ezAngelScriptUtils::ReadFromAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, ezVariant& out_value)
{
  void* pMemLoc = pMemoryLocation;

  if ((iAsTypeID & asTYPEID_APPOBJECT) == 0)
  {
    switch (iAsTypeID)
    {
      case asTYPEID_VOID:
        return EZ_FAILURE;

      case asTYPEID_BOOL:
        out_value = (*static_cast<ezInt8*>(pMemLoc) != 0) ? true : false;
        return EZ_SUCCESS;

      case asTYPEID_INT8:
        out_value = *static_cast<ezInt8*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_INT16:
        out_value = *static_cast<ezInt16*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_INT32:
        out_value = *static_cast<ezInt32*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_INT64:
        out_value = *static_cast<ezInt64*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_UINT8:
        out_value = *static_cast<ezUInt8*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_UINT16:
        out_value = *static_cast<ezUInt16*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_UINT32:
        out_value = *static_cast<ezUInt32*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_UINT64:
        out_value = *static_cast<ezUInt64*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_FLOAT:
        out_value = *static_cast<float*>(pMemLoc);
        return EZ_SUCCESS;

      case asTYPEID_DOUBLE:
        out_value = *static_cast<double*>(pMemLoc);
        return EZ_SUCCESS;
    }
  }
  else if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
  {
    const ezRTTI* pRtti = (const ezRTTI*)pInfo->GetUserData(ezAsUserData::RttiPtr);

    if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
    {
      out_value = *static_cast<ezUInt32*>(pMemLoc);
      return EZ_SUCCESS;
    }

    if (pRtti == ezGetStaticRTTI<ezAngle>())
    {
      out_value = *static_cast<ezAngle*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezTime>())
    {
      out_value = *static_cast<ezTime*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezColor>())
    {
      out_value = *static_cast<ezColor*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezColorGammaUB>())
    {
      out_value = *static_cast<ezColorGammaUB*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec2>())
    {
      out_value = *static_cast<ezVec2*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec3>())
    {
      out_value = *static_cast<ezVec3*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec4>())
    {
      out_value = *static_cast<ezVec4*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezQuat>())
    {
      out_value = *static_cast<ezQuat*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezTransform>())
    {
      out_value = *static_cast<ezTransform*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezString>())
    {
      out_value = *static_cast<ezString*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezStringView>())
    {
      out_value = *static_cast<ezStringView*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezHashedString>())
    {
      out_value = *static_cast<ezHashedString*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezTempHashedString>())
    {
      out_value = *static_cast<ezTempHashedString*>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezGameObjectHandle>())
    {
      out_value = *static_cast<ezGameObjectHandle**>(pMemLoc);
      return EZ_SUCCESS;
    }
    else if (pRtti == ezGetStaticRTTI<ezComponentHandle>())
    {
      out_value = *static_cast<ezComponentHandle**>(pMemLoc);
      return EZ_SUCCESS;
    }
  }

  // currently unsupported type for exposed parameter
  return EZ_FAILURE;
}

const char* ezAngelScriptUtils::VariantTypeToString(ezVariantType::Enum type)
{
  switch (type)
  {
    case ezVariantType::Bool:
      return "bool";
    case ezVariantType::Double:
      return "double";
    case ezVariantType::Float:
      return "float";
    case ezVariantType::Int8:
      return "int8";
    case ezVariantType::Int16:
      return "int16";
    case ezVariantType::Int32:
      return "int32";
    case ezVariantType::Int64:
      return "int64";
    case ezVariantType::UInt8:
      return "uint8";
    case ezVariantType::UInt16:
      return "uint16";
    case ezVariantType::UInt32:
      return "uint32";
    case ezVariantType::UInt64:
      return "uint64";
    case ezVariantType::Angle:
      return "ezAngle";
    case ezVariantType::Matrix3:
      return "ezMat3";
    case ezVariantType::Matrix4:
      return "ezMat4";
    case ezVariantType::Quaternion:
      return "ezQuat";
    case ezVariantType::Time:
      return "ezTime";
    case ezVariantType::Transform:
      return "ezTransform";
    case ezVariantType::Vector2:
      return "ezVec2";
    case ezVariantType::Vector3:
      return "ezVec3";
    case ezVariantType::Vector4:
      return "ezVec4";
    case ezVariantType::Color:
      return "ezColor";
    case ezVariantType::ColorGamma:
      return "ezColorGammaUB";
    case ezVariantType::String:
      return "ezString";
    case ezVariantType::HashedString:
      return "ezHashedString";
    case ezVariantType::StringView:
      return "ezStringView";
    case ezVariantType::TempHashedString:
      return "ezTempHashedString";

    default:
      break;
  }

  return nullptr;
}

ezString ezAngelScriptUtils::DefaultValueToString(const ezVariant& value)
{
  ezStringBuilder s;
  switch (value.GetType())
  {
    case ezVariantType::Angle:
      s.SetFormat("ezAngle::MakeDegrees({})", value.Get<ezAngle>().GetDegree());
      return s;
    case ezVariantType::Bool:
      s.Set(value.Get<bool>() ? "true" : "false");
      return s;
    case ezVariantType::Color:
      s.SetFormat("ezColor({}, {}, {}, {})", value.Get<ezColor>().r, value.Get<ezColor>().g, value.Get<ezColor>().b, value.Get<ezColor>().a);
      return s;

    case ezVariantType::ColorGamma:
      s.SetFormat("ezColorGammaUB({}, {}, {}, {})", value.Get<ezColorGammaUB>().r, value.Get<ezColorGammaUB>().g, value.Get<ezColorGammaUB>().b, value.Get<ezColorGammaUB>().a);
      return s;

    case ezVariantType::Double:
      s.SetFormat("{}", value.Get<double>());
      return s;

    case ezVariantType::Float:
      s.SetFormat("{}", value.Get<float>());
      return s;

    case ezVariantType::HashedString:
    case ezVariantType::String:
    case ezVariantType::StringView:
      s.SetFormat("\"{}\"", value.ConvertTo<ezString>());
      return s;

    case ezVariantType::Int8:
    case ezVariantType::Int16:
    case ezVariantType::Int32:
    case ezVariantType::Int64:
      s.SetFormat("{}", value.ConvertTo<ezInt64>());
      return s;

    case ezVariantType::UInt8:
    case ezVariantType::UInt16:
    case ezVariantType::UInt32:
    case ezVariantType::UInt64:
      s.SetFormat("{}", value.ConvertTo<ezUInt64>());
      return s;

    case ezVariantType::Matrix3:
      s.Set("ezMat3::MakeIdentity()");
      return s;
    case ezVariantType::Matrix4:
      s.Set("ezMat4::MakeIdentity()");
      return s;

    case ezVariantType::Quaternion:
      s.Set("ezQuat::MakeIdentity()");
      return s;

    case ezVariantType::Time:
      s.SetFormat("ezTime::Seconds({})", value.Get<ezTime>().GetSeconds());
      return s;

    case ezVariantType::Transform:
      s.Set("ezTransform::MakeIdentity()");
      return s;

    case ezVariantType::Vector2:
      s.SetFormat("ezVec2({}, {})", value.Get<ezVec2>().x, value.Get<ezVec2>().y);
      return s;
    case ezVariantType::Vector3:
      s.SetFormat("ezVec3({}, {}, {})", value.Get<ezVec3>().x, value.Get<ezVec3>().y, value.Get<ezVec3>().z);
      return s;
    case ezVariantType::Vector4:
      s.SetFormat("ezVec4({}, {}, {}, {})", value.Get<ezVec4>().x, value.Get<ezVec4>().y, value.Get<ezVec4>().z, value.Get<ezVec4>().w);
      return s;

    default:
      break;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

void ezAngelScriptUtils::RetrieveArg(asIScriptGeneric* pGen, ezUInt32 uiArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg)
{
  const ezRTTI* pArgRtti = pAbstractFuncProp->GetArgumentType(uiArg);

  if (pArgRtti->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
  {
    out_arg = (ezInt32)pGen->GetArgDWord(uiArg);
    return;
  }

  const ezVariantType::Enum type = pArgRtti->GetVariantType();
  switch (type)
  {
    case ezVariantType::Bool:
      out_arg = pGen->GetArgByte(uiArg) != 0;
      return;
    case ezVariantType::Double:
      out_arg = pGen->GetArgDouble(uiArg);
      return;
    case ezVariantType::Float:
      out_arg = pGen->GetArgFloat(uiArg);
      return;
    case ezVariantType::Int8:
      out_arg = (ezInt8)pGen->GetArgByte(uiArg);
      return;
    case ezVariantType::Int16:
      out_arg = (ezInt16)pGen->GetArgWord(uiArg);
      return;
    case ezVariantType::Int32:
      out_arg = (ezInt32)pGen->GetArgDWord(uiArg);
      return;
    case ezVariantType::Int64:
      out_arg = (ezInt64)pGen->GetArgQWord(uiArg);
      return;
    case ezVariantType::UInt8:
      out_arg = (ezUInt8)pGen->GetArgByte(uiArg);
      return;
    case ezVariantType::UInt16:
      out_arg = (ezUInt16)pGen->GetArgWord(uiArg);
      return;
    case ezVariantType::UInt32:
      out_arg = (ezUInt32)pGen->GetArgDWord(uiArg);
      return;
    case ezVariantType::UInt64:
      out_arg = (ezUInt64)pGen->GetArgQWord(uiArg);
      return;

    case ezVariantType::Vector2:
      out_arg = *((const ezVec2*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Vector3:
      out_arg = *((const ezVec3*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Vector4:
      out_arg = *((const ezVec4*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Quaternion:
      out_arg = *((const ezQuat*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Matrix3:
      out_arg = *((const ezMat3*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Matrix4:
      out_arg = *((const ezMat4*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Transform:
      out_arg = *((const ezTransform*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Time:
      out_arg = *((const ezTime*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Angle:
      out_arg = *((const ezAngle*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::Color:
      out_arg = *((const ezColor*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::ColorGamma:
      out_arg = *((const ezColorGammaUB*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::String:
      out_arg = *((const ezString*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::HashedString:
      out_arg = *((const ezHashedString*)pGen->GetArgObject(uiArg));
      return;
    case ezVariantType::StringView:
      out_arg = ezVariant(*(const ezStringView*)pGen->GetArgObject(uiArg), false);
      return;
    case ezVariantType::TempHashedString:
      out_arg = *((const ezTempHashedString*)pGen->GetArgObject(uiArg));
      return;

    case ezVariantType::VariantArray:
      RetrieveVarArgs(pGen, uiArg, pAbstractFuncProp, out_arg);
      return;

    case ezVariantType::Invalid:
    case ezVariantType::TypedObject:
      // handled below
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (pArgRtti->IsDerivedFrom(ezGetStaticRTTI<ezComponent>()))
  {
    out_arg = (ezComponent*)pGen->GetArgObject(uiArg);
    return;
  }

  if (pArgRtti == ezGetStaticRTTI<ezWorld>())
  {
    out_arg = (ezWorld*)pGen->GetArgObject(uiArg);
    return;
  }

  if (pArgRtti == ezGetStaticRTTI<ezGameObjectHandle>())
  {
    out_arg = (ezGameObjectHandle*)pGen->GetArgObject(uiArg);
    return;
  }

  if (pArgRtti->GetTypeName().StartsWith("ezVariant"))
  {
    auto argTypeId = pGen->GetArgTypeId(uiArg);

    if (ezAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), argTypeId, pGen->GetArgAddress(uiArg), out_arg).Succeeded())
      return;

    const char* typeName = "null";
    if (const asITypeInfo* pInfo = pGen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    ezLog::Error("Call to '{}': Argument {} got an unsupported type '{}' ({})", pAbstractFuncProp->GetPropertyName(), uiArg, typeName, argTypeId);
    return;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  // out_arg = ezVariant(pGen->GetArgObject(uiArg), pArgRtti); // works, but currently isn't needed
}

void ezAngelScriptUtils::RetrieveVarArgs(asIScriptGeneric* pGen, ezUInt32 uiStartArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg)
{
  ezVariantArray resArr;

  for (ezUInt32 uiArg = uiStartArg; uiArg < (ezUInt32)pGen->GetArgCount(); ++uiArg)
  {
    auto argTypeId = pGen->GetArgTypeId(uiArg);

    ezVariant res;
    if (ezAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), argTypeId, pGen->GetArgAddress(uiArg), res).Succeeded())
    {
      resArr.PushBack(res);
      continue;
    }

    ezStringBuilder typeName("null");
    if (const asITypeInfo* pInfo = pGen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    ezLog::Error("Call to '{}': Argument {} got an unsupported type '{}' ({})", pAbstractFuncProp->GetPropertyName(), uiArg, typeName, argTypeId);
    break;
  }

  out_arg = resArr;
}

void ezAngelScriptUtils::MakeGenericFunctionCall(asIScriptGeneric* pGen)
{
  const ezAbstractFunctionProperty* pAbstractFuncProp = (const ezAbstractFunctionProperty*)pGen->GetAuxiliary();
  const ezScriptableFunctionAttribute* pFuncAttr = pAbstractFuncProp->GetAttributeByType<ezScriptableFunctionAttribute>();
  void* pObject = pGen->GetObject();

  EZ_ASSERT_DEBUG(pAbstractFuncProp->GetArgumentCount() < 12, "Too many arguments");
  ezVariant args[12];
  bool bHasOutArgs = false;

  for (ezUInt32 uiArg = 0; uiArg < pAbstractFuncProp->GetArgumentCount(); ++uiArg)
  {
    if (pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
    {
      bHasOutArgs = true;
    }

    ezAngelScriptUtils::RetrieveArg(pGen, uiArg, pAbstractFuncProp, args[uiArg]);
  }

  ezVariant ret;
  pAbstractFuncProp->Execute(pObject, args, ret);

  if (bHasOutArgs)
  {
    for (ezUInt32 uiArg = 0; uiArg < pAbstractFuncProp->GetArgumentCount(); ++uiArg)
    {
      if (pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
      {
        ezAngelScriptUtils::WriteToAsTypeAtLocation(pGen->GetEngine(), pGen->GetArgTypeId(uiArg), pGen->GetArgAddress(uiArg), args[uiArg]).AssertSuccess();
      }
    }
  }

  if (pAbstractFuncProp->GetReturnType() != nullptr)
  {
    ezAngelScriptUtils::DefaultConstructInPlace(pGen->GetAddressOfReturnLocation(), pAbstractFuncProp->GetReturnType());
    ezAngelScriptUtils::WriteToAsTypeAtLocation(pGen->GetEngine(), pGen->GetReturnTypeId(), pGen->GetAddressOfReturnLocation(), ret).AssertSuccess();
  }
}

void ezAngelScriptUtils::DefaultConstructInPlace(void* pPtr, const ezRTTI* pRtti)
{
  if (pRtti == ezGetStaticRTTI<ezString>())
  {
    new (pPtr) ezString();
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezStringBuilder>())
  {
    new (pPtr) ezStringBuilder();
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezHashedString>())
  {
    new (pPtr) ezHashedString();
    return;
  }

  // TODO: add other non-POD types here as needed
}
