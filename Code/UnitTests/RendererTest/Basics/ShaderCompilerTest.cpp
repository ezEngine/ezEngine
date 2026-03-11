#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererTest/Basics/ShaderCompilerTest.h>

void ezRendererTestShaderCompiler::SetupSubTests()
{
  AddSubTest("Shader Resources", SubTests::ST_ShaderResources);
}

ezResult ezRendererTestShaderCompiler::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));

  m_hUVColorShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ShaderCompilerTest.ezShader");

  return EZ_SUCCESS;
}

ezResult ezRendererTestShaderCompiler::DeInitializeSubTest(ezInt32 iIdentifier)
{
  // m_hShader.Invalidate();
  m_hUVColorShader.Invalidate();

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestShaderCompiler::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  ezHashTable<ezHashedString, ezHashedString> m_PermutationVariables;
  ezShaderPermutationResourceHandle m_hActiveShaderPermutation = ezShaderManager::PreloadSinglePermutation(m_hUVColorShader, m_PermutationVariables, false);

  if (!EZ_TEST_BOOL(m_hActiveShaderPermutation.IsValid()))
    return ezTestAppRun::Quit;

  {
    ezResourceLock<ezShaderPermutationResource> pResource(m_hActiveShaderPermutation, ezResourceAcquireMode::BlockTillLoaded);

    ezArrayPtr<const ezPermutationVar> permutationVars = static_cast<const ezShaderPermutationResource*>(pResource.GetPointer())->GetPermutationVars();

    const ezGALShaderByteCode* pVertex = pResource->GetShaderByteCode(ezGALShaderStage::VertexShader);
    EZ_TEST_BOOL(pVertex);
    const auto& vertexDecl = pVertex->m_ShaderVertexInput;
    if (EZ_TEST_INT(vertexDecl.GetCount(), 12))
    {
      auto CheckVertexDecl = [&](ezGALVertexAttributeSemantic::Enum semantic, ezGALResourceFormat::Enum format, ezUInt8 uiLocation)
      {
        EZ_TEST_INT(vertexDecl[uiLocation].m_eSemantic, semantic);
        EZ_TEST_INT(vertexDecl[uiLocation].m_eFormat, format);
        EZ_TEST_INT(vertexDecl[uiLocation].m_uiLocation, uiLocation);
      };
      CheckVertexDecl(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::RGBAFloat, 0);
      CheckVertexDecl(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::RGBFloat, 1);
      CheckVertexDecl(ezGALVertexAttributeSemantic::Tangent, ezGALResourceFormat::RGFloat, 2);
      CheckVertexDecl(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RFloat, 3);
      CheckVertexDecl(ezGALVertexAttributeSemantic::Color7, ezGALResourceFormat::RGBAUInt, 4);
      CheckVertexDecl(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::RGBUInt, 5);
      CheckVertexDecl(ezGALVertexAttributeSemantic::TexCoord9, ezGALResourceFormat::RGUInt, 6);
      CheckVertexDecl(ezGALVertexAttributeSemantic::BiTangent, ezGALResourceFormat::RUInt, 7);
      CheckVertexDecl(ezGALVertexAttributeSemantic::BoneWeights0, ezGALResourceFormat::RGBAInt, 8);
      CheckVertexDecl(ezGALVertexAttributeSemantic::BoneWeights1, ezGALResourceFormat::RGBInt, 9);
      CheckVertexDecl(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGInt, 10);
      CheckVertexDecl(ezGALVertexAttributeSemantic::BoneIndices1, ezGALResourceFormat::RInt, 11);
    }

    const ezGALShaderByteCode* pPixel = pResource->GetShaderByteCode(ezGALShaderStage::PixelShader);
    EZ_TEST_BOOL(pPixel);
    const ezHybridArray<ezShaderResourceBinding, 8>& bindings = pPixel->m_ShaderResourceBindings;
    {
      auto CheckBinding = [&](ezStringView sName, ezGALShaderResourceType::Enum descriptorType, ezGALShaderTextureType::Enum textureType = ezGALShaderTextureType::Unknown, ezBitflags<ezGALShaderStageFlags> stages = ezGALShaderStageFlags::PixelShader, ezUInt32 uiArraySize = 1)
      {
        for (ezUInt32 i = 0; i < bindings.GetCount(); ++i)
        {
          if (bindings[i].m_sName.GetView() == sName)
          {
            EZ_TEST_INT(bindings[i].m_ResourceType, descriptorType);
            EZ_TEST_INT(bindings[i].m_TextureType, textureType);
            EZ_TEST_INT(bindings[i].m_Stages.GetValue(), stages.GetValue());
            EZ_TEST_INT(bindings[i].m_uiArraySize, uiArraySize);
            return;
          }
        }
        EZ_TEST_BOOL_MSG(false, "Shader resource not found in binding list");
      };
      const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();
      CheckBinding("PointClampSampler"_ezsv, ezGALShaderResourceType::Sampler);
      CheckBinding("PerFrame"_ezsv, ezGALShaderResourceType::ConstantBuffer);
      // CheckBinding("RES_Texture1D"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture1D);
      // CheckBinding("RES_Texture1DArray"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture1DArray);
      CheckBinding("RES_Texture2D"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2D);
      CheckBinding("RES_Texture2DArray"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DArray);
      CheckBinding("RES_Texture2DMS"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMS);
      if (caps.m_bSupportsMultiSampledArrays)
      {
        CheckBinding("RES_Texture2DMSArray"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture2DMSArray);
      }
      CheckBinding("RES_Texture3D"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::Texture3D);
      CheckBinding("RES_TextureCube"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCube);
      CheckBinding("RES_TextureCubeArray"_ezsv, ezGALShaderResourceType::Texture, ezGALShaderTextureType::TextureCubeArray);

      if (caps.m_bSupportsTexelBuffer)
      {
        CheckBinding("RES_Buffer"_ezsv, ezGALShaderResourceType::TexelBuffer);
      }
      CheckBinding("RES_StructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBuffer);
      CheckBinding("RES_ByteAddressBuffer"_ezsv, ezGALShaderResourceType::ByteAddressBuffer);

      // CheckBinding("RES_RWTexture1D"_ezsv, ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture1D);
      // CheckBinding("RES_RWTexture1DArray"_ezsv, ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture1DArray);
      CheckBinding("RES_RWTexture2D"_ezsv, ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture2D);
      CheckBinding("RES_RWTexture2DArray"_ezsv, ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture2DArray);
      CheckBinding("RES_RWTexture3D"_ezsv, ezGALShaderResourceType::TextureRW, ezGALShaderTextureType::Texture3D);
      if (caps.m_bSupportsTexelBuffer)
      {
        CheckBinding("RES_RWBuffer"_ezsv, ezGALShaderResourceType::TexelBufferRW);
      }
      CheckBinding("RES_RWStructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);
      CheckBinding("RES_RWByteAddressBuffer"_ezsv, ezGALShaderResourceType::ByteAddressBufferRW);

      CheckBinding("RES_AppendStructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);
      CheckBinding("RES_ConsumeStructuredBuffer"_ezsv, ezGALShaderResourceType::StructuredBufferRW);
    }
  }

  return ezTestAppRun::Quit;
}


static ezRendererTestShaderCompiler g_ShaderCompilerTest;
