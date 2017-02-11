#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <Foundation/IO/DependencyFile.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct EZ_RENDERERCORE_DLL ezShaderStateResourceDescriptor
{
  ezGALBlendStateCreationDescription m_BlendDesc;
  ezGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  ezGALRasterizerStateCreationDescription m_RasterizerDesc;

  ezResult Load(const char* szSource);
  void Load(ezStreamReader& stream);
  void Save(ezStreamWriter& stream) const;

  ezUInt32 CalculateHash() const;
};

class EZ_RENDERERCORE_DLL ezShaderPermutationBinary
{
public:
  ezShaderPermutationBinary();

  ezResult Write(ezStreamWriter& Stream);
  ezResult Read(ezStreamReader& Stream, bool& out_bOldVersion);

  ezUInt32 m_uiShaderStageHashes[ezGALShaderStage::ENUM_COUNT];

  ezDependencyFile m_DependencyFile;

  ezShaderStateResourceDescriptor m_StateDescriptor;

  ezHybridArray<ezPermutationVar, 16> m_PermutationVars;
};

