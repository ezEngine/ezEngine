#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERFOUNDATION_LIB
#    define EZ_RENDERERFOUNDATION_DLL __declspec(dllexport)
#  else
#    define EZ_RENDERERFOUNDATION_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_RENDERERFOUNDATION_DLL
#endif

// Necessary array sizes
#define EZ_GAL_MAX_CONSTANT_BUFFER_COUNT 16
#define EZ_GAL_MAX_SAMPLER_COUNT 16
#define EZ_GAL_MAX_VERTEX_BUFFER_COUNT 16
#define EZ_GAL_MAX_RENDERTARGET_COUNT 8

// Forward declarations

struct ezGALDeviceCreationDescription;
struct ezGALSwapChainCreationDescription;
struct ezGALShaderCreationDescription;
struct ezGALTextureCreationDescription;
struct ezGALBufferCreationDescription;
struct ezGALDepthStencilStateCreationDescription;
struct ezGALBlendStateCreationDescription;
struct ezGALRasterizerStateCreationDescription;
struct ezGALVertexDeclarationCreationDescription;
struct ezGALQueryCreationDescription;
struct ezGALSamplerStateCreationDescription;
struct ezGALResourceViewCreationDescription;
struct ezGALRenderTargetViewCreationDescription;
struct ezGALUnorderedAccessViewCreationDescription;

class ezGALSwapChain;
class ezGALShader;
class ezGALResourceBase;
class ezGALTexture;
class ezGALBuffer;
class ezGALDepthStencilState;
class ezGALBlendState;
class ezGALRasterizerState;
class ezGALRenderTargetSetup;
class ezGALVertexDeclaration;
class ezGALFence;
class ezGALQuery;
class ezGALSamplerState;
class ezGALResourceView;
class ezGALRenderTargetView;
class ezGALUnorderedAccessView;
class ezGALDevice;
class ezGALPass;
class ezGALCommandEncoder;
class ezGALRenderCommandEncoder;
class ezGALComputeCommandEncoder;

// Basic enums
struct ezGALPrimitiveTopology
{
  enum Enum
  {
    // keep this order, it is used to allocate the desired number of indices in ezMeshBufferResourceDescriptor::AllocateStreams
    Points,    // 1 index per primitive
    Lines,     // 2 indices per primitive
    Triangles, // 3 indices per primitive

    ENUM_COUNT
  };

  static ezUInt32 VerticesPerPrimitive(ezGALPrimitiveTopology::Enum e) { return (ezUInt32)e + 1; }
};

struct EZ_RENDERERFOUNDATION_DLL ezGALIndexType
{
  enum Enum
  {
    UShort,
    UInt,

    ENUM_COUNT
  };


  /// \brief The size in bytes of a single element of the given index format.
  static ezUInt8 GetSize(ezGALIndexType::Enum format) { return Size[format]; }

private:
  static const ezUInt8 Size[ezGALIndexType::ENUM_COUNT];
};


struct EZ_RENDERERFOUNDATION_DLL ezGALShaderStage
{
  enum Enum
  {
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,

    ComputeShader,

    ENUM_COUNT
  };

  static const char* Names[ENUM_COUNT];
};

struct EZ_RENDERERFOUNDATION_DLL ezGALMSAASampleCount
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 1,
    TwoSamples = 2,
    FourSamples = 4,
    EightSamples = 8,

    ENUM_COUNT = 4,

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALMSAASampleCount);

struct ezGALTextureType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Invalid = -1,
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DProxy,

    ENUM_COUNT,

    Default = Texture2D
  };
};

struct ezGALBlend
{
  enum Enum
  {
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphaSaturated,
    BlendFactor,
    InvBlendFactor,

    ENUM_COUNT
  };
};

struct ezGALBlendOp
{
  enum Enum
  {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,

    ENUM_COUNT
  };
};

struct ezGALStencilOp
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Keep = 0,
    Zero,
    Replace,
    IncrementSaturated,
    DecrementSaturated,
    Invert,
    Increment,
    Decrement,

    ENUM_COUNT,

    Default = Keep
  };
};

struct ezGALCompareFunc
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,

    ENUM_COUNT,

    Default = Never
  };
};

/// \brief Defines which sides of a polygon gets culled by the graphics card
struct ezGALCullMode
{
  typedef ezUInt8 StorageType;

  /// \brief Defines which sides of a polygon gets culled by the graphics card
  enum Enum
  {
    None = 0,  ///< Triangles do not get culled
    Front = 1, ///< When the 'front' of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< ezGALRasterizerStateCreationDescription for details.
    Back = 2,  ///< When the 'back'  of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< ezGALRasterizerStateCreationDescription for details.

    ENUM_COUNT,

    Default = Back
  };
};

struct ezGALTextureFilterMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Point = 0,
    Linear,
    Anisotropic,

    Default = Linear
  };
};

struct ezGALUpdateMode
{
  enum Enum
  {
    Discard,
    NoOverwrite,
    CopyToTempStorage
  };
};

// Basic structs
struct ezGALTextureSubresource
{
  ezUInt32 m_uiMipLevel = 0;
  ezUInt32 m_uiArraySlice = 0;
};

struct ezGALSystemMemoryDescription
{
  void* m_pData = nullptr;
  ezUInt32 m_uiRowPitch = 0;
  ezUInt32 m_uiSlicePitch = 0;
};

/// \brief Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class ezGALObject : public ezRefCounted
{
public:
  ezGALObject(const CreationDescription& Description)
    : m_Description(Description)
  {
  }

  EZ_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};

// Handles
namespace ezGAL
{
  typedef ezGenericId<16, 16> ez16_16Id;
  typedef ezGenericId<18, 14> ez18_14Id;
  typedef ezGenericId<20, 12> ez20_12Id;
} // namespace ezGAL

class ezGALSwapChainHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALSwapChainHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALShaderHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALShaderHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALTextureHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALTextureHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALBufferHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALBufferHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALResourceViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALResourceViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALUnorderedAccessViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALUnorderedAccessViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALRenderTargetViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALRenderTargetViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALDepthStencilStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALDepthStencilStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALBlendStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALBlendStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALRasterizerStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALRasterizerStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALSamplerStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALSamplerStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALVertexDeclarationHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALVertexDeclarationHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALFenceHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALFenceHandle, ezGAL::ez20_12Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

class ezGALQueryHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALQueryHandle, ezGAL::ez20_12Id);

  friend class ezGALDevice;
  friend struct ezGALContextState;
  friend class ezGALContext;
};

struct ezGALTimestampHandle
{
  EZ_DECLARE_POD_TYPE();

  ezUInt64 m_uiIndex;
  ezUInt64 m_uiFrameCounter;
};

namespace ezGAL
{
  struct ModifiedRange
  {
    EZ_ALWAYS_INLINE void Reset()
    {
      m_uiMin = ezInvalidIndex;
      m_uiMax = 0;
    }

    EZ_FORCE_INLINE void SetToIncludeValue(ezUInt32 value)
    {
      m_uiMin = ezMath::Min(m_uiMin, value);
      m_uiMax = ezMath::Max(m_uiMax, value);
    }

    EZ_FORCE_INLINE void SetToIncludeRange(ezUInt32 uiMin, ezUInt32 uiMax)
    {
      m_uiMin = ezMath::Min(m_uiMin, uiMin);
      m_uiMax = ezMath::Max(m_uiMax, uiMax);
    }

    EZ_ALWAYS_INLINE bool IsValid() const { return m_uiMin <= m_uiMax; }

    EZ_ALWAYS_INLINE ezUInt32 GetCount() const { return m_uiMax - m_uiMin + 1; }

    ezUInt32 m_uiMin = ezInvalidIndex;
    ezUInt32 m_uiMax = 0;
  };
} // namespace ezGAL
