#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Types/Variant.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

ezMap<ezVisualScriptInstance::AssignFuncKey, ezVisualScriptDataPinAssignFunc> ezVisualScriptInstance::s_DataPinAssignFunctions;

bool ezVisualScriptAssignNumberNumber(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<double*>(pDst) != *reinterpret_cast<const double*>(pSrc);
  *reinterpret_cast<double*>(pDst) = *reinterpret_cast<const double*>(pSrc);
  return res;
}

bool ezVisualScriptAssignNumberBool(const void* pSrc, void* pDst)
{
  const bool res = (*reinterpret_cast<bool*>(pDst) != (*reinterpret_cast<const double*>(pSrc) > 0.0));
  *reinterpret_cast<bool*>(pDst) = *reinterpret_cast<const double*>(pSrc) > 0.0;
  return res;
}

bool ezVisualScriptAssignNumberVec3(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<ezVec3*>(pDst) != ezVec3(static_cast<float>(*reinterpret_cast<const double*>(pSrc)));
  *reinterpret_cast<ezVec3*>(pDst) = ezVec3(static_cast<float>(*reinterpret_cast<const double*>(pSrc)));
  return res;
}

bool ezVisualScriptAssignNumberString(const void* pSrc, void* pDst)
{
  double newValue = *reinterpret_cast<const double*>(pSrc);
  ezStringBuilder sb;
  ezConversionUtils::ToString(newValue, sb);

  const bool res = *reinterpret_cast<ezString*>(pDst) != sb;
  *reinterpret_cast<ezString*>(pDst) = sb;
  return res;
}

bool ezVisualScriptAssignNumberVariant(const void* pSrc, void* pDst)
{
  ezVariant newValue = *reinterpret_cast<const double*>(pSrc);
  const bool res = *reinterpret_cast<ezVariant*>(pDst) != newValue;
  *reinterpret_cast<ezVariant*>(pDst) = std::move(newValue);
  return res;
}


bool ezVisualScriptAssignBoolBool(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<bool*>(pDst) != *reinterpret_cast<const bool*>(pSrc);
  *reinterpret_cast<bool*>(pDst) = *reinterpret_cast<const bool*>(pSrc);
  return res;
}

bool ezVisualScriptAssignBoolNumber(const void* pSrc, void* pDst)
{
  double newValue = *reinterpret_cast<const bool*>(pSrc) ? 1.0 : 0.0;
  const bool res = *reinterpret_cast<double*>(pDst) != newValue;
  *reinterpret_cast<double*>(pDst) = newValue;
  return res;
}

bool ezVisualScriptAssignBoolString(const void* pSrc, void* pDst)
{
  bool newValue = *reinterpret_cast<const bool*>(pSrc);
  ezStringBuilder sb;
  ezConversionUtils::ToString(newValue, sb);

  const bool res = *reinterpret_cast<ezString*>(pDst) != sb;
  *reinterpret_cast<ezString*>(pDst) = sb;
  return res;
}

bool ezVisualScriptAssignBoolVariant(const void* pSrc, void* pDst)
{
  ezVariant newValue = *reinterpret_cast<const bool*>(pSrc);
  const bool res = *reinterpret_cast<ezVariant*>(pDst) != newValue;
  *reinterpret_cast<ezVariant*>(pDst) = std::move(newValue);
  return res;
}


bool ezVisualScriptAssignVec3Vec3(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<ezVec3*>(pDst) != *reinterpret_cast<const ezVec3*>(pSrc);
  *reinterpret_cast<ezVec3*>(pDst) = *reinterpret_cast<const ezVec3*>(pSrc);
  return res;
}

bool ezVisualScriptAssignVec3Variant(const void* pSrc, void* pDst)
{
  ezVariant newValue = *reinterpret_cast<const ezVec3*>(pSrc);
  const bool res = *reinterpret_cast<ezVariant*>(pDst) != newValue;
  *reinterpret_cast<ezVariant*>(pDst) = std::move(newValue);
  return res;
}


bool ezVisualScriptAssignStringString(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<ezString*>(pDst) != *reinterpret_cast<const ezString*>(pSrc);
  *reinterpret_cast<ezString*>(pDst) = *reinterpret_cast<const ezString*>(pSrc);
  return res;
}

bool ezVisualScriptAssignStringVariant(const void* pSrc, void* pDst)
{
  ezVariant newValue = *reinterpret_cast<const ezString*>(pSrc);
  const bool res = *reinterpret_cast<ezVariant*>(pDst) != newValue;
  *reinterpret_cast<ezVariant*>(pDst) = std::move(newValue);
  return res;
}


bool ezVisualScriptAssignGameObject(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<ezGameObjectHandle*>(pDst) != *reinterpret_cast<const ezGameObjectHandle*>(pSrc);
  *reinterpret_cast<ezGameObjectHandle*>(pDst) = *reinterpret_cast<const ezGameObjectHandle*>(pSrc);
  return res;
}

bool ezVisualScriptAssignComponent(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<ezComponentHandle*>(pDst) != *reinterpret_cast<const ezComponentHandle*>(pSrc);
  *reinterpret_cast<ezComponentHandle*>(pDst) = *reinterpret_cast<const ezComponentHandle*>(pSrc);
  return res;
}

bool ezVisualScriptAssignVariantVariant(const void* pSrc, void* pDst)
{
  const bool res = *reinterpret_cast<ezVariant*>(pDst) != *reinterpret_cast<const ezVariant*>(pSrc);
  *reinterpret_cast<ezVariant*>(pDst) = *reinterpret_cast<const ezVariant*>(pSrc);
  return res;
}

void ezVisualScriptInstance::SetupPinDataTypeConversions()
{
  static bool bDone = false;
  if (bDone)
    return;

  bDone = true;

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Number, ezVisualScriptAssignNumberNumber);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Boolean, ezVisualScriptAssignNumberBool);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Vec3, ezVisualScriptAssignNumberVec3);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::String, ezVisualScriptAssignNumberString);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Number, ezVisualScriptDataPinType::Variant, ezVisualScriptAssignNumberVariant);

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Boolean, ezVisualScriptAssignBoolBool);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Number, ezVisualScriptAssignBoolNumber);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::String, ezVisualScriptAssignBoolString);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Boolean, ezVisualScriptDataPinType::Variant, ezVisualScriptAssignBoolVariant);

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Vec3, ezVisualScriptDataPinType::Vec3, ezVisualScriptAssignVec3Vec3);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Vec3, ezVisualScriptDataPinType::Variant, ezVisualScriptAssignVec3Variant);

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::String, ezVisualScriptDataPinType::String, ezVisualScriptAssignStringString);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::String, ezVisualScriptDataPinType::Variant, ezVisualScriptAssignStringVariant);

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptDataPinType::GameObjectHandle, ezVisualScriptAssignGameObject);
  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::ComponentHandle, ezVisualScriptDataPinType::ComponentHandle, ezVisualScriptAssignComponent);

  RegisterDataPinAssignFunction(ezVisualScriptDataPinType::Variant, ezVisualScriptDataPinType::Variant, ezVisualScriptAssignVariantVariant);
}

void ezVisualScriptInstance::RegisterDataPinAssignFunction(
  ezVisualScriptDataPinType::Enum sourceType, ezVisualScriptDataPinType::Enum dstType, ezVisualScriptDataPinAssignFunc func)
{
  AssignFuncKey key;
  key.m_SourceType = sourceType;
  key.m_DstType = dstType;

  s_DataPinAssignFunctions[key] = func;
}

ezVisualScriptDataPinAssignFunc ezVisualScriptInstance::FindDataPinAssignFunction(
  ezVisualScriptDataPinType::Enum sourceType, ezVisualScriptDataPinType::Enum dstType)
{
  AssignFuncKey key;
  key.m_SourceType = sourceType;
  key.m_DstType = dstType;

  return s_DataPinAssignFunctions.GetValueOrDefault(key, nullptr);
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Implementation_VisualScriptInstanceAssignFunctions);
