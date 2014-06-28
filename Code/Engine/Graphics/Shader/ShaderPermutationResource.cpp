#include <Graphics/PCH.h>
#include <Graphics/Shader/ShaderPermutationResource.h>
#include <Graphics/Shader/Helper.h>
#include <Graphics/ShaderCompiler/ShaderManager.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderPermutationResource, ezResourceBase, ezRTTIDefaultAllocator<ezShaderPermutationResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezShaderPermutationResource::ezShaderPermutationResource()
{
  m_bValid = false;
  m_uiMaxQualityLevel = 1;
  m_Flags.Add(ezResourceFlags::UpdateOnMainThread);
}

void ezShaderPermutationResource::UnloadData(bool bFullUnload)
{
  m_bValid = false;
  m_uiLoadedQualityLevel = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;

  if (!m_hVertexDeclaration.IsInvalidated())
  {
    ezShaderManager::GetDevice()->DestroyVertexDeclaration(m_hVertexDeclaration);
    m_hVertexDeclaration.Invalidate();
  }

  if (!m_hShader.IsInvalidated())
  {
    ezShaderManager::GetDevice()->DestroyShader(m_hShader);
    m_hShader.Invalidate();
  }
}

void ezShaderPermutationResource::UpdateContent(ezStreamReaderBase& Stream)
{
  ezUInt32 uiGPUMem = 0;
  SetMemoryUsageGPU(0);

  m_bValid = false;

  m_LoadingState = ezResourceLoadState::Loaded;
  m_uiLoadedQualityLevel = 1;
  m_uiMaxQualityLevel = 1;

  if (m_PermutationBinary.Read(Stream).Failed())
  {
    ezLog::Error("Shader Resource '%s': Could not read shader permutation binary", GetResourceID().GetData());
    return;
  }

  ezGALShaderCreationDescription ShaderDesc;

  // iterate over all shader stages, add them to the descriptor
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    const ezUInt32 uiStageHash = m_PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    ezShaderStageBinary* pStageBin = ezShaderStageBinary::LoadStageBinary((ezGALShaderStage::Enum) stage, uiStageHash);

    if (pStageBin == nullptr)
    {
      ezLog::Error("Shader Resource '%s': Stage %u could not be loaded", GetResourceID().GetData(), stage);
      return;
    }

    EZ_ASSERT(pStageBin->m_Stage == stage, "Invalid shader stage! Expected stage %u, but loaded data is for stage %u", stage, pStageBin->m_Stage);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_pGALByteCode;

    uiGPUMem += pStageBin->m_ByteCode.GetCount();
  }

  m_hShader = ezShaderManager::GetDevice()->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    ezLog::Error("Shader Resource '%s': Shader program creation failed", GetResourceID().GetData());
    return;
  }

  m_bValid = true;

  // *** HACK ***

  ezGALVertexDeclarationCreationDescription VertDeclDesc;
  VertDeclDesc.m_hShader = m_hShader;
  VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat, 0, 0, false));
  VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat, 12, 0, false));
  VertDeclDesc.m_VertexAttributes.PushBack(ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat, 24, 0, false));

  m_hVertexDeclaration = ezShaderManager::GetDevice()->CreateVertexDeclaration(VertDeclDesc);

  // ************

  SetMemoryUsageGPU(uiGPUMem);
}

void ezShaderPermutationResource::UpdateMemoryUsage()
{
  SetMemoryUsageCPU(sizeof(this));
}

void ezShaderPermutationResource::CreateResource(const ezShaderPermutationResourceDescriptor& descriptor)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}


