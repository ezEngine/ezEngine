#include <RendererCore/PCH.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <RendererCore/Shader/ShaderStageBinary.h>

ezUInt64 ezRendererCore::s_LastMaterialParamModification = 1;
static ezMap<ezUInt32, ezDynamicArray<ezUInt8> > s_MaterialParameters;

ezUInt32 ezShaderStageBinary::MaterialParameter::s_TypeSize[(ezUInt32) Type::ENUM_COUNT] =
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

ezRendererCore::MaterialParam* ezRendererCore::InternalSetMaterialParameter(const ezTempHashedString& sName, ezShaderStageBinary::MaterialParameter::Type type, ezUInt32 uiMaxArrayElements)
{
  auto& p = s_MaterialParameters[sName.GetHash()];

  MaterialParam* pData = reinterpret_cast<MaterialParam*>(p.GetData());

  if (p.IsEmpty())
  {
    ++s_LastMaterialParamModification;

    p.SetCount(sizeof(MaterialParam) + ezShaderStageBinary::MaterialParameter::s_TypeSize[(ezUInt32) type]);

    pData = reinterpret_cast<MaterialParam*>(p.GetData());

    pData->m_Type = type;
    pData->m_LastModification = s_LastMaterialParamModification;
    pData->m_uiArrayElements = uiMaxArrayElements;
    pData->m_uiDataSize = ezShaderStageBinary::MaterialParameter::s_TypeSize[(ezUInt32) pData->m_Type] * pData->m_uiArrayElements;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pData->m_Type != type)
  {
    ezLog::Error("ezRendererCore::SetMaterialParameter(0x%08X): Parameter is used with different types (%u and %u)", (ezUInt32) pData->m_Type, (ezUInt32) type);
    return nullptr;
  }
#endif

  return pData;
}

#define SetMaterialParameterX(TypeType, TypeEnum, ArrayMax) \
  void ezRendererCore::SetMaterialParameter(const ezTempHashedString& sName, const TypeType& value) \
  { \
    MaterialParam* pParam = InternalSetMaterialParameter(sName, ezShaderStageBinary::MaterialParameter::Type::TypeEnum, ArrayMax); \
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
SetMaterialParameterX(ezMat3, Mat3x3, 1);
SetMaterialParameterX(ezMat4, Mat4x4, 1);
SetMaterialParameterX(ezTransform, Mat3x4, 1);

const ezRendererCore::MaterialParam* ezRendererCore::GetMaterialParameterPointer(ezUInt32 uiNameHash)
{
  /// \todo Return array, not data pointer ? (only required, if array size may grow later on)

  return reinterpret_cast<MaterialParam*>(s_MaterialParameters[uiNameHash].GetData());
}
