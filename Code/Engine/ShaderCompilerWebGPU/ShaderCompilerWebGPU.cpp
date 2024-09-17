#include <ShaderCompilerWebGPU/ShaderCompilerWebGPU.h>

#include <Foundation/IO/OSFile.h>
#include <src/tint/lang/core/ir/module.h>
#include <src/tint/lang/spirv/reader/reader.h>
#include <src/tint/lang/wgsl/writer/writer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerWebGPU, 1, ezRTTIDefaultAllocator<ezShaderCompilerWebGPU>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezShaderCompilerWebGPU::GetSupportedPlatforms(ezHybridArray<ezString, 4>& out_platforms)
{
  out_platforms.PushBack("WGSL");
}

void ezShaderCompilerWebGPU::ConfigureDxcArgs(ezDynamicArray<ezStringWChar>& inout_Args)
{
  SUPER::ConfigureDxcArgs(inout_Args);

  inout_Args.PushBack(L"-Zpr"); // TODO WebGPU: treat matrices as row major
}

void ReplaceInputSemanticLocation(ezStringBuilder& code, ezStringView sKeyword, ezUInt32 uiLocation)
{

  if (const char* szKey = code.FindSubString(sKeyword))
  {
    if (const char* szLoc = code.FindLastSubString("@location", szKey))
    {
      ezStringBuilder repl;
      repl.SetFormat("@location({}) ", uiLocation);

      ezLog::Dev("Replaced '{}' with '{}'", ezStringView(szLoc, szKey), repl);
      code.ReplaceSubString(szLoc, szKey, repl);
    }
  }
}

void PatchVertexShader(ezStringBuilder& code)
{
  ReplaceInputSemanticLocation(code, "in_var_POSITION_param", ezGALVertexAttributeSemantic::Position);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD0_param", ezGALVertexAttributeSemantic::TexCoord0);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD1_param", ezGALVertexAttributeSemantic::TexCoord1);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD2_param", ezGALVertexAttributeSemantic::TexCoord2);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD3_param", ezGALVertexAttributeSemantic::TexCoord3);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD4_param", ezGALVertexAttributeSemantic::TexCoord4);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD5_param", ezGALVertexAttributeSemantic::TexCoord5);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD6_param", ezGALVertexAttributeSemantic::TexCoord6);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD7_param", ezGALVertexAttributeSemantic::TexCoord7);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD8_param", ezGALVertexAttributeSemantic::TexCoord8);
  ReplaceInputSemanticLocation(code, "in_var_TEXCOORD9_param", ezGALVertexAttributeSemantic::TexCoord9);
  ReplaceInputSemanticLocation(code, "in_var_NORMAL_param", ezGALVertexAttributeSemantic::Normal);
  ReplaceInputSemanticLocation(code, "in_var_TANGENT_param", ezGALVertexAttributeSemantic::Tangent);
  ReplaceInputSemanticLocation(code, "in_var_BITANGENT_param", ezGALVertexAttributeSemantic::BiTangent);
  ReplaceInputSemanticLocation(code, "in_var_COLOR_param", ezGALVertexAttributeSemantic::Color0);
  ReplaceInputSemanticLocation(code, "in_var_COLOR0_param", ezGALVertexAttributeSemantic::Color0);
  ReplaceInputSemanticLocation(code, "in_var_COLOR1_param", ezGALVertexAttributeSemantic::Color1);
  ReplaceInputSemanticLocation(code, "in_var_BONEINDICES_param", ezGALVertexAttributeSemantic::BoneIndices0);
  ReplaceInputSemanticLocation(code, "in_var_BONEINDICES0_param", ezGALVertexAttributeSemantic::BoneIndices0);
  ReplaceInputSemanticLocation(code, "in_var_BONEINDICES1_param", ezGALVertexAttributeSemantic::BoneIndices1);
  ReplaceInputSemanticLocation(code, "in_var_BONEWEIGHTS0_param", ezGALVertexAttributeSemantic::BoneWeights0);
  ReplaceInputSemanticLocation(code, "in_var_BONEWEIGHTS1_param", ezGALVertexAttributeSemantic::BoneWeights1);
}

ezResult ezShaderCompilerWebGPU::Compile(ezShaderProgramData& inout_data, ezLogInterface* pLog)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Compile(inout_data, pLog));

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_ByteCode[stage] == nullptr)
      continue;
    if (inout_data.m_bWriteToDisk[stage] == false)
    {
      // Shader for this stage is already compiled
      continue;
    }

    ezGALShaderByteCode& bytecode = *inout_data.m_ByteCode[stage].Borrow();

    tint::Program program;

    // read SPIR-V bytecode
    {
      std::vector<ezUInt32> spirv;
      spirv.resize(bytecode.GetSize() / 4);
      ezMemoryUtils::RawByteCopy(spirv.data(), bytecode.GetByteCode(), bytecode.GetSize());

      tint::spirv::reader::Options opt0;
      opt0.allow_non_uniform_derivatives = true;
      opt0.allowed_features = tint::wgsl::AllowedFeatures::Everything();
      program = tint::spirv::reader::Read(spirv, opt0);

      if (!program.IsValid())
      {
        ezLog::Error(pLog, program.Diagnostics().Str().c_str());
        return EZ_FAILURE;
      }
    }

    ezStringBuilder wgslCode;

    // convert to WGSL
    {
      tint::wgsl::writer::Options opt;
      opt.use_syntax_tree_writer = false;
      tint::Result<tint::wgsl::writer::Output> wgslOutput = tint::wgsl::writer::Generate(program, opt);

      if (wgslOutput != tint::Success)
      {
        for (auto& r : wgslOutput.Failure().reason)
        {
          ezLog::Error(r.message.Plain().c_str());
        }

        return EZ_FAILURE;
      }

      wgslCode = wgslOutput.Get().wgsl.c_str();
    }

    if (bytecode.m_Stage == ezGALShaderStage::VertexShader)
    {
      PatchVertexShader(wgslCode);
    }

    bytecode.m_ByteCode.SetCountUninitialized(wgslCode.GetElementCount() + 1);
    ezMemoryUtils::RawByteCopy(bytecode.m_ByteCode.GetData(), wgslCode.GetData(), wgslCode.GetElementCount() + 1);

    if constexpr (false)
    {
      if (bytecode.m_Stage == ezGALShaderStage::VertexShader)
      {
        ezOSFile f;
        if (f.Open("D:\\wgsl-shader-vs.txt", ezFileOpenMode::Write).Succeeded())
        {
          f.Write(wgslCode.GetData(), wgslCode.GetElementCount()).AssertSuccess();
          f.Close();
        }
      }

      if (bytecode.m_Stage == ezGALShaderStage::PixelShader)
      {
        ezOSFile f;
        if (f.Open("D:\\wgsl-shader-ps.txt", ezFileOpenMode::Write).Succeeded())
        {
          f.Write(wgslCode.GetData(), wgslCode.GetElementCount()).AssertSuccess();
          f.Close();
        }
      }
    }
  }

  return EZ_SUCCESS;
}
