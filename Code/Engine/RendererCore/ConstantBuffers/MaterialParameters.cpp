#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <RendererCore/Shader/ShaderStageBinary.h>

ezUInt64 ezRenderContext::s_LastMaterialParamModification = 1;
static ezMap<ezUInt32, ezDynamicArray<ezUInt8> > s_MaterialParameters;

ezUInt32 ezShaderMaterialParamCB::MaterialParameter::s_TypeSize[(ezUInt32) Type::ENUM_COUNT] =
{
  0,
  sizeof(float) * 1,
  sizeof(float) * 2,
  sizeof(float) * 3,
  sizeof(float) * 4,
  sizeof(int) * 1,
  sizeof(int) * 2,
  sizeof(int) * 3,
  sizeof(int) * 4,
  sizeof(float) * 9,
  sizeof(float) * 16,
  sizeof(float) * 12,
};

ezRenderContext::MaterialParam* ezRenderContext::InternalSetMaterialParameter(const ezTempHashedString& sName, ezShaderMaterialParamCB::MaterialParameter::Type type, ezUInt32 uiMaxArrayElements)
{
  auto& p = s_MaterialParameters[sName.GetHash()];

  MaterialParam* pData = reinterpret_cast<MaterialParam*>(p.GetData());

  if (p.IsEmpty())
  {
    ++s_LastMaterialParamModification;

    p.SetCount(sizeof(MaterialParam) + ezShaderMaterialParamCB::MaterialParameter::s_TypeSize[(ezUInt32) type]);

    pData = reinterpret_cast<MaterialParam*>(p.GetData());

    pData->m_Type = type;
    pData->m_LastModification = s_LastMaterialParamModification;
    pData->m_uiArrayElements = uiMaxArrayElements;
    pData->m_uiDataSize = ezShaderMaterialParamCB::MaterialParameter::s_TypeSize[(ezUInt32) pData->m_Type] * pData->m_uiArrayElements;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pData->m_Type != type)
  {
    ezLog::Error("ezRenderContext::SetMaterialParameter(0x%08X): Parameter is used with different types (%u and %u)", (ezUInt32) pData->m_Type, (ezUInt32) type);
    return nullptr;
  }
#endif

  return pData;
}

#define SetMaterialParameterX(TypeType, TypeEnum, ArrayMax) \
  void ezRenderContext::SetMaterialParameter(const ezTempHashedString& sName, const TypeType& value) \
  { \
    MaterialParam* pParam = InternalSetMaterialParameter(sName, ezShaderMaterialParamCB::MaterialParameter::Type::TypeEnum, ArrayMax); \
    \
    if (pParam == nullptr) \
      return; \
\
    TypeType* pValue = reinterpret_cast<TypeType*>(pParam + 1); \
\
    if (*pValue == value) \
      return; \
\
    ++s_LastMaterialParamModification; \
\
    *pValue = value; \
    pParam->m_LastModification = s_LastMaterialParamModification; \
  }

SetMaterialParameterX(float, Float1, 1);
SetMaterialParameterX(ezVec2, Float2, 1);
SetMaterialParameterX(ezVec3, Float3, 1);
SetMaterialParameterX(ezVec4, Float4, 1);
SetMaterialParameterX(ezColor, Float4, 1);
SetMaterialParameterX(ezInt32, Int1, 1);
SetMaterialParameterX(ezVec2I32, Int2, 1);
SetMaterialParameterX(ezVec3I32, Int3, 1);
SetMaterialParameterX(ezVec4I32, Int4, 1);
//SetMaterialParameterX(ezMat3, Mat3x3, 1);
SetMaterialParameterX(ezMat4, Mat4x4, 1);
SetMaterialParameterX(ezTransform, Mat3x4, 1);

void ezRenderContext::SetMaterialParameter(const ezTempHashedString& sName, const ezVariant& value)
{
  switch (value.GetType())
  {
  // case ezVariant::Type::Bool: /// \todo Bool parameters
  /// \todo Int vector
  case ezVariant::Type::Int8:
  case ezVariant::Type::UInt8:
  case ezVariant::Type::Int16:
  case ezVariant::Type::UInt16:
  case ezVariant::Type::Int32:
  case ezVariant::Type::UInt32:
  case ezVariant::Type::Int64:
  case ezVariant::Type::UInt64:
    SetMaterialParameter(sName, value.ConvertTo<ezInt32>());
    return;
  case ezVariant::Type::Float:
    SetMaterialParameter(sName, value.Get<float>());
    return;
  case ezVariant::Type::Double:
    SetMaterialParameter(sName, value.ConvertTo<float>());
    return;
  case ezVariant::Type::Color:
    SetMaterialParameter(sName, value.Get<ezColor>());
    return;
  case ezVariant::Type::Vector2:
    SetMaterialParameter(sName, value.Get<ezVec2>());
    return;
  case ezVariant::Type::Vector3:
    SetMaterialParameter(sName, value.Get<ezVec3>());
    return;
  case ezVariant::Type::Vector4:
    SetMaterialParameter(sName, value.Get<ezVec4>());
    return;
  //case ezVariant::Type::Matrix3: /// \todo fix this
  //  SetMaterialParameter(sName, value.Get<ezMat3>());
  //  return;
  case ezVariant::Type::Matrix4:
    SetMaterialParameter(sName, value.Get<ezMat4>());
    return;

  default:
    {
      ezLog::Debug("ezRenderContext::SetMaterialParameter: Variant contains invalid type of data");
    }
    return;
  }
}

const ezRenderContext::MaterialParam* ezRenderContext::GetMaterialParameterPointer(ezUInt32 uiNameHash)
{
  /// \todo Return array, not data pointer ? (only required, if array size may grow later on)

  return reinterpret_cast<MaterialParam*>(s_MaterialParameters[uiNameHash].GetData());
}
