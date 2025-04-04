#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <RendererFoundation/Shader/Types.h>

ezUInt32 ezShaderConstant::s_TypeSize[(ezUInt32)Type::ENUM_COUNT] = {0, sizeof(float) * 1, sizeof(float) * 2, sizeof(float) * 3, sizeof(float) * 4, sizeof(int) * 1, sizeof(int) * 2, sizeof(int) * 3, sizeof(int) * 4, sizeof(ezUInt32) * 1, sizeof(ezUInt32) * 2,
  sizeof(ezUInt32) * 3, sizeof(ezUInt32) * 4, sizeof(ezShaderMat3), sizeof(ezMat4), sizeof(ezShaderTransform), sizeof(ezShaderBool)};

void ezShaderConstant::CopyDataFromVariant(ezUInt8* pDest, const ezVariant* pValue) const
{
  EZ_ASSERT_DEV(m_uiArrayElements == 1, "Array constants are not supported");

  ezResult conversionResult = EZ_FAILURE;

  if (pValue != nullptr)
  {
    switch (m_Type)
    {
      case Type::Float1:
        *reinterpret_cast<float*>(pDest) = pValue->ConvertTo<float>(&conversionResult);
        break;
      case Type::Float2:
        *reinterpret_cast<ezVec2*>(pDest) = pValue->Get<ezVec2>();
        return;
      case Type::Float3:
        *reinterpret_cast<ezVec3*>(pDest) = pValue->Get<ezVec3>();
        return;
      case Type::Float4:
        if (pValue->GetType() == ezVariant::Type::Color || pValue->GetType() == ezVariant::Type::ColorGamma)
        {
          const ezColor tmp = pValue->ConvertTo<ezColor>();
          *reinterpret_cast<ezVec4*>(pDest) = *reinterpret_cast<const ezVec4*>(&tmp);
        }
        else
        {
          *reinterpret_cast<ezVec4*>(pDest) = pValue->Get<ezVec4>();
        }
        return;

      case Type::Int1:
        *reinterpret_cast<ezInt32*>(pDest) = pValue->ConvertTo<ezInt32>(&conversionResult);
        break;
      case Type::Int2:
        *reinterpret_cast<ezVec2I32*>(pDest) = pValue->Get<ezVec2I32>();
        return;
      case Type::Int3:
        *reinterpret_cast<ezVec3I32*>(pDest) = pValue->Get<ezVec3I32>();
        return;
      case Type::Int4:
        *reinterpret_cast<ezVec4I32*>(pDest) = pValue->Get<ezVec4I32>();
        return;

      case Type::UInt1:
        *reinterpret_cast<ezUInt32*>(pDest) = pValue->ConvertTo<ezUInt32>(&conversionResult);
        break;
      case Type::UInt2:
        *reinterpret_cast<ezVec2U32*>(pDest) = pValue->Get<ezVec2U32>();
        return;
      case Type::UInt3:
        *reinterpret_cast<ezVec3U32*>(pDest) = pValue->Get<ezVec3U32>();
        return;
      case Type::UInt4:
        *reinterpret_cast<ezVec4U32*>(pDest) = pValue->Get<ezVec4U32>();
        return;

      case Type::Mat3x3:
        *reinterpret_cast<ezShaderMat3*>(pDest) = pValue->Get<ezMat3>();
        return;
      case Type::Mat4x4:
        *reinterpret_cast<ezMat4*>(pDest) = pValue->Get<ezMat4>();
        return;
      case Type::Transform:
        *reinterpret_cast<ezShaderTransform*>(pDest) = pValue->Get<ezTransform>();
        return;

      case Type::Bool:
        *reinterpret_cast<ezShaderBool*>(pDest) = pValue->ConvertTo<bool>(&conversionResult);
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (conversionResult.Succeeded())
  {
    return;
  }

  // ezLog::Error("Constant '{0}' is not set, invalid or couldn't be converted to target type and will be set to zero.", m_sName);
  const ezUInt32 uiSize = s_TypeSize[m_Type];
  ezMemoryUtils::ZeroFill(pDest, uiSize);
}

ezResult ezShaderResourceBinding::CreateMergedShaderResourceBinding(const ezArrayPtr<ezArrayPtr<const ezShaderResourceBinding>>& resourcesPerStage, ezDynamicArray<ezShaderResourceBinding>& out_bindings, bool bAllowMultipleBindingPerName)
{
  ezUInt32 uiSize = 0;
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    uiSize += resourcesPerStage[stage].GetCount();
  }

  out_bindings.Clear();
  out_bindings.Reserve(uiSize);

  auto EqualBindings = [](const ezShaderResourceBinding& a, const ezShaderResourceBinding& b) -> bool
  {
    return a.m_sName == b.m_sName && a.m_ResourceType == b.m_ResourceType && a.m_TextureType == b.m_TextureType && a.m_uiArraySize == b.m_uiArraySize && a.m_iSet == b.m_iSet && a.m_iSlot == b.m_iSlot;
  };

  auto AddOrExtendBinding = [&](ezGALShaderStage::Enum stage, ezUInt32 uiStartIndex, const ezShaderResourceBinding& add)
  {
    for (ezUInt32 i = uiStartIndex + 1; i < out_bindings.GetCount(); i++)
    {
      if (EqualBindings(out_bindings[i], add))
      {
        out_bindings[i].m_Stages |= ezGALShaderStageFlags::MakeFromShaderStage(stage);
        return;
      }
    }
    ezShaderResourceBinding& newBinding = out_bindings.ExpandAndGetRef();
    newBinding = add;
    newBinding.m_Stages |= ezGALShaderStageFlags::MakeFromShaderStage(stage);
  };

  ezMap<ezHashedString, ezUInt32> nameToIndex;
  ezMap<ezHashedString, ezUInt32> samplerToIndex;
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (const ezShaderResourceBinding& res : resourcesPerStage[stage])
    {
      ezHashedString sName = res.m_sName;

      ezUInt32 uiIndex = ezInvalidIndex;
      if (res.m_ResourceType == ezGALShaderResourceType::Sampler)
      {
        // #TODO_SHADER Samplers are special! Since the shader compiler edits the reflection data and renames "*_AutoSampler" to just "*", we generate a naming collision between the texture and the sampler. See ezRenderContext::BindTexture2D for binding code. For now, we allow this collision, but it will probably bite us later on.
        samplerToIndex.TryGetValue(res.m_sName, uiIndex);
      }
      else
      {
        nameToIndex.TryGetValue(res.m_sName, uiIndex);
      }

      if (uiIndex != ezInvalidIndex)
      {
        ezShaderResourceBinding& current = out_bindings[uiIndex];
        if (!EqualBindings(current, res))
        {
          if (bAllowMultipleBindingPerName)
          {
            AddOrExtendBinding((ezGALShaderStage::Enum)stage, uiIndex, res);
            continue;
          }
          // #TODO_SHADER better error reporting.
          ezLog::Error("A shared shader resource '{}' has a mismatching signatures between stages", sName);
          return EZ_FAILURE;
        }

        current.m_Stages |= ezGALShaderStageFlags::MakeFromShaderStage((ezGALShaderStage::Enum)stage);
      }
      else
      {
        ezShaderResourceBinding& newBinding = out_bindings.ExpandAndGetRef();
        newBinding = res;
        newBinding.m_Stages |= ezGALShaderStageFlags::MakeFromShaderStage((ezGALShaderStage::Enum)stage);
        if (res.m_ResourceType == ezGALShaderResourceType::Sampler)
        {
          samplerToIndex[res.m_sName] = out_bindings.GetCount() - 1;
        }
        else
        {
          nameToIndex[res.m_sName] = out_bindings.GetCount() - 1;
        }
      }
    }
  }
  return EZ_SUCCESS;
}

ezGALShaderByteCode::ezGALShaderByteCode() = default;

ezGALShaderByteCode::~ezGALShaderByteCode()
{
  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_pLayout != nullptr)
    {
      ezShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      binding.m_pLayout = nullptr;

      if (pLayout->GetRefCount() == 0)
        EZ_DEFAULT_DELETE(pLayout);
    }
  }
}

bool ezShaderConstantBufferLayout::operator==(const ezShaderConstantBufferLayout& rhs) const
{
  if (m_uiTotalSize != rhs.m_uiTotalSize || m_Constants.GetCount() != rhs.m_Constants.GetCount())
    return false;

  const ezUInt32 uiCount = m_Constants.GetCount();
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const ezShaderConstant& a = m_Constants[i];
    const ezShaderConstant& b = rhs.m_Constants[i];

    // Some platforms return bool or uint1 for a bool type in a shader.
    ezEnum<ezShaderConstant::Type> aType = a.m_Type == ezShaderConstant::Type::Bool ? ezEnum<ezShaderConstant::Type>(ezShaderConstant::Type::UInt1) : a.m_Type;
    ezEnum<ezShaderConstant::Type> bType = b.m_Type == ezShaderConstant::Type::Bool ? ezEnum<ezShaderConstant::Type>(ezShaderConstant::Type::UInt1) : b.m_Type;

    if (a.m_sName != b.m_sName || aType != bType || a.m_uiArrayElements != b.m_uiArrayElements || a.m_uiOffset != b.m_uiOffset)
      return false;
  }
  return true;
}
