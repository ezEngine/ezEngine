#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/ResourceFormats.h>


/// \file

/// ***** Bind Group Item Notes *****
///
/// The classes in this file define the bindings in a bind group layout. The user should never create these by hand, they are used mainly for easy hashing and comparison of bind groups. Instead, users should use the ezBindGroupBuilder to create bind groups and their items.

/// \brief Sampler contents of ezGALBindGroupItem
struct ezSamplerBindGroupItem
{
  EZ_DECLARE_POD_TYPE();
  ezGALSamplerStateHandle m_hSampler;
};

/// \brief Texture contents of ezGALBindGroupItem
struct ezTextureBindGroupItem
{
  EZ_DECLARE_POD_TYPE();
  ezGALTextureHandle m_hTexture;
  ezGALSamplerStateHandle m_hSampler;               ///< Only used for slots of ezGALShaderResourceType::TextureAndSampler.
  ezGALTextureRange m_TextureRange;
  ezEnum<ezGALResourceFormat> m_OverrideViewFormat; ///< Overrides the default view format. E.g. used for converting between linear and gamma space.
};

/// \brief Buffer contents of ezGALBindGroupItem
struct ezGALBufferBindGroupItem
{
  EZ_DECLARE_POD_TYPE();
  ezGALBufferHandle m_hBuffer;
  ezGALBufferRange m_BufferRange;
  ezEnum<ezGALResourceFormat> m_OverrideTexelBufferFormat; ///< Texel buffer only: Overrides the default view format defined in the buffer description.
};

/// \brief Used by ezGALBindGroupItem to define its content.
struct ezGALBindGroupItemFlags
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    Sampler = EZ_BIT(0),          ///< ezGALBindGroupItem::m_Sampler is valid
    Texture = EZ_BIT(1),          ///< ezGALBindGroupItem::m_Texture is valid
    Buffer = EZ_BIT(2),           ///< ezGALBindGroupItem::m_Buffer is valid
    EmptyBinding = EZ_BIT(3),     ///< The binding slot was empty and filled with a fallback resource from ezGALRendererFallbackResources.
    FallbackResource = EZ_BIT(4), ///< The slot was filled with a fallback resource due to ezResourceAcquireMode::AllowLoadingFallback.
    PartiallyLoaded = EZ_BIT(5),  ///< The resource is only partially loaded.
    TypeFlags = Sampler | Texture | Buffer,
    MetaFlags = EmptyBinding | FallbackResource | PartiallyLoaded,
    Default = 0
  };

  struct Bits
  {
    StorageType Sampler : 1;
    StorageType Texture : 1;
    StorageType Buffer : 1;
    StorageType Fallback : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezGALBindGroupItemFlags);

/// \brief Used by ezGALBindGroupCreationDescription to bind resources to a ezShaderResourceBinding slot.
struct ezGALBindGroupItem : public ezHashableStruct<ezGALBindGroupItem>
{
  EZ_DECLARE_POD_TYPE();
  inline ezGALBindGroupItem();
  inline ezGALBindGroupItem(const ezGALBindGroupItem& rhs);
  inline void operator=(const ezGALBindGroupItem& rhs);

  ezBitflags<ezGALBindGroupItemFlags> m_Flags;
  union
  {
    ezSamplerBindGroupItem m_Sampler;
    ezTextureBindGroupItem m_Texture;
    ezGALBufferBindGroupItem m_Buffer;
  };
};

/// \brief Defines a bind group.
/// Can be set to the renderer via ezGALCommandEncoder::SetBindGroup.
struct EZ_RENDERERFOUNDATION_DLL ezGALBindGroupCreationDescription
{
  ezUInt64 CalculateHash() const;
  void AssertValidDescription(const ezGALDevice& galDevice) const;

  ezGALBindGroupLayoutHandle m_hBindGroupLayout;       ///< The layout that this bind group was created for.
  ezDynamicArray<ezGALBindGroupItem> m_BindGroupItems; ///< Contains one item for every ezShaderResourceBinding in the ezGALBindGroupLayout at matching indices in the arrays.
};

class EZ_RENDERERFOUNDATION_DLL ezGALBindGroup : public ezGALResource<ezGALBindGroupCreationDescription>
{
public:
  virtual bool IsInvalidated() const = 0;

protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
  virtual void Invalidate(ezGALDevice* pDevice) = 0;

  inline ezGALBindGroup(const ezGALBindGroupCreationDescription& Description);
  inline virtual ~ezGALBindGroup();
};

#include <RendererFoundation/Shader/Implementation/BindGroup_inl.h>
