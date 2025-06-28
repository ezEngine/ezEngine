#pragma once

#include <Foundation/Types/Status.h>
#include <RendererCore/Shader/ShaderHelper.h>
#include <RendererCore/ShaderCompiler/Declarations.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

class ezPropertyAttribute;

class EZ_RENDERERCORE_DLL ezShaderParser
{
public:
  struct AttributeDefinition
  {
    ezString m_sType;
    ezHybridArray<ezVariant, 8> m_Values;
  };

  struct ParameterDefinition
  {
    const ezRTTI* m_pType = nullptr;
    ezString m_sType;
    ezString m_sName;

    ezHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  struct EnumValue
  {
    ezHashedString m_sValueName;
    ezInt32 m_iValueValue = 0;
  };

  struct EnumDefinition
  {
    ezString m_sName;
    ezUInt32 m_uiDefaultValue = 0;
    ezHybridArray<EnumValue, 16> m_Values;
  };

  static ezResult PreprocessSection(ezStreamReader& inout_stream, ezShaderHelper::ezShaderSections::Enum section, ezArrayPtr<ezString> customDefines, ezStringBuilder& out_sResult);

  static void ParseMaterialParameterSection(ezStringView sSection, ezDynamicArray<ParameterDefinition>& out_parameter, ezDynamicArray<EnumDefinition>& out_enumDefinitions);

  static void ParsePermutationSection(ezStringView sPermutationSection, ezDynamicArray<ezHashedString>& out_permVars, ezDynamicArray<ezPermutationVar>& out_fixedPermVars);

  static ezStatus ParseMaterialConstantsSection(ezStringView sMaterialConstantsSection, ezSharedPtr<ezShaderConstantBufferLayout>& out_pMaterialConstantBufferLayout);
  static void LayoutMaterialConstants(ezShaderConstantBufferLayout& ref_materialConstantBufferLayout, ezEnum<ezGALBufferLayout> layout);

  static void ParsePermutationVarConfig(ezStringView sPermutationVarConfig, ezVariant& out_defaultValue, EnumDefinition& out_enumDefinition);



  /// \brief Tries to find shader resource declarations inside the shader source.
  ///
  /// Used by the shader compiler implementations to generate resource mappings to sets/slots without creating conflicts across shader stages. For a list of supported resource declarations and possible pitfalls, please refer to https://ezengine.net/pages/docs/graphics/shaders/shader-resources.html.
  /// \param sShaderStageSource The shader source to parse.
  /// \param out_Resources The shader resources found inside the source.
  static void ParseShaderResources(ezStringView sShaderStageSource, ezDynamicArray<ezShaderResourceDefinition>& out_resources);

  /// \brief Delegate to creates a new declaration and register binding for a specific shader ezShaderResourceDefinition.
  /// \param sPlatform The platform for which the shader is being compiled. Will be one of the values returned by GetSupportedPlatforms.
  /// \param sDeclaration The shader resource declaration without any attributes, e.g. "Texture2D DiffuseTexture"
  /// \param binding The binding that needs to be set on the output out_sDeclaration.
  /// \param out_sDeclaration The new declaration that changes sDeclaration according to the provided 'binding', e.g. "Texture2D DiffuseTexture : register(t0, space5)"
  using CreateResourceDeclaration = ezDelegate<void(ezStringView, ezStringView, const ezShaderResourceBinding&, ezStringBuilder&)>;

  /// \brief Merges the shader resource bindings of all used shader stages.
  ///
  /// The function can fail if a shader resource of the same name has different signatures in two stages. E.g. the type, slot or set is different. Shader resources must be uniquely identified via name.
  /// \param spd The shader currently being processed.
  /// \param out_bindings A hashmap from shader resource name to shader resource binding. If a binding is used in multiple stages, ezShaderResourceBinding::m_Stages will be the combination of all used stages.
  /// \param pLog Log interface to write errors to.
  /// \return Returns failure if the shader stages could not be merged.
  static ezResult MergeShaderResourceBindings(const ezShaderProgramData& spd, ezHashTable<ezHashedString, ezShaderResourceBinding>& out_bindings, ezLogInterface* pLog);

  /// \brief Makes sure that bindings fulfills the basic requirements that ezEngine has for resource bindings in a shader, e.g. that each binding has a set / slot set.
  static ezResult SanityCheckShaderResourceBindings(const ezHashTable<ezHashedString, ezShaderResourceBinding>& bindings, ezLogInterface* pLog);

  /// \brief Creates a new shader source code that patches all shader resources to contain fixed set / slot bindings.
  /// \param sPlatform The platform for which the shader should be patched.
  /// \param sShaderStageSource The original shader source code that should be patched.
  /// \param resources A list of all shader resources that need to be patched within sShaderStageSource.
  /// \param bindings The binding information that each shader resource should have after patching. These bindings must have unique set / slots combinations for each resource.
  /// \param createDeclaration The callback to be called to generate the new shader resource declaration.
  /// \param out_shaderStageSource The new shader source code after patching.
  static void ApplyShaderResourceBindings(ezStringView sPlatform, ezStringView sShaderStageSource, const ezDynamicArray<ezShaderResourceDefinition>& resources, const ezHashTable<ezHashedString, ezShaderResourceBinding>& bindings, const CreateResourceDeclaration& createDeclaration, ezStringBuilder& out_sShaderStageSource);
};
