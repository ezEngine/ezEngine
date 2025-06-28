#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Declarations.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

/// \brief Output of ParseShaderResources. A shader resource definition found inside the shader source code.
struct ezShaderResourceDefinition
{
  /// \brief Just the declaration inside the shader source, e.g. "Texture1D Texture".
  ezStringView m_sDeclaration;
  /// \brief The declaration with any optional register mappings, e.g. "Texture1D Texture : register(12t, space3)"
  ezStringView m_sDeclarationAndRegister;
  /// \brief The extracted reflection of the resource containing type, slot, set etc.
  ezShaderResourceBinding m_Binding;
};

/// \brief Flags that affect the compilation process of a shader
struct ezShaderCompilerFlags
{
  using StorageType = ezUInt8;
  enum Enum
  {
    Debug = EZ_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezShaderCompilerFlags);

/// \brief Storage used during the shader compilation process.
struct EZ_RENDERERCORE_DLL ezShaderProgramData
{
  ezShaderProgramData()
  {
    m_sPlatform = {};
    m_sSourceFile = {};

    for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
    {
      m_bWriteToDisk[stage] = true;
      m_sShaderSource[stage].Clear();
      m_Resources[stage].Clear();
      m_uiSourceHash[stage] = 0;
      m_ByteCode[stage].Clear();
    }
  }

  ezBitflags<ezShaderCompilerFlags> m_Flags;
  ezStringView m_sPlatform;
  ezStringView m_sSourceFile;
  ezString m_sShaderSource[ezGALShaderStage::ENUM_COUNT];
  ezHybridArray<ezShaderResourceDefinition, 8> m_Resources[ezGALShaderStage::ENUM_COUNT];
  ezUInt32 m_uiSourceHash[ezGALShaderStage::ENUM_COUNT];
  ezSharedPtr<ezGALShaderByteCode> m_ByteCode[ezGALShaderStage::ENUM_COUNT];
  bool m_bWriteToDisk[ezGALShaderStage::ENUM_COUNT];
  ezSet<ezString> m_MaterialParameters; ///< Any resource matching these names will be forced into the material bind group.
};
