#pragma once

#include <RendererCore/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;

struct ezShaderStageResource
{
  enum ResourceType
  {
    Texture1D,
    Texture1DArray,
    Texture2D,
    Texture2DArray,
    Texture2DMS,
    Texture2DMSArray,
    Texture3D,
    TextureCube,
    TextureCubeArray,
    ConstantBuffer
  };

  ResourceType m_Type;
  ezInt32 m_iSlot;
  ezHashedString m_Name;
};

struct EZ_RENDERERCORE_DLL ezShaderMaterialParamCB
{
  struct MaterialParameter
  {
    EZ_DECLARE_POD_TYPE();

    enum class Type : ezUInt8
    {
      Unknown,
      Float1,
      Float2,
      Float3,
      Float4,
      Int1,
      Int2,
      Int3,
      Int4,
      Mat3x3,
      Mat4x4,
      Mat3x4,
      ENUM_COUNT
    };

    static ezUInt32 s_TypeSize[(ezUInt32) Type::ENUM_COUNT];

    MaterialParameter()
    {
      m_Type = Type::Unknown;
      m_uiArrayElements = 0;
      m_uiOffset = 0;
      m_uiNameHash = 0;
      m_pCachedValues = nullptr;
    }
    
    Type m_Type;
    ezUInt8 m_uiArrayElements;
    ezUInt16 m_uiOffset;
    ezUInt32 m_uiNameHash;
    mutable void* m_pCachedValues;
  };

  ezShaderMaterialParamCB();
  ezUInt32 GetHash() const;

  /// \todo All the material cb data must be shareable across shaders, to enable reusing the same buffer if the layouts are identical
  ezUInt32 m_uiMaterialCBSize;
  ezHybridArray<MaterialParameter, 16> m_MaterialParameters;
  mutable ezUInt64 m_uiLastBufferModification;
  mutable ezConstantBufferResourceHandle m_hMaterialCB;
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

    ENUM_COUNT,
    VersionCurrent = ENUM_COUNT - 1
  };

  ezShaderStageBinary();
  ~ezShaderStageBinary();

  ezResult Write(ezStreamWriterBase& Stream) const;
  ezResult Read(ezStreamReaderBase& Stream);
  void CreateMaterialParamObject(const ezShaderMaterialParamCB& matparams);

  static void OnEngineShutdown();

//private: // Shader Compilers etc. need access to all data

  ezUInt32 m_uiSourceHash;
  ezGALShaderStage::Enum m_Stage;
  ezDynamicArray<ezUInt8> m_ByteCode;
  ezScopedRefPointer<ezGALShaderByteCode> m_pGALByteCode;
  ezHybridArray<ezShaderStageResource, 8> m_ShaderResourceBindings;

  ezShaderMaterialParamCB* m_pMaterialParamCB;

  ezResult WriteStageBinary() const;

  static ezShaderStageBinary* LoadStageBinary(ezGALShaderStage::Enum Stage, ezUInt32 uiHash);

  static ezMap<ezUInt32, ezShaderStageBinary> s_ShaderStageBinaries[ezGALShaderStage::ENUM_COUNT];
  static ezMap<ezUInt32, ezShaderMaterialParamCB> s_ShaderMaterialParamCBs;
};

