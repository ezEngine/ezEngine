#include <PCH.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerHLSL, 1, ezRTTIDefaultAllocator<ezShaderCompilerHLSL>);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(EZ_SHADERCOMPILERHLSL_DLL, ezShaderCompilerHLSLPlugin);

ezResult CompileDXShader(const char* szFile, const char* szSource, const char* szProfile, const char* szEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob = nullptr;

  if (FAILED(D3DCompile(szSource, strlen(szSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, 0, 0, &pResultBlob, &pErrorBlob)))
  {
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
    out_ByteCode.SetCountUninitialized((ezUInt32) pResultBlob->GetBufferSize());
    ezMemoryUtils::Copy(out_ByteCode.GetData(), static_cast<ezUInt8*>(pResultBlob->GetBufferPointer()), out_ByteCode.GetCount());
    pResultBlob->Release();
  }

  return EZ_SUCCESS;
}

void ezShaderCompilerHLSL::ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  auto byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();
  D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, (void**) &pReflector);

  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  for (ezUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    //ezLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);

    ezShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_Type = ezShaderResourceBinding::Unknown;
    shaderResourceBinding.m_iSlot = shaderInputBindDesc.BindPoint;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
    {
      switch (shaderInputBindDesc.Dimension)
      {
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture1D; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture1DArray; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2D; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2DArray; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2DMS; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2DMSArray; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture3D; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::TextureCube; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::TextureCubeArray; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::GenericBuffer; break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      switch (shaderInputBindDesc.Dimension)
      {
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture1D; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture1DArray; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture2D; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture2DArray; break;
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX:
        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWBuffer; break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
      shaderResourceBinding.m_Type = ezShaderResourceBinding::RWBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_Type = ezShaderResourceBinding::RWStructuredBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_Type = ezShaderResourceBinding::RWRawBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_Type = ezShaderResourceBinding::RWAppendBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_Type = ezShaderResourceBinding::RWConsumeBuffer;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_Type = ezShaderResourceBinding::RWStructuredBufferWithCounter;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_Type = ezShaderResourceBinding::ConstantBuffer;
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(inout_Data.m_StageBinary[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_Type = ezShaderResourceBinding::Sampler;
      if (ezStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        ezStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, ezStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_Type = ezShaderResourceBinding::GenericBuffer;
    }

    if (shaderResourceBinding.m_Type != ezShaderResourceBinding::Unknown)
    {
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

ezShaderConstantBufferLayout* ezShaderCompilerHLSL::ReflectConstantBufferLayout(ezShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  EZ_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  ezLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  ezShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (ezUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    EZ_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    ezShaderConstantBufferLayout::Constant constant;
    constant.m_uiArrayElements = ezMath::Max(std.Elements, 1u);
    constant.m_uiOffset = svd.StartOffset;
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
      case D3D_SVT_FLOAT:
        constant.m_Type = (ezShaderConstantBufferLayout::Constant::Type::Enum) ((ezInt32)ezShaderConstantBufferLayout::Constant::Type::Float1 + std.Columns - 1);
        break;
      case D3D_SVT_INT:
        constant.m_Type = (ezShaderConstantBufferLayout::Constant::Type::Enum) ((ezInt32)ezShaderConstantBufferLayout::Constant::Type::Int1 + std.Columns - 1);
        break;
      case D3D_SVT_UINT:
        constant.m_Type = (ezShaderConstantBufferLayout::Constant::Type::Enum) ((ezInt32)ezShaderConstantBufferLayout::Constant::Type::UInt1 + std.Columns - 1);
        break;
      case D3D_SVT_BOOL:
        if (std.Columns == 1)
        {
          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Bool;
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
        constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Mat4x4;
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
      continue;
    }

    if (constant.m_Type == ezShaderConstantBufferLayout::Constant::Type::Default)
    {
      ezLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
      continue;
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

const char* GetProfileName(const char* szPlatform, ezGALShaderStage::Enum Stage)
{
  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM40_93"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::VertexShader:    return "vs_4_0_level_9_3";
    case ezGALShaderStage::PixelShader:     return "ps_4_0_level_9_3";
    default: break;
    }
  }

  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM40"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::VertexShader:    return "vs_4_0";
    case ezGALShaderStage::GeometryShader:  return "gs_4_0";
    case ezGALShaderStage::PixelShader:     return "ps_4_0";
    case ezGALShaderStage::ComputeShader:   return "cs_4_0";
    default: break;
    }
  }

  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM41"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::GeometryShader:  return "gs_4_0";
    case ezGALShaderStage::VertexShader:    return "vs_4_1";
    case ezGALShaderStage::PixelShader:     return "ps_4_1";
    case ezGALShaderStage::ComputeShader:   return "cs_4_1";
    default: break;
    }
  }

  if (ezStringUtils::IsEqual(szPlatform, "DX11_SM50"))
  {
    switch (Stage)
    {
    case ezGALShaderStage::VertexShader:    return "vs_5_0";
    case ezGALShaderStage::HullShader:      return "hs_5_0";
    case ezGALShaderStage::DomainShader:    return "ds_5_0";
    case ezGALShaderStage::GeometryShader:  return "gs_5_0";
    case ezGALShaderStage::PixelShader:     return "ps_5_0";
    case ezGALShaderStage::ComputeShader:   return "cs_5_0";
    default: break;
    }
  }

  EZ_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", szPlatform, Stage);
  return "";
}

ezResult ezShaderCompilerHLSL::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // shader already compiled
    if (!inout_Data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      ezLog::Debug("Shader for stage '{0}' is already compiled.", ezGALShaderStage::Names[stage]);
      continue;
    }

    const char* szShaderSource = inout_Data.m_szShaderSource[stage];
    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szShaderSource);

    if (uiLength > 0 && ezStringUtils::FindSubString(szShaderSource, "main") != nullptr)
    {
      if (CompileDXShader(inout_Data.m_szSourceFile, szShaderSource, GetProfileName(inout_Data.m_szPlatform, (ezGALShaderStage::Enum) stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        ReflectShaderStage(inout_Data, (ezGALShaderStage::Enum) stage);
      }
      else
      {
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

