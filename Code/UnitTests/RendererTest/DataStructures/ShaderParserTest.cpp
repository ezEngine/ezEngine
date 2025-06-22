#include <RendererTest/TestClass/SimpleRendererTest.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) || EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <RendererCore/Shader/ShaderPermutationResource.h>
#  include <RendererCore/ShaderCompiler/ShaderManager.h>
#  include <RendererCore/ShaderCompiler/ShaderParser.h>
#  include <RendererFoundation/Shader/Shader.h>

void CompareLayouts(const ezShaderConstantBufferLayout& layoutA, const ezShaderConstantBufferLayout& layoutB)
{
  if (EZ_TEST_INT(layoutA.m_Constants.GetCount(), layoutB.m_Constants.GetCount()))
  {
    for (ezUInt32 i = 0; i < layoutA.m_Constants.GetCount(); ++i)
    {
      const auto& constantA = layoutA.m_Constants[i];
      const auto& constantB = layoutB.m_Constants[i];

      EZ_TEST_STRING(constantA.m_sName.GetData(), constantB.m_sName.GetData());
      if (constantA.m_Type == ezShaderConstant::Type::Bool || constantB.m_Type == ezShaderConstant::Type::Bool)
      {
        EZ_TEST_BOOL(constantA.m_Type == ezShaderConstant::Type::Bool || constantA.m_Type == ezShaderConstant::Type::UInt1);
        EZ_TEST_BOOL(constantB.m_Type == ezShaderConstant::Type::Bool || constantB.m_Type == ezShaderConstant::Type::UInt1);
      }
      else
      {
        EZ_TEST_INT(constantA.m_Type.GetValue(), constantB.m_Type.GetValue());
      }
      EZ_TEST_INT(constantA.m_uiArrayElements, constantB.m_uiArrayElements);
      EZ_TEST_INT(constantA.m_uiOffset, constantB.m_uiOffset);
    }
  }
  EZ_TEST_INT(layoutA.m_uiTotalSize, layoutB.m_uiTotalSize);
}

void TestMaterialConstants(ezStringView sMaterialConstants, ezStringView sMaterialUsage, ezStringView sShaderName, ezUInt32 uiParameterCount)
{
  // Read template
  ezStringBuilder sEzFileContent;
  {
    ezFileReader fileEz;
    if (!EZ_TEST_RESULT(fileEz.Open("RendererTest/Shaders/ShaderParserTest.ezShader.template")))
    {
      return;
    }
    sEzFileContent.ReadAll(fileEz);
  }

  // Insert shaderSection
  sEzFileContent.ReplaceFirst("{{MATERIAL_SECTION}}", sMaterialConstants);
  sEzFileContent.ReplaceFirst("{{MATERIAL_USAGE}}", sMaterialUsage);

  // Write temp shader
  ezStringBuilder sTempFile(":imgout/", sShaderName);
  {
    ezFileWriter TempFile;
    EZ_TEST_BOOL(TempFile.Open(sTempFile) == EZ_SUCCESS);
    TempFile.WriteBytes(sEzFileContent.GetData(), sEzFileContent.GetElementCount()).IgnoreResult();
    TempFile.Close();
  }

  // Load / compile shader permutation
  auto m_hUVColorShader = ezResourceManager::LoadResource<ezShaderResource>(sShaderName);
  ezResourceLock<ezShaderResource> pShaderResource(m_hUVColorShader, ezResourceAcquireMode::BlockTillLoaded);

  ezHashTable<ezHashedString, ezHashedString> m_PermutationVariables;
  ezShaderPermutationResourceHandle m_hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(m_hUVColorShader, m_PermutationVariables, false);

  if (!EZ_TEST_BOOL(m_hActiveShaderPermutation.IsValid()))
    return;

  ezShaderPermutationResource* pShaderPermutation = ezResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, ezResourceAcquireMode::BlockTillLoaded);
  const ezGALShader* pGalShader = ezGALDevice::GetDefaultDevice()->GetShader(pShaderPermutation->GetGALShader());

  if (EZ_TEST_BOOL(pGalShader))
  {
    ezTempHashedString sConstantBufferName("materialData");
    const ezShaderResourceBinding* pBinding = pGalShader->GetShaderResourceBinding(sConstantBufferName);
    // Compared parsed vs compiled layout
    if (EZ_TEST_BOOL(pBinding && pBinding->m_pLayout && pShaderResource->GetMaterialLayout()))
    {
      EZ_TEST_INT(uiParameterCount, pBinding->m_pLayout->m_Constants.GetCount());
      EZ_TEST_INT(uiParameterCount, pShaderResource->GetMaterialLayout()->m_Constants.GetCount());
      CompareLayouts(*pShaderResource->GetMaterialLayout(), *pBinding->m_pLayout);
    }
  }
  ezResourceManager::EndAcquireResource(pShaderPermutation);
}

EZ_CREATE_SIMPLE_RENDERER_TEST(DataStructures, ShaderParser)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Default")
  {
    ezStringView shaderSection =
      "  COLOR4F(BaseColor);\n"
      "  COLOR4F(EmissiveColor);\n"
      "  FLOAT1(MetallicValue);\n"
      "  FLOAT1(RoughnessValue);\n"
      "  FLOAT1(MaskThreshold);\n"
      "  BOOL1(UseBaseTexture);\n"
      "  BOOL1(UseNormalTexture);\n"
      "  BOOL1(UseRoughnessTexture);\n"
      "  BOOL1(UseMetallicTexture);\n"
      "  BOOL1(UseEmissiveTexture);\n"
      "  BOOL1(UseOcclusionTexture);\n"
      "  BOOL1(UseOrmTexture);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(BaseColor).r", "Temp_Default.ezShader", 12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Zoo")
  {
    ezStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  FLOAT2(Value2);\n"
      "  FLOAT3(Value3);\n"
      "  FLOAT4(Value4);\n"
      "  INT1(Value5);\n"
      "  INT2(Value6);\n"
      "  INT3(Value7);\n"
      "  INT4(Value8);\n"
      "  UINT1(Value9);\n"
      "  UINT2(Value10);\n"
      "  UINT3(Value11);\n"
      "  UINT4(Value12);\n"
      "  MAT3(Value13);\n"
      "  MAT4(Value14);\n"
      "  TRANSFORM(Value15);\n"
      "  COLOR4F(Value16);\n"
      "  BOOL1(Value17);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Zoo.ezShader", 17);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PoorPacking1")
  {
    ezStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  BOOL1(Value2);\n"
      "  COLOR4F(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking1.ezShader", 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PoorPacking2")
  {
    ezStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  COLOR4F(Value2);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking2.ezShader", 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PoorPacking3")
  {
    ezStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  MAT3(Value2);\n"
      "  FLOAT2(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking3.ezShader", 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PoorPacking4")
  {
    ezStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  MAT4(Value2);\n"
      "  INT1(Value3);\n"
      "  UINT2(Value4);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking4.ezShader", 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PoorPacking5")
  {
    ezStringView shaderSection =
      "  UINT2(Value1);\n"
      "  TRANSFORM(Value2);\n"
      "  INT2(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_PoorPacking5.ezShader", 3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Float2a")
  {
    ezStringView shaderSection =
      "  FLOAT2(Value1);\n"
      "  UINT2(Value2);\n"
      "  FLOAT1(Value3);\n"
      "  INT1(Value4);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float2a.ezShader", 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Float2b")
  {
    ezStringView shaderSection =
      "  FLOAT2(Value1);\n"
      "  FLOAT1(Value2);\n"
      "  FLOAT2(Value3);\n"
      "  FLOAT1(Value4);\n"
      "  FLOAT1(Value5);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float2b.ezShader", 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Float2c")
  {
    ezStringView shaderSection =
      "  FLOAT1(Value1);\n"
      "  FLOAT1(Value2);\n"
      "  FLOAT2(Value3);\n"
      "  FLOAT2(Value4);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float2c.ezShader", 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Float3")
  {
    ezStringView shaderSection =
      "  FLOAT3(Value1);\n"
      "  FLOAT3(Value2);\n"
      "  FLOAT3(Value3);\n";
    TestMaterialConstants(shaderSection, "GetMaterialData(Value1).r", "Temp_Float3.ezShader", 3);
  }
}
#endif
