#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/World.h>
#include <Foundation/Types/Variant.h>


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

void ezAngelScriptUtils::WriteAsProperty(int iPropertyTypeID, void* pPropertyAddress, asIScriptEngine* pEngine, const ezVariant& value)
{
  void* pProp = pPropertyAddress;

  switch (iPropertyTypeID)
  {
    case asTYPEID_BOOL:
      *static_cast<ezInt8*>(pProp) = value.ConvertTo<bool>() ? 1 : 0;
      return;
    case asTYPEID_INT8:
      *static_cast<ezInt8*>(pProp) = value.ConvertTo<ezInt8>();
      return;
    case asTYPEID_INT16:
      *static_cast<ezInt16*>(pProp) = value.ConvertTo<ezInt16>();
      return;
    case asTYPEID_INT32:
      *static_cast<ezInt32*>(pProp) = value.ConvertTo<ezInt32>();
      return;
    case asTYPEID_INT64:
      *static_cast<ezInt64*>(pProp) = value.ConvertTo<ezInt64>();
      return;
    case asTYPEID_UINT8:
      *static_cast<ezUInt8*>(pProp) = value.ConvertTo<ezUInt8>();
      return;
    case asTYPEID_UINT16:
      *static_cast<ezUInt16*>(pProp) = value.ConvertTo<ezUInt16>();
      return;
    case asTYPEID_UINT32:
      *static_cast<ezUInt32*>(pProp) = value.ConvertTo<ezUInt32>();
      return;
    case asTYPEID_UINT64:
      *static_cast<ezUInt64*>(pProp) = value.ConvertTo<ezUInt64>();
      return;
    case asTYPEID_FLOAT:
      *static_cast<float*>(pProp) = value.ConvertTo<float>();
      return;
    case asTYPEID_DOUBLE:
      *static_cast<double*>(pProp) = value.ConvertTo<double>();
      return;
  }

  if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iPropertyTypeID))
  {
    ezStringView sTypeName = pInfo->GetName();
    const ezRTTI* pRtti = (const ezRTTI*)pInfo->GetUserData(ezAsUserData::RttiPtr);

    if (sTypeName == "string")
    {
      *static_cast<std::string*>(pProp) = value.ConvertTo<ezString>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezAngle>())
    {
      *static_cast<ezAngle*>(pProp) = value.ConvertTo<ezAngle>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezTime>())
    {
      *static_cast<ezTime*>(pProp) = value.ConvertTo<ezTime>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezColor>())
    {
      *static_cast<ezColor*>(pProp) = value.ConvertTo<ezColor>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezColorGammaUB>())
    {
      *static_cast<ezColorGammaUB*>(pProp) = value.ConvertTo<ezColorGammaUB>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec2>())
    {
      *static_cast<ezVec2*>(pProp) = value.ConvertTo<ezVec2>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec3>())
    {
      *static_cast<ezVec3*>(pProp) = value.ConvertTo<ezVec3>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezVec4>())
    {
      *static_cast<ezVec4*>(pProp) = value.ConvertTo<ezVec4>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezQuat>())
    {
      *static_cast<ezQuat*>(pProp) = value.ConvertTo<ezQuat>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezTransform>())
    {
      *static_cast<ezTransform*>(pProp) = value.ConvertTo<ezTransform>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezString>())
    {
      *static_cast<ezString*>(pProp) = value.ConvertTo<ezString>();
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezHashedString>())
    {
      static_cast<ezHashedString*>(pProp)->Assign(value.ConvertTo<ezString>());
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezGameObjectHandle>())
    {
      *static_cast<ezGameObjectHandle*>(pProp) = *((const ezGameObjectHandle*)value.GetData());
      return;
    }
    else if (pRtti == ezGetStaticRTTI<ezComponentHandle>())
    {
      *static_cast<ezComponentHandle*>(pProp) = *((const ezComponentHandle*)value.GetData());
      return;
    }
  }

  // currently unsupported type for exposed parameter
  EZ_ASSERT_NOT_IMPLEMENTED;
}

ezResult ezAngelScriptUtils::ReadAsProperty(int iPropertyTypeID, void* pPropertyAddress, asIScriptEngine* pEngine, ezVariant& out_Value)
{
  void* pProp = pPropertyAddress;

  if ((iPropertyTypeID & asTYPEID_APPOBJECT) == 0)
  {

    switch (iPropertyTypeID)
    {
      case asTYPEID_VOID:
        return EZ_FAILURE;

      case asTYPEID_BOOL:
        out_Value = (*static_cast<ezInt8*>(pProp) != 0) ? true : false;
        return EZ_SUCCESS;

      case asTYPEID_INT8:
        out_Value = *static_cast<ezInt8*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_INT16:
        out_Value = *static_cast<ezInt16*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_INT32:
        out_Value = *static_cast<ezInt32*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_INT64:
        out_Value = *static_cast<ezInt64*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_UINT8:
        out_Value = *static_cast<ezUInt8*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_UINT16:
        out_Value = *static_cast<ezUInt16*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_UINT32:
        out_Value = *static_cast<ezUInt32*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_UINT64:
        out_Value = *static_cast<ezUInt64*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_FLOAT:
        out_Value = *static_cast<float*>(pProp);
        return EZ_SUCCESS;

      case asTYPEID_DOUBLE:
        out_Value = *static_cast<double*>(pProp);
        return EZ_SUCCESS;
    }
  }
  else if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iPropertyTypeID))
  {
    const ezString sTypeName = pInfo->GetName();

    // TODO AngelScript: compare against ezRTTI instead of string

    if (sTypeName == "string")
    {
      out_Value = static_cast<std::string*>(pProp)->c_str();
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezAngle")
    {
      out_Value = *static_cast<ezAngle*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezTime")
    {
      out_Value = *static_cast<ezTime*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezColor")
    {
      out_Value = *static_cast<ezColor*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezColorGammaUB")
    {
      out_Value = *static_cast<ezColorGammaUB*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezVec2")
    {
      out_Value = *static_cast<ezVec2*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezVec3")
    {
      out_Value = *static_cast<ezVec3*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezVec4")
    {
      out_Value = *static_cast<ezVec4*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezQuat")
    {
      out_Value = *static_cast<ezQuat*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezTransform")
    {
      out_Value = *static_cast<ezTransform*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezString")
    {
      out_Value = *static_cast<ezString*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezStringView")
    {
      out_Value = *static_cast<ezStringView*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezHashedString")
    {
      out_Value = *static_cast<ezHashedString*>(pProp);
      return EZ_SUCCESS;
    }
    else if (sTypeName == "ezTempHashedString")
    {
      out_Value = *static_cast<ezTempHashedString*>(pProp);
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
    case ezVariantType::TempHashedString:
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

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return "";
}

void ezAngelScriptUtils::RetrieveArg(asIScriptGeneric* gen, ezUInt32 uiArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg)
{
  const ezRTTI* pArgRtti = pAbstractFuncProp->GetArgumentType(uiArg);

  if (pArgRtti->GetTypeFlags().IsAnySet(ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
  {
    out_arg = (ezInt32)gen->GetArgDWord(uiArg);
    return;
  }

  const ezVariantType::Enum type = pArgRtti->GetVariantType();
  switch (type)
  {
    case ezVariantType::Bool:
      out_arg = gen->GetArgByte(uiArg) != 0;
      return;
    case ezVariantType::Double:
      out_arg = gen->GetArgDouble(uiArg);
      return;
    case ezVariantType::Float:
      out_arg = gen->GetArgFloat(uiArg);
      return;
    case ezVariantType::Int8:
      out_arg = (ezInt8)gen->GetArgByte(uiArg);
      return;
    case ezVariantType::Int16:
      out_arg = (ezInt16)gen->GetArgWord(uiArg);
      return;
    case ezVariantType::Int32:
      out_arg = (ezInt32)gen->GetArgDWord(uiArg);
      return;
    case ezVariantType::Int64:
      out_arg = (ezInt64)gen->GetArgQWord(uiArg);
      return;
    case ezVariantType::UInt8:
      out_arg = (ezUInt8)gen->GetArgByte(uiArg);
      return;
    case ezVariantType::UInt16:
      out_arg = (ezUInt16)gen->GetArgWord(uiArg);
      return;
    case ezVariantType::UInt32:
      out_arg = (ezUInt32)gen->GetArgDWord(uiArg);
      return;
    case ezVariantType::UInt64:
      out_arg = (ezUInt64)gen->GetArgQWord(uiArg);
      return;

    case ezVariantType::Vector2:
      out_arg = *((const ezVec2*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Vector3:
      out_arg = *((const ezVec3*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Vector4:
      out_arg = *((const ezVec4*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Quaternion:
      out_arg = *((const ezQuat*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Matrix3:
      out_arg = *((const ezMat3*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Matrix4:
      out_arg = *((const ezMat4*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Transform:
      out_arg = *((const ezTransform*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Time:
      out_arg = *((const ezTime*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Angle:
      out_arg = *((const ezAngle*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::Color:
      out_arg = *((const ezColor*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::ColorGamma:
      out_arg = *((const ezColorGammaUB*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::String:
      out_arg = *((const ezString*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::HashedString:
      out_arg = *((const ezHashedString*)gen->GetArgObject(uiArg));
      return;
    case ezVariantType::StringView:
      out_arg = ezVariant(*(const ezStringView*)gen->GetArgObject(uiArg), false);
      return;
    case ezVariantType::TempHashedString:
      out_arg = *((const ezTempHashedString*)gen->GetArgObject(uiArg));
      return;

    case ezVariantType::VariantArray:
      RetrieveVarArgs(gen, uiArg, pAbstractFuncProp, out_arg);
      return;

    case ezVariantType::Invalid:
    case ezVariantType::TypedObject:
      // handled below
      break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (pArgRtti->IsDerivedFrom(ezGetStaticRTTI<ezComponent>()))
  {
    out_arg = (ezComponent*)gen->GetArgObject(uiArg);
    return;
  }

  if (pArgRtti == ezGetStaticRTTI<ezWorld>())
  {
    out_arg = (ezWorld*)gen->GetArgObject(uiArg);
    return;
  }

  if (pArgRtti == ezGetStaticRTTI<ezGameObjectHandle>())
  {
    out_arg = (ezGameObjectHandle*)gen->GetArgObject(uiArg);
    return;
  }

  if (pArgRtti->GetTypeName().StartsWith("ezVariant"))
  {
    auto argTypeId = gen->GetArgTypeId(uiArg);

    if (ezAngelScriptUtils::ReadAsProperty(argTypeId, gen->GetArgAddress(uiArg), gen->GetEngine(), out_arg).Succeeded())
      return;

    ezStringBuilder typeName("null");
    if (const asITypeInfo* pInfo = gen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    ezLog::Error("Call to '{}': Argument {} got an unsupported type '{}' ({})", pAbstractFuncProp->GetPropertyName(), uiArg, typeName, argTypeId);
    return;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezAngelScriptUtils::RetrieveVarArgs(asIScriptGeneric* gen, ezUInt32 uiStartArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg)
{
  ezVariantArray resArr;

  for (ezUInt32 uiArg = uiStartArg; uiArg < (ezUInt32)gen->GetArgCount(); ++uiArg)
  {
    auto argTypeId = gen->GetArgTypeId(uiArg);

    ezVariant res;
    if (ezAngelScriptUtils::ReadAsProperty(argTypeId, gen->GetArgAddress(uiArg), gen->GetEngine(), res).Succeeded())
    {
      resArr.PushBack(res);
      continue;
    }

    ezStringBuilder typeName("null");
    if (const asITypeInfo* pInfo = gen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    ezLog::Error("Call to '{}': Argument {} got an unsupported type '{}' ({})", pAbstractFuncProp->GetPropertyName(), uiArg, typeName, argTypeId);
    break;
  }

  out_arg = resArr;
}


void ezAngelScriptUtils::MakeGenericFunctionCall(asIScriptGeneric* gen)
{
  const ezAbstractFunctionProperty* pAbstractFuncProp = (const ezAbstractFunctionProperty*)gen->GetAuxiliary();
  const ezScriptableFunctionAttribute* pFuncAttr = pAbstractFuncProp->GetAttributeByType<ezScriptableFunctionAttribute>();
  void* pObject = gen->GetObject();

  EZ_ASSERT_DEBUG(pAbstractFuncProp->GetArgumentCount() < 12, "Too many arguments");
  ezVariant args[12];
  bool bHasOutArgs = false;

  for (ezUInt32 uiArg = 0; uiArg < pAbstractFuncProp->GetArgumentCount(); ++uiArg)
  {
    if (pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
    {
      bHasOutArgs = true;
    }

    ezAngelScriptUtils::RetrieveArg(gen, uiArg, pAbstractFuncProp, args[uiArg]);
  }

  ezVariant ret;
  pAbstractFuncProp->Execute(pObject, args, ret);

  if (bHasOutArgs)
  {
    for (ezUInt32 uiArg = 0; uiArg < pAbstractFuncProp->GetArgumentCount(); ++uiArg)
    {
      if (pFuncAttr->GetArgumentType(uiArg) == ezScriptableFunctionAttribute::ArgType::Out)
      {
        ezAngelScriptUtils::WriteAsProperty(gen->GetArgTypeId(uiArg), gen->GetArgAddress(uiArg), gen->GetEngine(), args[uiArg]);
      }
    }
  }

  if (pAbstractFuncProp->GetReturnType() != nullptr)
  {
    switch (pAbstractFuncProp->GetReturnType()->GetVariantType())
    {
      case ezVariantType::Bool:
        gen->SetReturnByte(ret.Get<bool>() ? 1 : 0);
        return;
      case ezVariantType::Double:
        gen->SetReturnDouble(ret.Get<double>());
        return;
      case ezVariantType::Float:
        gen->SetReturnFloat(ret.Get<float>());
        return;
      case ezVariantType::Int8:
        gen->SetReturnByte(ret.Get<ezInt8>());
        return;
      case ezVariantType::Int16:
        gen->SetReturnWord(ret.Get<ezInt16>());
        return;
      case ezVariantType::Int32:
        gen->SetReturnDWord(ret.Get<ezInt32>());
        return;
      case ezVariantType::Int64:
        gen->SetReturnQWord(ret.Get<ezInt64>());
        return;
      case ezVariantType::UInt8:
        gen->SetReturnByte(ret.Get<ezUInt8>());
        return;
      case ezVariantType::UInt16:
        gen->SetReturnWord(ret.Get<ezUInt16>());
        return;
      case ezVariantType::UInt32:
        gen->SetReturnDWord(ret.Get<ezUInt32>());
        return;
      case ezVariantType::UInt64:
        gen->SetReturnQWord(ret.Get<ezUInt64>());
        return;

      case ezVariantType::Vector2:
        *((ezVec2*)gen->GetAddressOfReturnLocation()) = ret.Get<ezVec2>();
        return;
      case ezVariantType::Vector3:
        *((ezVec3*)gen->GetAddressOfReturnLocation()) = ret.Get<ezVec3>();
        return;
      case ezVariantType::Vector4:
        *((ezVec4*)gen->GetAddressOfReturnLocation()) = ret.Get<ezVec4>();
        return;
      case ezVariantType::Quaternion:
        *((ezQuat*)gen->GetAddressOfReturnLocation()) = ret.Get<ezQuat>();
        return;
      case ezVariantType::Matrix3:
        *((ezMat3*)gen->GetAddressOfReturnLocation()) = ret.Get<ezMat3>();
        return;
      case ezVariantType::Matrix4:
        *((ezMat4*)gen->GetAddressOfReturnLocation()) = ret.Get<ezMat4>();
        return;
      case ezVariantType::Transform:
        *((ezTransform*)gen->GetAddressOfReturnLocation()) = ret.Get<ezTransform>();
        return;
      case ezVariantType::Time:
        *((ezTime*)gen->GetAddressOfReturnLocation()) = ret.Get<ezTime>();
        return;
      case ezVariantType::Angle:
        *((ezAngle*)gen->GetAddressOfReturnLocation()) = ret.Get<ezAngle>();
        return;

      case ezVariantType::String:
      case ezVariantType::HashedString:
      case ezVariantType::StringView:
      case ezVariantType::TempHashedString:
      {
        // TODO AngelScript: all the string types
        EZ_ASSERT_NOT_IMPLEMENTED;
        void* dst = gen->GetAddressOfReturnLocation();
        new (dst) std::string(ret.ConvertTo<ezString>());
        return;
      }

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
}

void ezAngelScriptUtils::DefaultConstructInPlace(void* ptr, const ezRTTI* pRtti)
{
  if (pRtti == ezGetStaticRTTI<ezString>())
  {
    new (ptr) ezString();
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezStringBuilder>())
  {
    new (ptr) ezStringBuilder();
    return;
  }

  // TODO: add other non-POD types here as needed
}
