#include <PCH.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerHLSL, ezShaderProgramCompiler, 1, ezRTTIDefaultAllocator<ezShaderCompilerHLSL>);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

void OnLoadPlugin(bool bReloading) { }
void OnUnloadPlugin(bool bReloading) { }

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);

EZ_DYNAMIC_PLUGIN_IMPLEMENTATION(ezShaderCompilerHLSLPlugin);

ezResult CompileDXShader(const char* szFile, const char* source, const char* profile, const char* entryPoint, ezDynamicArray<ezUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  ID3DBlob* ResultBlob = nullptr;
  ID3DBlob* ErrorBlob = nullptr;

  if (FAILED(D3DCompile(source, strlen(source), szFile, nullptr, nullptr, entryPoint, profile, 0, 0, &ResultBlob, &ErrorBlob)))
  {
    const char* szError = (const char*) ErrorBlob->GetBufferPointer();

    EZ_LOG_BLOCK("Shader Compilation Failed", szFile);

    ezLog::Error("Could not compile shader '%s' for profile '%s'", szFile, profile);
    ezLog::Error("%s", szError);

    ErrorBlob->Release();
    return EZ_FAILURE;
  }

  if (ErrorBlob != nullptr)
  {
    const char* szError = (const char*) ErrorBlob->GetBufferPointer();

    EZ_LOG_BLOCK("Shader Compilation Error Message", szFile);
    ezLog::Dev("%s", szError);

    ErrorBlob->Release();
  }

  if (ResultBlob != nullptr)
  {
    out_ByteCode.SetCount((ezUInt32) ResultBlob->GetBufferSize());
    ezMemoryUtils::Copy(out_ByteCode.GetData(), (ezUInt8*) ResultBlob->GetBufferPointer(), out_ByteCode.GetCount());
    ResultBlob->Release();
  }

  return EZ_SUCCESS;
}

void ezShaderCompilerHLSL::ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  D3DReflect(inout_Data.m_StageBinary[Stage].m_ByteCode.GetData(), inout_Data.m_StageBinary[Stage].m_ByteCode.GetCount(), IID_ID3D11ShaderReflection, (void**) &pReflector);

  D3D11_SHADER_DESC sd;
  pReflector->GetDesc(&sd);

  //ezLog::Info("Bound Resources: %u", sd.BoundResources);
  //ezLog::Info("Num Constant Buffers: %u", sd.ConstantBuffers);
  //ezLog::Info("Num Inputs: %u", sd.InputParameters);
  //ezLog::Info("Num Outputs: %u", sd.OutputParameters);

  for (ezUInt32 r = 0; r < sd.BoundResources; ++r)
  {
    ezShaderStageResource ssr;

    D3D11_SHADER_INPUT_BIND_DESC sibd;
    pReflector->GetResourceBindingDesc(r, &sibd);

    //ezLog::Info("Bound Resource: '%s' at slot %u (Count: %u, Flags: %u)", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);

    ssr.m_Name.Assign(sibd.Name);
    ssr.m_iSlot = sibd.BindPoint;


    //if (sibd.Type == D3D_SIT_TEXTURE || sibd.Type == D3D_SIT_TBUFFER)
    //{
    //  
    //  switch (sibd.Dimension)
    //  {
    //  //case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER: ezLog::Info("Resource is a Buffer"); break;
    //  //case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX: ezLog::Info("Resource is a Buffer Ex"); break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D: ezLog::Info("Resource is a 1D Texture"); ssr.m_Type = ezShaderStageResource::Texture1D; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY: ezLog::Info("Resource is a 1D Texture Array"); ssr.m_Type = ezShaderStageResource::Texture1DArray; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D: ezLog::Info("Resource is a 2D Texture"); ssr.m_Type = ezShaderStageResource::Texture2D; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY: ezLog::Info("Resource is a 2D Texture Array"); ssr.m_Type = ezShaderStageResource::Texture2DArray; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS: ezLog::Info("Resource is a 2D Texture MS"); ssr.m_Type = ezShaderStageResource::Texture2DMS; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY: ezLog::Info("Resource is a 2D Texture MS Array"); ssr.m_Type = ezShaderStageResource::Texture2DMSArray; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D: ezLog::Info("Resource is a 3D Texture"); ssr.m_Type = ezShaderStageResource::Texture3D; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE: ezLog::Info("Resource is a Cube Texture"); ssr.m_Type = ezShaderStageResource::TextureCube; break;
    //  case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY: ezLog::Info("Resource is a Cube Texture Array"); ssr.m_Type = ezShaderStageResource::TextureCubeArray; break;
    //  default:
    //    ezLog::Error("Resource Dimension is an unknown type");
    //    break;
    //  }
    //}

    if (sibd.Type == D3D_SIT_TEXTURE)
    {
      inout_Data.m_StageBinary[Stage].m_ShaderResourceBindings.PushBack(ssr);
    }

    if (sibd.Type == D3D_SIT_CBUFFER)
    {
      ssr.m_Type = ezShaderStageResource::ConstantBuffer;
      inout_Data.m_StageBinary[Stage].m_ShaderResourceBindings.PushBack(ssr);
    }

    //switch (sibd.Type)
    //{
    //case D3D_SIT_TBUFFER: ezLog::Info("Resource is Texture Buffer"); break;
    //case D3D_SIT_TEXTURE: ezLog::Info("Resource is Texture"); break;
    //case D3D_SIT_CBUFFER: ezLog::Info("Resource is Constant Buffer"); break;
    //case D3D_SIT_SAMPLER: ezLog::Info("Resource is Sampler"); break;
    //case D3D_SIT_UAV_RWTYPED:
    //case D3D_SIT_STRUCTURED:
    //case D3D_SIT_UAV_RWSTRUCTURED:
    //case D3D_SIT_BYTEADDRESS:
    //case D3D_SIT_UAV_RWBYTEADDRESS:
    //case D3D_SIT_UAV_APPEND_STRUCTURED:
    //case D3D_SIT_UAV_CONSUME_STRUCTURED:
    //case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
    //default:
    //  ezLog::Error("Resource is some weird type"); break;
    //}
  }

  ReflectMaterialParameters(inout_Data, Stage, pReflector);
}

void ezShaderCompilerHLSL::ReflectMaterialParameters(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage, ID3D11ShaderReflection* pReflector)
{

  const char* szMaterialCBName = "MaterialCB";

  ID3D11ShaderReflectionConstantBuffer* pMaterialCB = pReflector->GetConstantBufferByName(szMaterialCBName);

  if (pMaterialCB != nullptr)
  {
    D3D11_SHADER_BUFFER_DESC sbd;

    if (SUCCEEDED(pMaterialCB->GetDesc(&sbd)))
    {
      EZ_LOG_BLOCK("Material Block", szMaterialCBName);
      ezLog::Debug("MaterialCB has %u variables, Size is %u", sbd.Variables, sbd.Size);

      ezShaderMaterialParamCB mcb;

      mcb.m_uiMaterialCBSize = sbd.Size;

      for (ezUInt32 var = 0; var < sbd.Variables; ++var)
      {
        ID3D11ShaderReflectionVariable* pVar = pMaterialCB->GetVariableByIndex(var);

        D3D11_SHADER_VARIABLE_DESC svd;
        pVar->GetDesc(&svd);

        EZ_LOG_BLOCK("Material Parameter", svd.Name);

        D3D11_SHADER_TYPE_DESC std;
        pVar->GetType()->GetDesc(&std);

        ezShaderMaterialParamCB::MaterialParameter mp;
        mp.m_Type = ezShaderMaterialParamCB::MaterialParameter::Type::Unknown;
        mp.m_uiNameHash = ezTempHashedString(svd.Name).GetHash();
        mp.m_uiOffset = svd.StartOffset;
        mp.m_uiArrayElements = std.Elements;

        if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
        {
          switch (std.Type)
          {
          case D3D_SVT_FLOAT:
            mp.m_Type = (ezShaderMaterialParamCB::MaterialParameter::Type) ((ezInt32) ezShaderMaterialParamCB::MaterialParameter::Type::Float1 + std.Columns - 1);
            break;
          case D3D_SVT_INT:
            mp.m_Type = (ezShaderMaterialParamCB::MaterialParameter::Type) ((ezInt32) ezShaderMaterialParamCB::MaterialParameter::Type::Int1 + std.Columns - 1);
            break;

          default:
            break;
          }
        }
        else if (std.Class == D3D_SVC_MATRIX_ROWS || std.Class == D3D_SVC_MATRIX_COLUMNS)
        {
          /// \todo Only support one matrix layout type

          if (std.Type != D3D_SVT_FLOAT)
          {
            ezLog::Error("Variable '%s': Only float matrices are supported", svd.Name);
            continue;
          }

          if (std.Columns == 3 && std.Rows == 3)
            mp.m_Type = ezShaderMaterialParamCB::MaterialParameter::Type::Mat3x3;
          else if (std.Columns == 4 && std.Rows == 4)
            mp.m_Type = ezShaderMaterialParamCB::MaterialParameter::Type::Mat4x4;
          else if (std.Columns == 4 && std.Rows == 3)
            mp.m_Type = ezShaderMaterialParamCB::MaterialParameter::Type::Mat3x4;
          else
          {
            ezLog::Error("Variable '%s': %ux%u matrices are not supported", svd.Name, std.Rows, std.Columns);
            continue;
          }
        }
        else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
        {
          ezLog::Error("Variable '%s': Column-Major matrices are not supported", svd.Name);
          continue;
        }

        if (mp.m_Type == ezShaderMaterialParamCB::MaterialParameter::Type::Unknown)
        {
          ezLog::Error("Variable '%s': Variable type is unknown / not supported", svd.Name);
          continue;
        }

        mcb.m_MaterialParameters.PushBack(mp);

        //ezLog::Dev("Variable '%s', Offset: %u, Size: %u", svd.Name, svd.StartOffset, svd.Size);

        //switch (std.Class)
        //{
        //case D3D_SVC_SCALAR:
        //  ezLog::Dev("%s: Type is scalar", std.Name);
        //  break;
        //case D3D_SVC_VECTOR:
        //  ezLog::Dev("%s: Type is vector", std.Name);
        //  break;
        //case D3D_SVC_MATRIX_ROWS:
        //  ezLog::Dev("%s: Type is row matrix", std.Name);
        //  break;
        //case D3D_SVC_MATRIX_COLUMNS:
        //  ezLog::Dev("%s: Type is column matrix", std.Name);
        //  break;
        //default:
        //  ezLog::Error("Unknown Type Class");
        //}

        //switch (std.Type)
        //{
        //case D3D_SVT_FLOAT:
        //  ezLog::Dev("%s: Type is FLOAT", std.Name);
        //  break;

        //case D3D_SVT_INT:
        //  ezLog::Dev("%s: Type is INT", std.Name);
        //  break;

        //case D3D_SVT_BOOL:
        //case D3D_SVT_UINT:
        //case D3D_SVT_UINT8:
        //case D3D_SVT_DOUBLE:

        //default:
        //  ezLog::Error("Unknown Type");
        //}
      }

      inout_Data.m_StageBinary[Stage].CreateMaterialParamObject(mcb);
    }

  }

  pReflector->Release();
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

  EZ_REPORT_FAILURE("Unknown Platform '%s' or Stage %i", szPlatform, Stage);
  return "";
}

ezResult ezShaderCompilerHLSL::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    // shader already compiled
    if (!inout_Data.m_StageBinary[stage].m_ByteCode.IsEmpty())
    {
      ezLog::Debug("Shader for stage '%s' is already compiled.", ezGALShaderStage::Names[stage]);
      continue;
    }

    const ezUInt32 uiLength = ezStringUtils::GetStringElementCount(inout_Data.m_szShaderSource[stage]);

    if (uiLength > 0)
    {
      if (CompileDXShader(inout_Data.m_szSourceFile, inout_Data.m_szShaderSource[stage], GetProfileName(inout_Data.m_szPlatform, (ezGALShaderStage::Enum) stage), "main", inout_Data.m_StageBinary[stage].m_ByteCode).Succeeded())
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

