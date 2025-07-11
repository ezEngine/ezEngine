#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

/// Serializes ezGALShaderByteCode and provides access to the shader cache via LoadStageBinary.
class EZ_RENDERERCORE_DLL ezShaderStageBinary
{
public:
  enum Version
  {
    Version0,
    Version1,
    Version2,
    Version3, // Added Material Parameters
    Version4, // Constant buffer layouts
    Version5, // Debug flag
    Version6, // Rewrite, no backwards compatibility. Moves all data into ezGALShaderByteCode.
    Version7, // Added tessellation support (m_uiTessellationPatchControlPoints)

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  ezShaderStageBinary();
  ~ezShaderStageBinary();

  ezSharedPtr<const ezGALShaderByteCode> GetByteCode() const;

private:
  friend class ezRenderContext;
  friend class ezShaderCompiler;
  friend class ezShaderPermutationResource;
  friend class ezShaderPermutationResourceLoader;

  ezResult WriteStageBinary(ezLogInterface* pLog, ezStringView sPlatform) const;
  ezResult Write(ezStreamWriter& inout_stream) const;
  ezResult Read(ezStreamReader& inout_stream);
  ezResult Write(ezStreamWriter& inout_stream, const ezShaderConstantBufferLayout& layout) const;
  ezResult Read(ezStreamReader& inout_stream, ezShaderConstantBufferLayout& out_layout);

private:
  ezUInt32 m_uiSourceHash = 0;
  ezSharedPtr<ezGALShaderByteCode> m_pGALByteCode;

private: // statics
  static ezShaderStageBinary* LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash, ezStringView sPlatform);

  static void OnEngineShutdown();

  static ezMutex s_ShaderStageBinariesLock;
  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
};
