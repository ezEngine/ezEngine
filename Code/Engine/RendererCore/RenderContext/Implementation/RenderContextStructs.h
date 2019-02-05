#pragma once

#include <RendererCore/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

//////////////////////////////////////////////////////////////////////////
// ezShaderBindFlags
//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezShaderBindFlags
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

//////////////////////////////////////////////////////////////////////////
// ezRenderContextFlags
//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezRenderContextFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    ShaderStateChanged = EZ_BIT(0),
    TextureBindingChanged = EZ_BIT(1),
    UAVBindingChanged = EZ_BIT(2),
    SamplerBindingChanged = EZ_BIT(3),
    BufferBindingChanged = EZ_BIT(4),
    ConstantBufferBindingChanged = EZ_BIT(5),
    MeshBufferBindingChanged = EZ_BIT(6),
    MaterialBindingChanged = EZ_BIT(7),

    AllStatesInvalid = ShaderStateChanged | TextureBindingChanged | UAVBindingChanged | SamplerBindingChanged | BufferBindingChanged | ConstantBufferBindingChanged | MeshBufferBindingChanged,
    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType UAVBindingChanged : 1;
    StorageType SamplerBindingChanged : 1;
    StorageType BufferBindingChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType MaterialBindingChanged : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezRenderContextFlags);

//////////////////////////////////////////////////////////////////////////
// ezDefaultSamplerFlags
//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezDefaultSamplerFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    PointFiltering = 0,
    LinearFiltering = EZ_BIT(0),

    Wrap = 0,
    Clamp = EZ_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezDefaultSamplerFlags);

