#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

/// Descriptor for creating a shader resource.
///
/// Currently empty as shaders are typically loaded from files rather than created from descriptors.
struct ezShaderResourceDescriptor
{
};

class ezShaderConstantBufferLayout;

/// Represents a shader resource loaded from an ezShader file.
///
/// This resource stores metadata about a shader including which permutation variables it uses
/// and the layout of material constants. The actual compiled shader code is stored in
/// ezShaderPermutationResource instances.
///
/// Shader resources are parsed from .ezShader files which contain sections like [PERMUTATIONS]
/// and [MATERIALCONSTANTS]. The resource itself does not contain GPU shader bytecode.
class EZ_RENDERERCORE_DLL ezShaderResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezShaderResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezShaderResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezShaderResource, ezShaderResourceDescriptor);

public:
  ezShaderResource();
  ~ezShaderResource() = default;

  /// Returns whether the shader resource was loaded and parsed successfully.
  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  /// Returns the list of permutation variables that this shader declares.
  ///
  /// These are parsed from the [PERMUTATIONS] section of the .ezShader file.
  /// The system uses this information to generate shader permutations for different
  /// combinations of these variables.
  ezArrayPtr<const ezHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

  /// Returns the layout of material constants for this shader.
  ///
  /// This describes the structure and types of constants that can be set per-material.
  /// Returns nullptr if the shader has no [MATERIALCONSTANTS] section.
  const ezSharedPtr<ezShaderConstantBufferLayout>& GetMaterialLayout() const { return m_pLayout; }

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  ezHybridArray<ezHashedString, 16> m_PermutationVarsUsed;
  ezSharedPtr<ezShaderConstantBufferLayout> m_pLayout;
  bool m_bShaderResourceIsValid = false;
};
