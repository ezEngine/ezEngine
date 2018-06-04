Render State {#RenderState}
============

The state of the rendering pipeline can only be set through shaders. There is no way to change the state of the rendering pipeline, other than to select a specific shader which includes that specific state.

Use **Shader Permutations** to create variants of a shader. Each variant may incorporate a different render state. By setting shader permutation variables at runtime you select the specific shader variant (permutation) and thus also get its render state.

This design follows what rendering APIs such as DirectX 12 and Vulkan require.


The Shader Render State Section
-------------------------------

Each shader is made up of several **sections**.

```{.c}
[PLATFORMS]
ALL
DEBUG

[PERMUTATIONS]

ALPHATEST
WIREFRAME

[RENDERSTATE]

#if WIREFRAME == 1
  WireFrame = true
#endif

[VERTEXSHADER]

VS_OUT main(VS_IN Input)
{
  ...
}

[PIXELSHADER]

    ...
```

The render pipeline state associated with the shader is defined in the **[RENDERSTATE]** section. It may use permutation variables just like the shader code. To have different state for different permutations, use standard C preprocessor syntax.


Render States
-------------

The following variables are available in the **[RENDERSTATE]** section. Simply overwrite them with the desired value.

### Rasterizer States

  * bool **DepthClip** = false

  * bool **FrontCounterClockwise** = false

  * bool **LineAA** = false

  * bool **MSAA** = false

  * bool **ScissorTest** = false

  * bool **WireFrame** = false

  * enum **CullMode** = CullMode_Back

```{.c}
    CullMode = CullMode_None
    CullMode = CullMode_Back
    CullMode = CullMode_Front
```

  * float **DepthBiasClamp** = 0.0

  * float **SlopeScaledDepthBias** = 0.0

  * int **DepthBias** = 0



### Depth-Stencil State


  * enum **BackFaceDepthFailOp** = StencilOp_Keep

```{.c}
    BackFaceDepthFailOp = StencilOp_Keep
    BackFaceDepthFailOp = StencilOp_Zero
    BackFaceDepthFailOp = StencilOp_Replace
    BackFaceDepthFailOp = StencilOp_IncrementSaturated
    BackFaceDepthFailOp = StencilOp_DecrementSaturated
    BackFaceDepthFailOp = StencilOp_Invert
    BackFaceDepthFailOp = StencilOp_Increment
    BackFaceDepthFailOp = StencilOp_Decrement
```

  * enum **BackFaceFailOp** = StencilOp_Keep

```{.c}
    BackFaceFailOp = StencilOp_Keep
    BackFaceFailOp = StencilOp_Zero
    BackFaceFailOp = StencilOp_Replace
    BackFaceFailOp = StencilOp_IncrementSaturated
    BackFaceFailOp = StencilOp_DecrementSaturated
    BackFaceFailOp = StencilOp_Invert
    BackFaceFailOp = StencilOp_Increment
    BackFaceFailOp = StencilOp_Decrement
```

  * enum **BackFacePassOp** = StencilOp_Keep

```{.c}
    BackFacePassOp = StencilOp_Keep
    BackFacePassOp = StencilOp_Zero
    BackFacePassOp = StencilOp_Replace
    BackFacePassOp = StencilOp_IncrementSaturated
    BackFacePassOp = StencilOp_DecrementSaturated
    BackFacePassOp = StencilOp_Invert
    BackFacePassOp = StencilOp_Increment
    BackFacePassOp = StencilOp_Decrement
```

  * enum **BackFaceStencilFunc** = CompareFunc_Always

```{.c}
    BackFaceStencilFunc = CompareFunc_Never
    BackFaceStencilFunc = CompareFunc_Less
    BackFaceStencilFunc = CompareFunc_Equal
    BackFaceStencilFunc = CompareFunc_LessEqual
    BackFaceStencilFunc = CompareFunc_Greater
    BackFaceStencilFunc = CompareFunc_NotEqual
    BackFaceStencilFunc = CompareFunc_GreaterEqual
    BackFaceStencilFunc = CompareFunc_Always
```

  * enum **FrontFaceDepthFailOp** = StencilOp_Keep

```{.c}
    FrontFaceDepthFailOp = CompareFunc_Never
    FrontFaceDepthFailOp = CompareFunc_Less
    FrontFaceDepthFailOp = CompareFunc_Equal
    FrontFaceDepthFailOp = CompareFunc_LessEqual
    FrontFaceDepthFailOp = CompareFunc_Greater
    FrontFaceDepthFailOp = CompareFunc_NotEqual
    FrontFaceDepthFailOp = CompareFunc_GreaterEqual
    FrontFaceDepthFailOp = CompareFunc_Always
```

  * enum **FrontFaceFailOp** = StencilOp_Keep

```{.c}
    FrontFaceFailOp = CompareFunc_Never
    FrontFaceFailOp = CompareFunc_Less
    FrontFaceFailOp = CompareFunc_Equal
    FrontFaceFailOp = CompareFunc_LessEqual
    FrontFaceFailOp = CompareFunc_Greater
    FrontFaceFailOp = CompareFunc_NotEqual
    FrontFaceFailOp = CompareFunc_GreaterEqual
    FrontFaceFailOp = CompareFunc_Always
```

  * enum **FrontFacePassOp** = StencilOp_Keep

```{.c}
    FrontFacePassOp = CompareFunc_Never
    FrontFacePassOp = CompareFunc_Less
    FrontFacePassOp = CompareFunc_Equal
    FrontFacePassOp = CompareFunc_LessEqual
    FrontFacePassOp = CompareFunc_Greater
    FrontFacePassOp = CompareFunc_NotEqual
    FrontFacePassOp = CompareFunc_GreaterEqual
    FrontFacePassOp = CompareFunc_Always
```

  * enum **FrontFaceStencilFunc** = CompareFunc_Always

```{.c}
    FrontFaceStencilFunc = CompareFunc_Never
    FrontFaceStencilFunc = CompareFunc_Less
    FrontFaceStencilFunc = CompareFunc_Equal
    FrontFaceStencilFunc = CompareFunc_LessEqual
    FrontFaceStencilFunc = CompareFunc_Greater
    FrontFaceStencilFunc = CompareFunc_NotEqual
    FrontFaceStencilFunc = CompareFunc_GreaterEqual
    FrontFaceStencilFunc = CompareFunc_Always
```

  * bool **DepthTest** = true

  * bool **DepthWrite** = true

  * bool **SeparateFrontAndBack** = false

  * bool **StencilTest** = false

  * enum **DepthTestFunc** = CompareFunc_Less

```{.c}
    DepthTestFunc = CompareFunc_Never
    DepthTestFunc = CompareFunc_Less
    DepthTestFunc = CompareFunc_Equal
    DepthTestFunc = CompareFunc_LessEqual
    DepthTestFunc = CompareFunc_Greater
    DepthTestFunc = CompareFunc_NotEqual
    DepthTestFunc = CompareFunc_GreaterEqual
    DepthTestFunc = CompareFunc_Always
```

  * int **StencilReadMask** = 255

  * int **StencilWriteMask** = 255


### Blend State

  * bool **AlphaToCoverage** = false

  * bool **IndependentBlend** = false

The following variables exist with suffix 0 to 7. If **IndependentBlend** is disabled, only the ones with suffix 0 are used.

  * bool **BlendingEnabled0** = false

  * enum **BlendOp0** = BlendOp_Add

```{.c}
    BlendOp0 = BlendOp_Add
    BlendOp0 = BlendOp_Subtract
    BlendOp0 = BlendOp_RevSubtract
    BlendOp0 = BlendOp_Min
    BlendOp0 = BlendOp_Max
```

  * enum **BlendOpAlpha0** = BlendOp_Add

```{.c}
    BlendOpAlpha0 = BlendOp_Add
    BlendOpAlpha0 = BlendOp_Subtract
    BlendOpAlpha0 = BlendOp_RevSubtract
    BlendOpAlpha0 = BlendOp_Min
    BlendOpAlpha0 = BlendOp_Max
```

  * enum **DestBlend0** = Blend_One

```{.c}
    DestBlend0 = Blend_Zero
    DestBlend0 = Blend_One
    DestBlend0 = Blend_SrcColor
    DestBlend0 = Blend_InvSrcColor
    DestBlend0 = Blend_SrcAlpha
    DestBlend0 = Blend_InvSrcAlpha
    DestBlend0 = Blend_DestAlpha
    DestBlend0 = Blend_InvDestAlpha
    DestBlend0 = Blend_DestColor
    DestBlend0 = Blend_InvDestColor
    DestBlend0 = Blend_SrcAlphaSaturated
    DestBlend0 = Blend_BlendFactor
    DestBlend0 = Blend_InvBlendFactor
```

  * enum **DestBlendAlpha0** = Blend_One

```{.c}
    DestBlendAlpha0 = Blend_Zero
    DestBlendAlpha0 = Blend_One
    DestBlendAlpha0 = Blend_SrcColor
    DestBlendAlpha0 = Blend_InvSrcColor
    DestBlendAlpha0 = Blend_SrcAlpha
    DestBlendAlpha0 = Blend_InvSrcAlpha
    DestBlendAlpha0 = Blend_DestAlpha
    DestBlendAlpha0 = Blend_InvDestAlpha
    DestBlendAlpha0 = Blend_DestColor
    DestBlendAlpha0 = Blend_InvDestColor
    DestBlendAlpha0 = Blend_SrcAlphaSaturated
    DestBlendAlpha0 = Blend_BlendFactor
    DestBlendAlpha0 = Blend_InvBlendFactor
```

  * enum **SourceBlend0** = Blend_One

```{.c}
    SourceBlend0 = Blend_Zero
    SourceBlend0 = Blend_One
    SourceBlend0 = Blend_SrcColor
    SourceBlend0 = Blend_InvSrcColor
    SourceBlend0 = Blend_SrcAlpha
    SourceBlend0 = Blend_InvSrcAlpha
    SourceBlend0 = Blend_DestAlpha
    SourceBlend0 = Blend_InvDestAlpha
    SourceBlend0 = Blend_DestColor
    SourceBlend0 = Blend_InvDestColor
    SourceBlend0 = Blend_SrcAlphaSaturated
    SourceBlend0 = Blend_BlendFactor
    SourceBlend0 = Blend_InvBlendFactor
```

  * enum **SourceBlendAlpha0** = Blend_One

```{.c}
    SourceBlendAlpha0 = Blend_Zero
    SourceBlendAlpha0 = Blend_One
    SourceBlendAlpha0 = Blend_SrcColor
    SourceBlendAlpha0 = Blend_InvSrcColor
    SourceBlendAlpha0 = Blend_SrcAlpha
    SourceBlendAlpha0 = Blend_InvSrcAlpha
    SourceBlendAlpha0 = Blend_DestAlpha
    SourceBlendAlpha0 = Blend_InvDestAlpha
    SourceBlendAlpha0 = Blend_DestColor
    SourceBlendAlpha0 = Blend_InvDestColor
    SourceBlendAlpha0 = Blend_SrcAlphaSaturated
    SourceBlendAlpha0 = Blend_BlendFactor
    SourceBlendAlpha0 = Blend_InvBlendFactor
```

  * int **WriteMask** = 255























