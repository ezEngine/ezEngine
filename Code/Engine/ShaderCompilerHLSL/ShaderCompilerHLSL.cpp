#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerHLSL, 1, ezRTTIDefaultAllocator<ezShaderCompilerHLSL>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult CompileDXShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, ezDynamicArray<ezUInt8>& out_byteCode)
{
  out_byteCode.Clear();

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob = nullptr;

  const char* szCompileSource = szSource;
  ezStringBuilder sDebugSource;
  UINT flags1 = 0;
  if (bDebug)
  {
    flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;
  }

  if (FAILED(D3DCompile(szCompileSource, strlen(szCompileSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)))
  {
    if (bDebug)
    {
      // Try again with '#line' intact to get correct error messages with file and line info.
      pErrorBlob->Release();
      pErrorBlob = nullptr;
      EZ_VERIFY(FAILED(D3DCompile(szSource, strlen(szSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)), "Debug compilation with commented out '#line' failed but original version did not.");
    }

    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    EZ_LOG_BLOCK("Shader Compilation Failed", szFile);

    ezLog::Error("Could not compile shader '{0}' for profile '{1}'", szFile, szProfile);
    ezLog::Error("{0}", szError);

    pErrorBlob->Release();
    return EZ_FAILURE;
  }

  if (pErrorBlob != nullptr)
  {
    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    EZ_LOG_BLOCK("Shader Compilation Error Message", szFile);
    ezLog::Dev("{0}", szError);

    pErrorBlob->Release();
  }

  if (pResultBlob != nullptr)
  {
    out_byteCode.SetCountUninitialized((ezUInt32)pResultBlob->GetBufferSize());
    ezMemoryUtils::Copy(out_byteCode.GetData(), static_cast<ezUInt8*>(pResultBlob->GetBufferPointer()), out_byteCode.GetCount());
    pResultBlob->Release();
  }

  return EZ_SUCCESS;
}

void ezShaderCompilerHLSL::ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  ezGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  D3DReflect(pShader->m_ByteCode.GetData(), pShader->m_ByteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);


  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  if (Stage == ezGALShaderStage::VertexShader)
  {
    auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
    vertexInputAttributes.Reserve(shaderDesc.InputParameters);
    for (ezUInt32 i = 0; i < shaderDesc.InputParameters; ++i)
    {
      D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
      pReflector->GetInputParameterDesc(i, &paramDesc);

      ezGALVertexAttributeSemantic::Enum semantic;
      if (!m_VertexInputMapping.TryGetValue(paramDesc.SemanticName, semantic))
      {
        // We ignore all system-value semantics as they are not provided by the user but the system so we don't care to reflect them.
        if (ezStringUtils::StartsWith_NoCase(paramDesc.SemanticName, "SV_"))
          continue;

        EZ_ASSERT_NOT_IMPLEMENTED;
      }
      switch (semantic)
      {
        case ezGALVertexAttributeSemantic::Color0:
          EZ_ASSERT_DEBUG(paramDesc.SemanticIndex <= 7, "Color out of range");
          semantic = static_cast<ezGALVertexAttributeSemantic::Enum>((ezUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case ezGALVertexAttributeSemantic::TexCoord0:
          EZ_ASSERT_DEBUG(paramDesc.SemanticIndex <= 9, "TexCoord out of range");
          semantic = static_cast<ezGALVertexAttributeSemantic::Enum>((ezUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case ezGALVertexAttributeSemantic::BoneIndices0:
          EZ_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneIndices out of range");
          semantic = static_cast<ezGALVertexAttributeSemantic::Enum>((ezUInt32)semantic + paramDesc.SemanticIndex);
          break;
        case ezGALVertexAttributeSemantic::BoneWeights0:
          EZ_ASSERT_DEBUG(paramDesc.SemanticIndex <= 1, "BoneWeights out of range");
          semantic = static_cast<ezGALVertexAttributeSemantic::Enum>((ezUInt32)semantic + paramDesc.SemanticIndex);
          break;
        default:
          break;
      }

      ezShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
      attr.m_eSemantic = semantic;
      attr.m_eFormat = GetEZFormat(paramDesc);
      attr.m_uiLocation = paramDesc.Register;
    }
  }
  else if (Stage == ezGALShaderStage::HullShader)
  {
    pShader->m_uiTessellationPatchControlPoints = shaderDesc.cControlPoints;
  }

  for (ezUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    // ezLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);
    // #TODO_SHADER remove [x] at the end of the name for arrays
    ezShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_iSet = 0;
    shaderResourceBinding.m_iSlot = static_cast<ezInt16>(shaderInputBindDesc.BindPoint);
    shaderResourceBinding.m_uiArraySize = shaderInputBindDesc.BindCount;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);
    shaderResourceBinding.m_Stages = ezGALShaderStageFlags::MakeFromShaderStage(Stage);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE || shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      shaderResourceBinding.m_ResourceType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? ezGALShaderResourceType::Texture : ezGALShaderResourceType::TextureRW;
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture1D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture1DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture2D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture2DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture2DMS;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture2DMSArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Texture3D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::TextureCube;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::TextureCubeArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
          shaderResourceBinding.m_ResourceType = shaderInputBindDesc.Type == D3D_SIT_TEXTURE ? ezGALShaderResourceType::TexelBuffer : ezGALShaderResourceType::TexelBufferRW;
          shaderResourceBinding.m_TextureType = ezGALShaderTextureType::Unknown;
          break;

        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_STRUCTURED)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::StructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_BYTEADDRESS)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::ByteAddressBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::ByteAddressBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::ConstantBuffer;
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(*inout_Data.m_ByteCode[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::Sampler;
      if (ezStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        ezStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, ezStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::Enum::Unknown;
    }

    if (shaderResourceBinding.m_ResourceType != ezGALShaderResourceType::Unknown)
    {
      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

ezSharedPtr<ezShaderConstantBufferLayout> ezShaderCompilerHLSL::ReflectConstantBufferLayout(ezGALShaderByteCode& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  EZ_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  ezLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  ezSharedPtr<ezShaderConstantBufferLayout> pLayout = EZ_DEFAULT_NEW(ezShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (ezUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    EZ_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    ezShaderConstant constant;
    constant.m_uiArrayElements = static_cast<ezUInt8>(ezMath::Max(std.Elements, 1u));
    constant.m_uiOffset = static_cast<ezUInt16>(svd.StartOffset);
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (ezShaderConstant::Type::Enum)((ezInt32)ezShaderConstant::Type::Float1 + std.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (ezShaderConstant::Type::Enum)((ezInt32)ezShaderConstant::Type::Int1 + std.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (ezShaderConstant::Type::Enum)((ezInt32)ezShaderConstant::Type::UInt1 + std.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (std.Columns == 1)
          {
            constant.m_Type = ezShaderConstant::Type::Bool;
          }
          break;

        default:
          break;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
    {
      if (std.Type != D3D_SVT_FLOAT)
      {
        ezLog::Error("Variable '{0}': Only float matrices are supported", svd.Name);
        continue;
      }

      if (std.Columns == 3 && std.Rows == 3)
      {
        constant.m_Type = ezShaderConstant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = ezShaderConstant::Type::Mat4x4;
      }
      else
      {
        ezLog::Error("Variable '{0}': {1}x{2} matrices are not supported", svd.Name, std.Rows, std.Columns);
        continue;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_ROWS)
    {
      ezLog::Error("Variable '{0}': Row-Major matrices are not supported", svd.Name);
      continue;
    }
    else if (std.Class == D3D_SVC_STRUCT)
    {
      constant.m_Type = ezShaderConstant::Type::Struct;
      if (svd.Size == 48 && std.Members == 3)
      {
        ezStringView sMember0 = pVar->GetType()->GetMemberTypeName(0);
        ezStringView sMember1 = pVar->GetType()->GetMemberTypeName(1);
        ezStringView sMember2 = pVar->GetType()->GetMemberTypeName(2);
        if (sMember0 == "r0" && sMember1 == "r1" && sMember2 == "r2")
        {
          constant.m_Type = ezShaderConstant::Type::Transform;
        }
      }
    }

    if (constant.m_Type == ezShaderConstant::Type::Default)
    {
      ezLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
      continue;
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

const char* GetProfileName(ezStringView sPlatform, ezGALShaderStage::Enum stage)
{
  if (sPlatform == "DX11_SM40_93")
  {
    switch (stage)
    {
      case ezGALShaderStage::VertexShader:
        return "vs_4_0_level_9_3";
      case ezGALShaderStage::PixelShader:
        return "ps_4_0_level_9_3";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM40")
  {
    switch (stage)
    {
      case ezGALShaderStage::VertexShader:
        return "vs_4_0";
      case ezGALShaderStage::GeometryShader:
        return "gs_4_0";
      case ezGALShaderStage::PixelShader:
        return "ps_4_0";
      case ezGALShaderStage::ComputeShader:
        return "cs_4_0";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM41")
  {
    switch (stage)
    {
      case ezGALShaderStage::GeometryShader:
        return "gs_4_0";
      case ezGALShaderStage::VertexShader:
        return "vs_4_1";
      case ezGALShaderStage::PixelShader:
        return "ps_4_1";
      case ezGALShaderStage::ComputeShader:
        return "cs_4_1";
      default:
        break;
    }
  }

  if (sPlatform == "DX11_SM50")
  {
    switch (stage)
    {
      case ezGALShaderStage::VertexShader:
        return "vs_5_0";
      case ezGALShaderStage::HullShader:
        return "hs_5_0";
      case ezGALShaderStage::DomainShader:
        return "ds_5_0";
      case ezGALShaderStage::GeometryShader:
        return "gs_5_0";
      case ezGALShaderStage::PixelShader:
        return "ps_5_0";
      case ezGALShaderStage::ComputeShader:
        return "cs_5_0";
      default:
        break;
    }
  }

  EZ_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", sPlatform, stage);
  return "";
}

ezResult ezShaderCompilerHLSL::ModifyShaderSource(ezShaderProgramData& inout_data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    ezShaderParser::ParseShaderResources(inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage]);
  }

  ezHashTable<ezHashedString, ezShaderResourceBinding> bindings;
  EZ_SUCCEED_OR_RETURN(ezShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  EZ_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));

  for (auto it : bindings)
  {
    if (it.Value().m_ResourceType == ezGALShaderResourceType::ConstantBuffer && it.Value().m_iSlot >= EZ_GAL_MAX_CONSTANT_BUFFER_COUNT)
    {
      ezLog::Error(pLog, "Shader constant buffer resource '{}' has slot index {}. EZ only supports up to {} slots.", it.Key(), it.Value().m_iSlot, EZ_GAL_MAX_CONSTANT_BUFFER_COUNT);
      return EZ_FAILURE;
    }
    if (it.Value().m_ResourceType == ezGALShaderResourceType::Sampler && it.Value().m_iSlot >= EZ_GAL_MAX_SAMPLER_COUNT)
    {
      ezLog::Error(pLog, "Shader sampler resource '{}' has slot index {}. EZ only supports up to {} slots.", it.Key(), it.Value().m_iSlot, EZ_GAL_MAX_SAMPLER_COUNT);
      return EZ_FAILURE;
    }
  }

  // Apply shader resource bindings
  ezStringBuilder sNewShaderCode;
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;
    ezShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage], bindings, ezMakeDelegate(&ezShaderCompilerHLSL::CreateNewShaderResourceDeclaration, this), sNewShaderCode);
    inout_data.m_sShaderSource[stage] = sNewShaderCode;
    inout_data.m_Resources[stage].Clear();
  }
  return EZ_SUCCESS;
}

ezResult ezShaderCompilerHLSL::Compile(ezShaderProgramData& inout_data, ezLogInterface* pLog)
{
  Initialize();
  ezStringBuilder sFile, sSource;

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Shader stage not used.
    if (inout_data.m_uiSourceHash[stage] == 0)
      continue;

    // Shader already compiled.
    if (inout_data.m_bWriteToDisk[stage] == false)
    {
      ezLog::Debug("Shader for stage '{0}' is already compiled.", ezGALShaderStage::Names[stage]);
      continue;
    }

    ezStringView sShaderSource = inout_data.m_sShaderSource[stage];
    const ezUInt32 uiLength = sShaderSource.GetElementCount();

    if (uiLength > 0 && sShaderSource.FindSubString("main") != nullptr)
    {
      if (CompileDXShader(inout_data.m_sSourceFile.GetData(sFile), sShaderSource.GetData(sSource), inout_data.m_Flags.IsSet(ezShaderCompilerFlags::Debug), GetProfileName(inout_data.m_sPlatform, (ezGALShaderStage::Enum)stage), "main", inout_data.m_ByteCode[stage]->m_ByteCode).Succeeded())
      {
        ReflectShaderStage(inout_data, (ezGALShaderStage::Enum)stage);
      }
      else
      {
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

namespace
{
  struct DX11ResourceCategory
  {
    using StorageType = ezUInt8;
    static constexpr int ENUM_COUNT = 4;
    enum Enum : ezUInt8
    {
      Sampler = EZ_BIT(0),
      ConstantBuffer = EZ_BIT(1),
      SRV = EZ_BIT(2),
      UAV = EZ_BIT(3),
      Default = 0
    };

    struct Bits
    {
      StorageType Sampler : 1;
      StorageType ConstantBuffer : 1;
      StorageType SRV : 1;
      StorageType UAV : 1;
    };

    static ezBitflags<DX11ResourceCategory> MakeFromShaderDescriptorType(ezGALShaderResourceType::Enum type);
  };

  EZ_DECLARE_FLAGS_OPERATORS(DX11ResourceCategory);
} // namespace

inline ezBitflags<DX11ResourceCategory> DX11ResourceCategory::MakeFromShaderDescriptorType(ezGALShaderResourceType::Enum type)
{
  switch (type)
  {
    case ezGALShaderResourceType::Sampler:
      return DX11ResourceCategory::Sampler;
    case ezGALShaderResourceType::ConstantBuffer:
    case ezGALShaderResourceType::PushConstants:
      return DX11ResourceCategory::ConstantBuffer;
    case ezGALShaderResourceType::Texture:
    case ezGALShaderResourceType::TexelBuffer:
    case ezGALShaderResourceType::StructuredBuffer:
    case ezGALShaderResourceType::ByteAddressBuffer:
      return DX11ResourceCategory::SRV;
    case ezGALShaderResourceType::TextureRW:
    case ezGALShaderResourceType::TexelBufferRW:
    case ezGALShaderResourceType::StructuredBufferRW:
    case ezGALShaderResourceType::ByteAddressBufferRW:
      return DX11ResourceCategory::UAV;
    case ezGALShaderResourceType::TextureAndSampler:
      return DX11ResourceCategory::SRV | DX11ResourceCategory::Sampler;
    default:
      EZ_REPORT_FAILURE("Missing enum");
      return {};
  }
}

ezResult ezShaderCompilerHLSL::DefineShaderResourceBindings(const ezShaderProgramData& data, ezHashTable<ezHashedString, ezShaderResourceBinding>& inout_resourceBinding, ezLogInterface* pLog)
{
  ezHybridBitfield<64> indexInUse[DX11ResourceCategory::ENUM_COUNT];
  for (auto it : inout_resourceBinding)
  {
    const ezBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(it.Value().m_ResourceType);
    // Convert bit to index. We know that only one bit can be set in DX11 as TextureAndSampler is not supported.
    const ezUInt32 uiIndex = ezMath::FirstBitLow((ezUInt32)type.GetValue());
    const ezInt16 iSlot = it.Value().m_iSlot;
    if (iSlot != -1)
    {
      indexInUse[uiIndex].SetCount(ezMath::Max(indexInUse[uiIndex].GetCount(), static_cast<ezUInt32>(iSlot + 1)));
      indexInUse[uiIndex].SetBit(iSlot);
    }
    // DX11: Everything is set 0.
    it.Value().m_iSet = 0;
  }

  // Create stable order of resources
  ezHybridArray<ezHashedString, 16> order[DX11ResourceCategory::ENUM_COUNT];
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : data.m_Resources[stage])
    {
      const ezBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(res.m_Binding.m_ResourceType);
      const ezUInt32 uiIndex = ezMath::FirstBitLow((ezUInt32)type.GetValue());
      if (!order[uiIndex].Contains(res.m_Binding.m_sName))
      {
        order[uiIndex].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  // EZ: We only allow constant buffers to be bound globally, so they must all have unique indices.
  // DX11: UAV are bound globally
  // DX11: SRV, Samplers can be bound by stage, so indices can be re-used. Thus, we don't set an index for any of them and let the compiler choose.
  for (auto type : ezBitflags<DX11ResourceCategory>(DX11ResourceCategory::UAV | DX11ResourceCategory::ConstantBuffer))
  {
    const ezUInt32 uiIndex = ezMath::FirstBitLow((ezUInt32)type);
    ezUInt32 uiCurrentIndex = 0;
    // Workaround for this: error X4509: UAV registers live in the same name space as outputs, so they must be bound to at least u1, manual bind to slot u0 failed
    if (type == DX11ResourceCategory::UAV)
      uiCurrentIndex = 1;

    for (const auto& sName : order[uiIndex])
    {
      while (uiCurrentIndex < indexInUse[uiIndex].GetCount() && indexInUse[uiIndex].IsBitSet(uiCurrentIndex))
      {
        uiCurrentIndex++;
      }
      inout_resourceBinding[sName].m_iSlot = static_cast<ezInt16>(uiCurrentIndex);
      indexInUse[uiIndex].SetCount(ezMath::Max(indexInUse[uiIndex].GetCount(), uiCurrentIndex + 1));
      indexInUse[uiIndex].SetBit(uiCurrentIndex);
    }
  }

  return EZ_SUCCESS;
}

void ezShaderCompilerHLSL::CreateNewShaderResourceDeclaration(ezStringView sPlatform, ezStringView sDeclaration, const ezShaderResourceBinding& binding, ezStringBuilder& out_sDeclaration)
{
  EZ_ASSERT_DEBUG(binding.m_iSet == 0, "HLSL: error X3721: space is only supported for shader targets 5.1 and higher");
  const ezBitflags<DX11ResourceCategory> type = DX11ResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);
  ezStringView sResourcePrefix;
  if (binding.m_iSlot == -1)
  {
    // Let the compiler choose an index.
    out_sDeclaration.SetFormat("{}", sDeclaration);
    return;
  }

  switch (type.GetValue())
  {
    case DX11ResourceCategory::Sampler:
      sResourcePrefix = "s"_ezsv;
      break;
    case DX11ResourceCategory::ConstantBuffer:
      sResourcePrefix = "b"_ezsv;
      break;
    case DX11ResourceCategory::SRV:
      sResourcePrefix = "t"_ezsv;
      break;
    case DX11ResourceCategory::UAV:
      sResourcePrefix = "u"_ezsv;
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }
  out_sDeclaration.SetFormat("{} : register({}{})", sDeclaration, sResourcePrefix, binding.m_iSlot);
}

void ezShaderCompilerHLSL::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["POSITION"] = ezGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["NORMAL"] = ezGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["TANGENT"] = ezGALVertexAttributeSemantic::Tangent;
    m_VertexInputMapping["COLOR"] = ezGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["TEXCOORD"] = ezGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["BITANGENT"] = ezGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["BONEINDICES"] = ezGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["BONEWEIGHTS"] = ezGALVertexAttributeSemantic::BoneWeights0;
  }
}

ezGALResourceFormat::Enum ezShaderCompilerHLSL::GetEZFormat(const _D3D11_SIGNATURE_PARAMETER_DESC& paramDesc)
{
  ezUInt32 uiComponents = ezMath::Log2i(paramDesc.Mask + 1);
  switch (paramDesc.ComponentType)
  {
    case D3D_REGISTER_COMPONENT_UNKNOWN:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
    case D3D_REGISTER_COMPONENT_UINT32:
      switch (uiComponents)
      {
        case 1:
          return ezGALResourceFormat::RUInt;
        case 2:
          return ezGALResourceFormat::RGUInt;
        case 3:
          return ezGALResourceFormat::RGBUInt;
        case 4:
          return ezGALResourceFormat::RGBAUInt;
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    case D3D_REGISTER_COMPONENT_SINT32:
      switch (uiComponents)
      {
        case 1:
          return ezGALResourceFormat::RInt;
        case 2:
          return ezGALResourceFormat::RGInt;
        case 3:
          return ezGALResourceFormat::RGBInt;
        case 4:
          return ezGALResourceFormat::RGBAInt;
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
      switch (uiComponents)
      {
        case 1:
          return ezGALResourceFormat::RFloat;
        case 2:
          return ezGALResourceFormat::RGFloat;
        case 3:
          return ezGALResourceFormat::RGBFloat;
        case 4:
          return ezGALResourceFormat::RGBAFloat;
        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          break;
      }
      break;
    default:
      return ezGALResourceFormat::Invalid;
  }
  return ezGALResourceFormat::Invalid;
}
