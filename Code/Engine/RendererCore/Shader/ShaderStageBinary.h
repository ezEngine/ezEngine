#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERCORE_DLL ezShaderConstantBufferLayout : public ezRefCounted
{
public:
  struct Constant
  {
    EZ_DECLARE_MEM_RELOCATABLE_TYPE();

    struct Type
    {
      typedef ezUInt8 StorageType;

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

    Constant()
    {
      m_uiArrayElements = 0;
      m_uiOffset = 0;
    }

    void CopyDataFormVariant(ezUInt8* pDest, ezVariant* pValue) const;

    ezHashedString m_sName;
    ezEnum<Type> m_Type;
    ezUInt8 m_uiArrayElements;
    ezUInt16 m_uiOffset;
  };

private:
  friend class ezShaderStageBinary;
  friend class ezMemoryUtils;

  ezShaderConstantBufferLayout();
  ~ezShaderConstantBufferLayout();

public:
  ezResult Write(ezStreamWriter& stream) const;
  ezResult Read(ezStreamReader& stream);

  ezUInt32 m_uiTotalSize;
  ezHybridArray<Constant, 16> m_Constants;
};

struct EZ_RENDERERCORE_DLL ezShaderResourceBinding
{
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

  enum ResourceType
  {
    Unknown = 0,

    Texture1D = 1,
    Texture1DArray = 2,
    Texture2D = 3,
    Texture2DArray = 4,
    Texture2DMS = 5,
    Texture2DMSArray = 6,
    Texture3D = 7,
    TextureCube = 8,
    TextureCubeArray = 9,

    UAV = 10,            ///< RW textures and buffers
    ConstantBuffer = 20, ///< Constant buffers
    GenericBuffer = 21,  ///< Read only (structured) buffers
    Sampler = 22,        ///< Separate sampler states
  };

  ezShaderResourceBinding();
  ~ezShaderResourceBinding();

  ResourceType m_Type;
  ezInt32 m_iSlot;
  ezHashedString m_sName;
  ezScopedRefPointer<ezShaderConstantBufferLayout> m_pLayout;
};

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

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  ezShaderStageBinary();
  ~ezShaderStageBinary();

  ezResult Write(ezStreamWriter& Stream) const;
  ezResult Read(ezStreamReader& Stream);

  ezDynamicArray<ezUInt8>& GetByteCode();

  void AddShaderResourceBinding(const ezShaderResourceBinding& binding);
  ezArrayPtr<const ezShaderResourceBinding> GetShaderResourceBindings() const;
  const ezShaderResourceBinding* GetShaderResourceBinding(const ezTempHashedString& sName) const;

  ezShaderConstantBufferLayout* CreateConstantBufferLayout() const;

private:
  friend class ezRenderContext;
  friend class ezShaderCompiler;
  friend class ezShaderPermutationResource;
  friend class ezShaderPermutationResourceLoader;

  ezUInt32 m_uiSourceHash;
  ezGALShaderStage::Enum m_Stage;
  ezDynamicArray<ezUInt8> m_ByteCode;
  ezScopedRefPointer<ezGALShaderByteCode> m_pGALByteCode;
  ezHybridArray<ezShaderResourceBinding, 8> m_ShaderResourceBindings;

  ezResult WriteStageBinary(ezLogInterface* pLog) const;
  static ezShaderStageBinary* LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash);

  static void OnEngineShutdown();

  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
};
