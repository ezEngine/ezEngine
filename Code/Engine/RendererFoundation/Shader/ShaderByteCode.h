
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

/// \brief The reflection data of a constant in a shader constant buffer.
/// \sa ezShaderConstantBufferLayout
struct EZ_RENDERERFOUNDATION_DLL ezShaderConstant
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  struct Type
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Default,
      Float1,
      Float2,
      Float3,
      Float4,
      Int1,
      Int2,
      Int3,
      Int4,
      UInt1,
      UInt2,
      UInt3,
      UInt4,
      Mat3x3,
      Mat4x4,
      Transform,
      Bool,
      Struct,
      ENUM_COUNT
    };
  };

  static ezUInt32 s_TypeSize[Type::ENUM_COUNT];

  void CopyDataFromVariant(ezUInt8* pDest, const ezVariant* pValue) const;

  ezHashedString m_sName;
  ezEnum<Type> m_Type;
  ezUInt8 m_uiArrayElements = 0;
  ezUInt16 m_uiOffset = 0;
};

/// \brief Reflection data of a shader constant buffer.
/// \sa ezShaderResourceBinding
class EZ_RENDERERFOUNDATION_DLL ezShaderConstantBufferLayout : public ezRefCounted
{
public:
  ezUInt32 m_uiTotalSize = 0;
  ezHybridArray<ezShaderConstant, 16> m_Constants;
  bool operator==(const ezShaderConstantBufferLayout& rhs) const;
  EZ_ADD_DEFAULT_OPERATOR_NOTEQUAL(const ezShaderConstantBufferLayout&);
};

/// \brief Shader reflection of the vertex shader input.
/// This is needed to figure out how to map a ezGALVertexDeclaration to a vertex shader stage.
/// \sa ezGALShaderByteCode
struct EZ_RENDERERFOUNDATION_DLL ezShaderVertexInputAttribute
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  ezEnum<ezGALVertexAttributeSemantic> m_eSemantic = ezGALVertexAttributeSemantic::Position;
  ezEnum<ezGALResourceFormat> m_eFormat = ezGALResourceFormat::XYZFloat;
  ezUInt8 m_uiLocation = 0; // The bind slot of a vertex input
};

/// \brief Shader reflection of a single shader resource (texture, constant buffer, etc.).
/// \sa ezGALShaderByteCode
struct EZ_RENDERERFOUNDATION_DLL ezShaderResourceBinding
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();
  ezEnum<ezGALShaderResourceType> m_ResourceType;             //< The type of shader resource. Note, not all are supported by EZ right now.
  ezEnum<ezGALShaderTextureType> m_TextureType;               //< Only valid if m_ResourceType is Texture, TextureRW or TextureAndSampler.
  ezBitflags<ezGALShaderStageFlags> m_Stages;                 //< The shader stages under which this resource is bound.
  ezInt16 m_iSet = -1;                                        //< The set to which this resource belongs. Aka. Vulkan descriptor set.
  ezInt16 m_iSlot = -1;                                       //< The slot under which the resource needs to be bound in the set.
  ezUInt32 m_uiArraySize = 1;                                 //< Number of array elements. Only 1 is currently supported. 0 if bindless.
  ezHashedString m_sName;                                     //< Name under which a resource must be bound to fulfill this resource binding.
  ezSharedPtr<ezShaderConstantBufferLayout> m_pLayout;        //< Only valid if ezGALShaderResourceType is ConstantBuffer, PushConstants, StructuredBuffer, StructuredBufferRW.

  static ezResult CreateMergedShaderResourceBinding(const ezArrayPtr<ezArrayPtr<const ezShaderResourceBinding>>& resourcesPerStage, ezDynamicArray<ezShaderResourceBinding>& out_bindings, bool bAllowMultipleBindingPerName);
};

/// \brief This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class EZ_RENDERERFOUNDATION_DLL ezGALShaderByteCode : public ezRefCounted
{
public:
  ezGALShaderByteCode();
  ~ezGALShaderByteCode();

  inline const void* GetByteCode() const;
  inline ezUInt32 GetSize() const;
  inline bool IsValid() const;

public:
  // Filled out by Shader Compiler platform implementation
  ezDynamicArray<ezUInt8> m_ByteCode;
  ezHybridArray<ezShaderResourceBinding, 8> m_ShaderResourceBindings;
  ezHybridArray<ezShaderVertexInputAttribute, 8> m_ShaderVertexInput;
  // Only set in the hull shader.
  ezUInt8 m_uiTessellationPatchControlPoints = 0;

  // Filled out by compiler base library
  ezEnum<ezGALShaderStage> m_Stage = ezGALShaderStage::ENUM_COUNT;
  bool m_bWasCompiledWithDebug = false;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
