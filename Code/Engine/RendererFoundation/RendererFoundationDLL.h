#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERFOUNDATION_LIB
#    define EZ_RENDERERFOUNDATION_DLL EZ_DECL_EXPORT
#  else
#    define EZ_RENDERERFOUNDATION_DLL EZ_DECL_IMPORT
#  endif
#else
#  define EZ_RENDERERFOUNDATION_DLL
#endif

// #TODO_SHADER obsolete, DX11 only
#define EZ_GAL_MAX_CONSTANT_BUFFER_COUNT 16
#define EZ_GAL_MAX_SAMPLER_COUNT 16

// Necessary array sizes
#define EZ_GAL_MAX_VERTEX_BUFFER_COUNT 16
#define EZ_GAL_MAX_RENDERTARGET_COUNT 8

// Forward declarations

struct ezGALDeviceCreationDescription;
struct ezGALSwapChainCreationDescription;
struct ezGALWindowSwapChainCreationDescription;
struct ezGALShaderCreationDescription;
struct ezGALTextureCreationDescription;
struct ezGALBufferCreationDescription;
struct ezGALDepthStencilStateCreationDescription;
struct ezGALBlendStateCreationDescription;
struct ezGALRasterizerStateCreationDescription;
struct ezGALVertexDeclarationCreationDescription;
struct ezGALSamplerStateCreationDescription;
struct ezGALTextureResourceViewCreationDescription;
struct ezGALBufferResourceViewCreationDescription;
struct ezGALRenderTargetViewCreationDescription;
struct ezGALTextureUnorderedAccessViewCreationDescription;
struct ezGALBufferUnorderedAccessViewCreationDescription;

class ezGALSwapChain;
class ezGALShader;
class ezGALResourceBase;
class ezGALTexture;
class ezGALSharedTexture;
class ezGALBuffer;
class ezGALReadbackBuffer;
class ezGALReadbackTexture;
class ezGALDepthStencilState;
class ezGALBlendState;
class ezGALRasterizerState;
class ezGALRenderTargetSetup;
class ezGALVertexDeclaration;
class ezGALSamplerState;
class ezGALTextureResourceView;
class ezGALBufferResourceView;
class ezGALRenderTargetView;
class ezGALTextureUnorderedAccessView;
class ezGALBufferUnorderedAccessView;
class ezGALDevice;
class ezGALCommandEncoder;

// Basic enums
struct ezGALPrimitiveTopology
{
  using StorageType = ezUInt8;
  enum Enum
  {
    // keep this order, it is used to allocate the desired number of indices in ezMeshBufferResourceDescriptor::AllocateStreams
    Points,        // 1 index per primitive
    Lines,         // 2 indices per primitive
    Triangles,     // 3 indices per primitive
    TriangleStrip, // 3 indices per primitive, but the first two indices are shared with the previous primitive

    ENUM_COUNT,

    Default = Triangles
  };

  static ezUInt32 GetIndexCount(Enum e, ezUInt32 uiPrimitiveCount)
  {
    if (e <= Triangles)
      return uiPrimitiveCount * ((ezUInt32)e + 1);

    // TriangleStrip
    return uiPrimitiveCount > 0 ? uiPrimitiveCount + 2 : 0;
  }
};

struct EZ_RENDERERFOUNDATION_DLL ezGALIndexType
{
  enum Enum
  {
    None,   // indices are not used, vertices are just used in order to form primitives
    UShort, // 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices
    UInt,   // 32 bit indices are used to select which vertices shall form a primitive

    ENUM_COUNT
  };


  /// \brief The size in bytes of a single element of the given index format.
  static ezUInt8 GetSize(ezGALIndexType::Enum format) { return s_Size[format]; }

private:
  static const ezUInt8 s_Size[ezGALIndexType::ENUM_COUNT];
};

/// \brief The stage of a shader. A complete shader can consist of multiple stages.
/// \sa ezGALShaderStageFlags, ezGALShaderCreationDescription
struct EZ_RENDERERFOUNDATION_DLL ezGALShaderStage
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,
    ComputeShader,
    /*
    // #TODO_SHADER: Future work:
    TaskShader,
    MeshShader,
    RayGenShader,
    RayAnyHitShader,
    RayClosestHitShader,
    RayMissShader,
    RayIntersectionShader,
    */
    ENUM_COUNT,
    Default = VertexShader
  };

  static const char* Names[ENUM_COUNT];
};

/// \brief A set of shader stages.
/// \sa ezGALShaderStage, ezShaderResourceBinding
struct EZ_RENDERERFOUNDATION_DLL ezGALShaderStageFlags
{
  using StorageType = ezUInt16;

  enum Enum : ezUInt16
  {
    VertexShader = EZ_BIT(0),
    HullShader = EZ_BIT(1),
    DomainShader = EZ_BIT(2),
    GeometryShader = EZ_BIT(3),
    PixelShader = EZ_BIT(4),
    ComputeShader = EZ_BIT(5),
    /*
    // #TODO_SHADER: Future work:
    TaskShader = EZ_BIT(6),
    MeshShader = EZ_BIT(7),
    RayGenShader = EZ_BIT(8),
    RayAnyHitShader = EZ_BIT(9),
    RayClosestHitShader = EZ_BIT(10),
    RayMissShader = EZ_BIT(11),
    RayIntersectionShader = EZ_BIT(12),
    */
    Default = 0
  };

  struct Bits
  {
    StorageType VertexShader : 1;
    StorageType HullShader : 1;
    StorageType DomainShader : 1;
    StorageType GeometryShader : 1;
    StorageType PixelShader : 1;
    StorageType ComputeShader : 1;
  };

  inline static ezGALShaderStageFlags::Enum MakeFromShaderStage(ezGALShaderStage::Enum stage)
  {
    return static_cast<ezGALShaderStageFlags::Enum>(EZ_BIT(stage));
  }
};
EZ_DECLARE_FLAGS_OPERATORS(ezGALShaderStageFlags);


struct EZ_RENDERERFOUNDATION_DLL ezGALMSAASampleCount
{
  using StorageType = ezUInt8;

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

struct ezGALTextureType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Invalid = -1,
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DProxy,
    Texture2DShared,
    Texture2DArray,
    TextureCubeArray,

    ENUM_COUNT,

    Default = Texture2D
  };
};

struct ezGALBlend
{
  using StorageType = ezUInt8;

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

    ENUM_COUNT,

    Default = One
  };
};

struct ezGALBlendOp
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,

    ENUM_COUNT,
    Default = Add
  };
};

struct ezGALStencilOp
{
  using StorageType = ezUInt8;

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
  using StorageType = ezUInt8;

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
  using StorageType = ezUInt8;

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
  using StorageType = ezUInt8;

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
    TransientConstantBuffer, ///< Can be executed at any time in a command encoder. Buffer must be completely overwritten. Data will not persist across frames. Only allowed on transient constant buffers.
    AheadOfTime,             ///< Can be executed at any time in a command encoder. Copy is ensured to happen before the next command in the command encoder. The same memory location can't be updated twice in one frame. Note that no GPU access must have happened to the modified memory range in the current command encoder before this call or undefined behavior will occur.
    CopyToTempStorage        ///< Only allowed outside a render pass. Upload to temp buffer, then buffer to buffer transfer at the current time in the command buffer.
  };
};

/// \brief The current state of an async operations in the renderer
struct ezGALAsyncResult
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Ready,   ///< The async operation has finished and the result is ready.
    Pending, ///< The async operation is still running, retry later.
    Expired, ///< The async operation is too old and the result was thrown away. Pending results should be queried every frame until they are ready.
    Default = Expired
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
  ezConstByteBlobPtr m_pData;
  ezUInt32 m_uiRowPitch = 0;
  ezUInt32 m_uiSlicePitch = 0;
};

/// \brief Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class ezGALObject : public ezRefCounted
{
public:
  ezGALObject(const CreationDescription& description)
    : m_Description(description)
  {
  }

  EZ_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};

// Handles
namespace ezGAL
{
  using ez16_16Id = ezGenericId<16, 16>;
  using ez18_14Id = ezGenericId<18, 14>;
  using ez20_12Id = ezGenericId<20, 12>;
  using ez20_44Id = ezGenericId<20, 44>;
} // namespace ezGAL

class ezGALSwapChainHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALSwapChainHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
};

class ezGALShaderHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALShaderHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALTextureHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALTextureHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALReadbackTextureHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALReadbackTextureHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALBufferHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALBufferHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALReadbackBufferHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALReadbackBufferHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALTextureResourceViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALTextureResourceViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALBufferResourceViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALBufferResourceViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALTextureUnorderedAccessViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALTextureUnorderedAccessViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALBufferUnorderedAccessViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALBufferUnorderedAccessViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALRenderTargetViewHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALRenderTargetViewHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

class ezGALDepthStencilStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALDepthStencilStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
};

class ezGALBlendStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALBlendStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
};

class ezGALRasterizerStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALRasterizerStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
};

class ezGALSamplerStateHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALSamplerStateHandle, ezGAL::ez16_16Id);

  friend class ezGALDevice;
};

class ezGALVertexDeclarationHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALVertexDeclarationHandle, ezGAL::ez18_14Id);

  friend class ezGALDevice;
};

using ezGALPoolHandle = ezGAL::ez20_44Id;
using ezGALTimestampHandle = ezGALPoolHandle;
using ezGALOcclusionHandle = ezGALPoolHandle;
using ezGALFenceHandle = ezUInt64;

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
