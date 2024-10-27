#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief The type of a shader resource (ezShaderResourceBinding).
/// Shader resources need to be bound to a shader for it to function. This includes samplers, constant buffers, textures etc. which are all handled by EZ via GAL resource types / views. However, vertex buffers, index buffers and vertex layouts are not considered shader resources and are handled separately.
/// \sa ezGALShaderTextureType, ezShaderResourceBinding
struct ezGALShaderResourceType
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Unknown = 0,
    /// Texture sampler (ezGALSamplerStateHandle). HLSL: SamplerState, SamplerComparisonState
    Sampler,

    /// Read-only struct (ezGALBufferHandle). HLSL: cbuffer, ConstantBuffer
    ConstantBuffer,
    // Read-only struct. Set directly via ezGALCommandEncoder::SetPushConstants. HLSL: Use macro BEGIN_PUSH_CONSTANTS, END_PUSH_CONSTANTS, GET_PUSH_CONSTANT
    PushConstants,

    /// \name Shader Resource Views (SRVs). These are set via ezGALTextureResourceViewHandle / ezGALBufferResourceViewHandle.
    ///@{

    /// Read-only texture view. When set, ezGALShaderTextureType is also set. HLSL: Texture*
    Texture,
    /// Read-only texture view with attached sampler. When set, ezGALShaderTextureType is also set. HLSL: Name sampler the same as texture with _AutoSampler appended.
    TextureAndSampler,
    /// Read-only texel buffer. It's like a 1D texture. HLSL: Buffer
    TexelBuffer,
    /// Read-only array of structs. HLSL: StructuredBuffer<T>, ByteAddressBuffer.
    StructuredBuffer,

    ///@}
    /// \name Unordered Access Views (UAVs). These are set via ezGALTextureUnorderedAccessViewHandle / ezGALBufferUnorderedAccessViewHandle.
    ///@{

    /// Read-write texture view. When set, ezGALShaderTextureType is also set. HLSL: RWTexture*
    TextureRW,
    /// Read-write texel buffer. It's like a 1D texture. HLSL: RWBuffer
    TexelBufferRW,
    /// Read-write array of structs. HLSL: RWStructuredBuffer<T>, RWByteAddressBuffer, AppendStructuredBuffer, ConsumeStructuredBuffer
    StructuredBufferRW,

    ///@}

    // #TODO_SHADER: Future work:
    // Not supported: EZ does not support AppendStructuredBuffer, ConsumeStructuredBuffer yet so while the shader can be compiled, nothing can be bound to these resources. On Vulkan, will probably need yet another type to distinguish the data from the count resource (uav_counter_binding).
    // Not supported: tbuffer, TextureBuffer, these map to CBV on DX11 and to eStorageBuffer on Vulkan, requiring to use a constantBufferView or a UAV. Thus, it bleeds platform implementation details.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, frame-buffer local read-only image view. Required for render passes on mobile.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,Vulkan 1.3 addition, surpasses push-constants but not widely supported yet. May be able to abstract this via PushConstants and custom shader compiler / GAL implementations.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, Vulkan extension for raytracing.

    Default = Unknown
  };
};

/// \brief General category of the shader resource (ezShaderResourceBinding).
/// Note that these are flags because some resources can be multiple resource types, e.g. ezGALShaderResourceType::TextureAndSampler.
struct ezGALShaderResourceCategory
{
  using StorageType = ezUInt8;
  static constexpr int ENUM_COUNT = 4;
  enum Enum : ezUInt8
  {
    Sampler = EZ_BIT(0),        //< Sampler (ezGALSamplerStateHandle).
    ConstantBuffer = EZ_BIT(1), //< Constant Buffer (ezGALBufferHandle)
    TextureSRV = EZ_BIT(2),     //< Shader Resource Views (ezGALTextureResourceViewHandle).
    BufferSRV = EZ_BIT(3),      //< Shader Resource Views (ezGALBufferResourceViewHandle).
    TextureUAV = EZ_BIT(4),     //< Unordered Access Views (ezGALTextureUnorderedAccessViewHandle).
    BufferUAV = EZ_BIT(5),      //< Unordered Access Views (ezGALBufferUnorderedAccessViewHandle).
    Default = 0
  };

  struct Bits
  {
    StorageType Sampler : 1;
    StorageType ConstantBuffer : 1;
    StorageType TextureSRV : 1;
    StorageType BufferSRV : 1;
    StorageType TextureUAV : 1;
    StorageType BufferUAV : 1;
  };

  static ezBitflags<ezGALShaderResourceCategory> MakeFromShaderDescriptorType(ezGALShaderResourceType::Enum type);
};

EZ_DECLARE_FLAGS_OPERATORS(ezGALShaderResourceCategory);

/// \brief The texture type of the shader resource (ezShaderResourceBinding).
struct ezGALShaderTextureType
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
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

    Default = Unknown
  };

  static bool IsArray(ezGALShaderTextureType::Enum format);
};

/// \brief Defines a swap chain's present mode.
/// \sa ezGALWindowSwapChainCreationDescription
struct ezGALPresentMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Immediate,
    VSync,
    ENUM_COUNT,
    Default = VSync
  };
};

/// \brief Defines the usage semantic of a vertex attribute.
/// \sa ezGALVertexAttribute
struct ezGALVertexAttributeSemantic
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    Position,
    Normal,
    Tangent,
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    TexCoord8,
    TexCoord9,

    BiTangent,
    BoneIndices0,
    BoneIndices1,
    BoneWeights0,
    BoneWeights1,

    ENUM_COUNT,
    Default = Position
  };
};

/// \brief Defines for what purpose a buffer can be used for.
/// \sa ezGALBufferCreationDescription
struct ezGALBufferUsageFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    VertexBuffer = EZ_BIT(0),      ///< Can be used as a vertex buffer.
    IndexBuffer = EZ_BIT(1),       ///< Can be used as an index buffer.
    ConstantBuffer = EZ_BIT(2),    ///< Can be used as a constant buffer. Can't be combined with any of the other *Buffer flags.
    TexelBuffer = EZ_BIT(3),       ///< Can be used as a texel buffer.
    StructuredBuffer = EZ_BIT(4),  ///< ezGALShaderResourceType::StructuredBuffer
    ByteAddressBuffer = EZ_BIT(5), ///< ezGALShaderResourceType::ByteAddressBuffer (RAW)

    ShaderResource = EZ_BIT(6),    ///< Can be used for ezGALShaderResourceType in the SRV section.
    UnorderedAccess = EZ_BIT(7),   ///< Can be used for ezGALShaderResourceType in the UAV section.
    DrawIndirect = EZ_BIT(8),      ///< Can be used in an indirect draw call.
    Transient = EZ_BIT(9),      ///< Does not persist across frames. If ConstantBuffer is also set, it can be updated multiple times per frame in the middle of any operation as new memory is created on every update call.

    Default = 0
  };

  struct Bits
  {
    StorageType VertexBuffer : 1;
    StorageType IndexBuffer : 1;
    StorageType ConstantBuffer : 1;
    StorageType TexelBuffer : 1;
    StorageType StructuredBuffer : 1;
    StorageType ByteAddressBuffer : 1;
    StorageType ShaderResource : 1;
    StorageType UnorderedAccess : 1;
    StorageType DrawIndirect : 1;
    StorageType Transient : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezGALBufferUsageFlags);

/// \brief Type of GPU->CPU query.
struct ezGALQueryType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    /// Number of samples that passed the depth and stencil test between begin and end (on a context).
    NumSamplesPassed,
    /// Boolean version of NumSamplesPassed. Any number bigger than 0 equals true.
    AnySamplesPassed,

    Default = NumSamplesPassed
  };
};

/// \brief Type of the shared texture (INTERNAL)
struct ezGALSharedTextureType
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    None,     ///< Not shared
    Exported, ///< Allocation owned by this process
    Imported, ///< Allocation owned by a different process
    Default = None
  };
};

#include <RendererFoundation/Descriptors/Implementation/Enumerations_inl.h>
