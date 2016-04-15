#pragma once

#include <RendererCore/Basics.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/Bitflags.h>

class ezGALContext;
class ezShaderStageBinary;
struct ezVertexDeclarationInfo;

typedef ezTypedResourceHandle<class ezTextureResource> ezTextureResourceHandle;
typedef ezTypedResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;
typedef ezTypedResourceHandle<class ezMeshBufferResource> ezMeshBufferResourceHandle;
typedef ezTypedResourceHandle<class ezMaterialResource> ezMaterialResourceHandle;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;
typedef ezTypedResourceHandle<class ezShaderPermutationResource> ezShaderPermutationResourceHandle;

struct ezShaderBindFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,                           ///< No flags causes the default shader binding behavior (all render states are applied)
    ForceRebind = EZ_BIT(0),    ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was previously used with custom bound states
    NoRasterizerState = EZ_BIT(1),    ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer 
    NoDepthStencilState = EZ_BIT(2),    ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil 
    NoBlendState = EZ_BIT(3),    ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend 
    NoStateBinding = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType ForceRebind : 1;
    StorageType NoRasterizerState : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezShaderBindFlags);


struct ezRenderContextFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    ShaderStateChanged = EZ_BIT(0),
    TextureBindingChanged = EZ_BIT(1),
    ConstantBufferBindingChanged = EZ_BIT(2),
    MeshBufferBindingChanged = EZ_BIT(3),

    ShaderStateValid = EZ_BIT(4),
    
    AllStatesInvalid = ShaderStateChanged | TextureBindingChanged | ConstantBufferBindingChanged | MeshBufferBindingChanged,
    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType ShaderStateValid : 1;
    StorageType MeshBufferHasIndexBuffer : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezRenderContextFlags);