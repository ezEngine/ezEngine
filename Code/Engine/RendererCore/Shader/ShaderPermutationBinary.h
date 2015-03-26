#pragma once

#include <RendererCore/Basics.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <Foundation/IO/DependencyFile.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct EZ_RENDERERCORE_DLL ezShaderStateResourceDescriptor
{
  ezGALBlendStateCreationDescription m_BlendDesc;
  ezGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  ezGALRasterizerStateCreationDescription m_RasterizerDesc;

  ezResult Load(const char* szSource);
  void Load(ezStreamReaderBase& stream);
  void Save(ezStreamWriterBase& stream) const;

  ezUInt32 CalculateHash() const;
};

class EZ_RENDERERCORE_DLL ezShaderPermutationBinary
{
public:
  ezShaderPermutationBinary();

  ezResult Write(ezStreamWriterBase& Stream);
  ezResult Read(ezStreamReaderBase& Stream);

  ezUInt32 m_uiShaderStageHashes[ezGALShaderStage::ENUM_COUNT];

  ezDependencyFile m_DependencyFile;

  ezShaderStateResourceDescriptor m_StateDescriptor;
};

