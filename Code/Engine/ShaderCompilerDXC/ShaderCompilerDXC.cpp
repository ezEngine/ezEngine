#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

// atlbase.h won't compile with NULL being nullptr
#ifdef NULL
#  undef NULL
#endif
#define NULL 0

#include <atlbase.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerDXC, 1, ezRTTIDefaultAllocator<ezShaderCompilerDXC>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPlugin g_Plugin(false);

CComPtr<IDxcUtils> s_pDxcUtils;
CComPtr<IDxcCompiler3> s_pDxcCompiler;

static ezResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode);

static const char* GetProfileName(const char* szPlatform, ezGALShaderStage::Enum Stage)
{
  if (ezStringUtils::IsEqual(szPlatform, "VULKAN"))
  {
    switch (Stage)
    {
      case ezGALShaderStage::VertexShader:
        return "vs_6_0";
      case ezGALShaderStage::HullShader:
        return "hs_6_0";
      case ezGALShaderStage::DomainShader:
        return "ds_6_0";
      case ezGALShaderStage::GeometryShader:
        return "gs_6_0";
      case ezGALShaderStage::PixelShader:
        return "ps_6_0";
      case ezGALShaderStage::ComputeShader:
        return "cs_6_0";
      default:
        break;
    }
  }

  EZ_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", szPlatform, Stage);
  return "";
}

ezResult ezShaderCompilerDXC::Initialize()
{
  if (s_pDxcUtils != nullptr)
    return EZ_SUCCESS;

  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_pDxcUtils));
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_pDxcCompiler));

  return EZ_SUCCESS;
}

ezResult ezShaderCompilerDXC::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  EZ_SUCCEED_OR_RETURN(Initialize());

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (!inout_Data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      ezLog::Debug("Shader for stage '{0}' is already compiled.", ezGALShaderStage::Names[stage]);
      continue;
    }

    const char* szShaderSource = inout_Data.m_szShaderSource[stage];
    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szShaderSource);

    if (uiLength > 0 && ezStringUtils::FindSubString(szShaderSource, "main") != nullptr)
    {
      if (CompileVulkanShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(ezShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (ezGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        ReflectShaderStage(inout_Data, (ezGALShaderStage::Enum)stage);
      }
      else
      {
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  const char* szCompileSource = szSource;
  ezStringBuilder sDebugSource;

  ezDynamicArray<ezStringWChar> args;
  args.PushBack(ezStringWChar(szFile));
  args.PushBack(L"-E");
  args.PushBack(ezStringWChar(szEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(ezStringWChar(szProfile));
  args.PushBack(L"-Qstrip_reflect"); // Strip reflection into a separate blob.
  args.PushBack(L"-spirv");

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;

    ezLog::Warning("Vulkan DEBUG shader support not really implemented.");

    // args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  CComPtr<IDxcBlobEncoding> pSource = nullptr;
  s_pDxcUtils->CreateBlob(szCompileSource, (UINT32)strlen(szCompileSource), DXC_CP_UTF8, &pSource);

  DxcBuffer Source;
  Source.Ptr = pSource->GetBufferPointer();
  Source.Size = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  ezHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (ezUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  CComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(&pResults));

  CComPtr<IDxcBlobUtf8> pErrors = nullptr;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    ezLog::Error("Vulkan shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      ezLog::Error("{}", ezStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return EZ_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      ezLog::Warning("{}", ezStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  CComPtr<IDxcBlob> pShader = nullptr;
  CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), &pShaderName);

  if (pShader == nullptr)
  {
    ezLog::Error("No Vulkan bytecode was generated.");
    return EZ_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<ezUInt32>(pShader->GetBufferSize()));

  ezMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<ezUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return EZ_SUCCESS;
}

void ezShaderCompilerDXC::ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage)
{
  // ID3D11ShaderReflection* pReflector = nullptr;

  // auto byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();
  // D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);

  // D3D11_SHADER_DESC shaderDesc;
  // pReflector->GetDesc(&shaderDesc);

  // for (ezUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  //{
  //  D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
  //  pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

  //  // ezLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);

  //  ezShaderResourceBinding shaderResourceBinding;
  //  shaderResourceBinding.m_Type = ezShaderResourceBinding::Unknown;
  //  shaderResourceBinding.m_iSlot = shaderInputBindDesc.BindPoint;
  //  shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);

  //  if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
  //  {
  //    switch (shaderInputBindDesc.Dimension)
  //    {
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture1D;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture1DArray;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2D;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2DArray;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2DMS;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture2DMSArray;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::Texture3D;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::TextureCube;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::TextureCubeArray;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::GenericBuffer;
  //        break;

  //      default:
  //        EZ_ASSERT_NOT_IMPLEMENTED;
  //        break;
  //    }
  //  }

  //  else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
  //  {
  //    switch (shaderInputBindDesc.Dimension)
  //    {
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture1D;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture1DArray;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture2D;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWTexture2DArray;
  //        break;
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
  //      case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX:
  //        shaderResourceBinding.m_Type = ezShaderResourceBinding::RWBuffer;
  //        break;

  //      default:
  //        EZ_ASSERT_NOT_IMPLEMENTED;
  //        break;
  //    }
  //  }

  //  else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::RWStructuredBuffer;

  //  else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::RWRawBuffer;

  //  else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::RWAppendBuffer;

  //  else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::RWConsumeBuffer;

  //  else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::RWStructuredBufferWithCounter;

  //  else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
  //  {
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::ConstantBuffer;
  //    shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(inout_Data.m_StageBinary[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
  //  }
  //  else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
  //  {
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::Sampler;
  //    if (ezStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
  //    {
  //      ezStringBuilder sb = shaderInputBindDesc.Name;
  //      sb.Shrink(0, ezStringUtils::GetStringElementCount("_AutoSampler"));
  //      shaderResourceBinding.m_sName.Assign(sb.GetData());
  //    }
  //  }
  //  else
  //  {
  //    shaderResourceBinding.m_Type = ezShaderResourceBinding::GenericBuffer;
  //  }

  //  if (shaderResourceBinding.m_Type != ezShaderResourceBinding::Unknown)
  //  {
  //    inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
  //  }
  //}

  // pReflector->Release();
}

// ezShaderConstantBufferLayout* ezShaderCompilerDXC::ReflectConstantBufferLayout(ezShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
//{
// D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

// if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
//{
//  return nullptr;
//}

// EZ_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
// ezLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

// ezShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

// pLayout->m_uiTotalSize = shaderBufferDesc.Size;

// for (ezUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
//{
//  ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

//  D3D11_SHADER_VARIABLE_DESC svd;
//  pVar->GetDesc(&svd);

//  EZ_LOG_BLOCK("Constant", svd.Name);

//  D3D11_SHADER_TYPE_DESC std;
//  pVar->GetType()->GetDesc(&std);

//  ezShaderConstantBufferLayout::Constant constant;
//  constant.m_uiArrayElements = static_cast<ezUInt8>(ezMath::Max(std.Elements, 1u));
//  constant.m_uiOffset = svd.StartOffset;
//  constant.m_sName.Assign(svd.Name);

//  if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
//  {
//    switch (std.Type)
//    {
//      case D3D_SVT_FLOAT:
//        constant.m_Type = (ezShaderConstantBufferLayout::Constant::Type::Enum)((ezInt32)ezShaderConstantBufferLayout::Constant::Type::Float1 + std.Columns - 1);
//        break;
//      case D3D_SVT_INT:
//        constant.m_Type = (ezShaderConstantBufferLayout::Constant::Type::Enum)((ezInt32)ezShaderConstantBufferLayout::Constant::Type::Int1 + std.Columns - 1);
//        break;
//      case D3D_SVT_UINT:
//        constant.m_Type = (ezShaderConstantBufferLayout::Constant::Type::Enum)((ezInt32)ezShaderConstantBufferLayout::Constant::Type::UInt1 + std.Columns - 1);
//        break;
//      case D3D_SVT_BOOL:
//        if (std.Columns == 1)
//        {
//          constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Bool;
//        }
//        break;

//      default:
//        break;
//    }
//  }
//  else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
//  {
//    if (std.Type != D3D_SVT_FLOAT)
//    {
//      ezLog::Error("Variable '{0}': Only float matrices are supported", svd.Name);
//      continue;
//    }

//    if (std.Columns == 3 && std.Rows == 3)
//    {
//      constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Mat3x3;
//    }
//    else if (std.Columns == 4 && std.Rows == 4)
//    {
//      constant.m_Type = ezShaderConstantBufferLayout::Constant::Type::Mat4x4;
//    }
//    else
//    {
//      ezLog::Error("Variable '{0}': {1}x{2} matrices are not supported", svd.Name, std.Rows, std.Columns);
//      continue;
//    }
//  }
//  else if (std.Class == D3D_SVC_MATRIX_ROWS)
//  {
//    ezLog::Error("Variable '{0}': Row-Major matrices are not supported", svd.Name);
//    continue;
//  }
//  else if (std.Class == D3D_SVC_STRUCT)
//  {
//    continue;
//  }

//  if (constant.m_Type == ezShaderConstantBufferLayout::Constant::Type::Default)
//  {
//    ezLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
//    continue;
//  }

//  pLayout->m_Constants.PushBack(constant);
//}

// return pLayout;
//}
