#include <GameEnginePCH.h>

#include <Foundation/Types/Variant.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

ezMap<ezVisualScriptInstance::AssignFuncKey, ezVisualScriptDataPinAssignFunc> ezVisualScriptInstance::s_DataPinAssignFunctions;

bool ezVisualScriptAssignNumberNumber(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<double*>(dst) != *reinterpret_cast<const double*>(src);
  *reinterpret_cast<double*>(dst) = *reinterpret_cast<const double*>(src);
  return res;
}

bool ezVisualScriptAssignNumberBool(const void* src, void* dst)
{
  const bool res = (*reinterpret_cast<bool*>(dst) != (*reinterpret_cast<const double*>(src) > 0.0));
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const double*>(src) > 0.0;
  return res;
}

bool ezVisualScriptAssignNumberVec3(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezVec3*>(dst) != ezVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
  *reinterpret_cast<ezVec3*>(dst) = ezVec3(static_cast<float>(*reinterpret_cast<const double*>(src)));
  return res;
}

bool ezVisualScriptAssignNumberString(const void* src, void* dst)
{
  double newValue = *reinterpret_cast<const double*>(src);
  ezStringBuilder sb;
  ezConversionUtils::ToString(newValue, sb);

  const bool res = *reinterpret_cast<ezString*>(dst) != sb;
  *reinterpret_cast<ezString*>(dst) = sb;
  return res;
}

bool ezVisualScriptAssignNumberVariant(const void* src, void* dst)
{
  ezVariant newValue = *reinterpret_cast<const double*>(src);
  const bool res = *reinterpret_cast<ezVariant*>(dst) != newValue;
  *reinterpret_cast<ezVariant*>(dst) = newValue;
  return res;
}


bool ezVisualScriptAssignBoolBool(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<bool*>(dst) != *reinterpret_cast<const bool*>(src);
  *reinterpret_cast<bool*>(dst) = *reinterpret_cast<const bool*>(src);
  return res;
}

bool ezVisualScriptAssignBoolNumber(const void* src, void* dst)
{
  double newValue = *reinterpret_cast<const bool*>(src) ? 1.0 : 0.0;
  const bool res = *reinterpret_cast<double*>(dst) != newValue;
  *reinterpret_cast<double*>(dst) = newValue;
  return res;
}

bool ezVisualScriptAssignBoolString(const void* src, void* dst)
{
  bool newValue = *reinterpret_cast<const bool*>(src);
  ezStringBuilder sb;
  ezConversionUtils::ToString(newValue, sb);

  const bool res = *reinterpret_cast<ezString*>(dst) != sb;
  *reinterpret_cast<ezString*>(dst) = sb;
  return res;
}

bool ezVisualScriptAssignBoolVariant(const void* src, void* dst)
{
  ezVariant newValue = *reinterpret_cast<const bool*>(src);
  const bool res = *reinterpret_cast<ezVariant*>(dst) != newValue;
  *reinterpret_cast<ezVariant*>(dst) = newValue;
  return res;
}


bool ezVisualScriptAssignVec3Vec3(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezVec3*>(dst) != *reinterpret_cast<const ezVec3*>(src);
  *reinterpret_cast<ezVec3*>(dst) = *reinterpret_cast<const ezVec3*>(src);
  return res;
}

bool ezVisualScriptAssignVec3Variant(const void* src, void* dst)
{
  ezVariant newValue = *reinterpret_cast<const ezVec3*>(src);
  const bool res = *reinterpret_cast<ezVariant*>(dst) != newValue;
  *reinterpret_cast<ezVariant*>(dst) = newValue;
  return res;
}


bool ezVisualScriptAssignStringString(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezString*>(dst) != *reinterpret_cast<const ezString*>(src);
  *reinterpret_cast<ezString*>(dst) = *reinterpret_cast<const ezString*>(src);
  return res;
}

bool ezVisualScriptAssignStringVariant(const void* src, void* dst)
{
  ezVariant newValue = *reinterpret_cast<const ezString*>(src);
  const bool res = *reinterpret_cast<ezVariant*>(dst) != newValue;
  *reinterpret_cast<ezVariant*>(dst) = newValue;
  return res;
}


bool ezVisualScriptAssignGameObject(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezGameObjectHandle*>(dst) != *reinterpret_cast<const ezGameObjectHandle*>(src);
  *reinterpret_cast<ezGameObjectHandle*>(dst) = *reinterpret_cast<const ezGameObjectHandle*>(src);
  return res;
}

bool ezVisualScriptAssignComponent(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezComponentHandle*>(dst) != *reinterpret_cast<const ezComponentHandle*>(src);
  *reinterpret_cast<ezComponentHandle*>(dst) = *reinterpret_cast<const ezComponentHandle*>(src);
  return res;
}

bool ezVisualScriptAssignVariantVariant(const void* src, void* dst)
{
  const bool res = *reinterpret_cast<ezVariant*>(dst) != *reinterpret_cast<const ezVariant*>(src);
  *reinterpret_cast<ezVariant*>(dst) = *reinterpret_cast<const ezVariant*>(src);
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
