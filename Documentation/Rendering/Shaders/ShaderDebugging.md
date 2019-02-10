Shader Debugging {#ShaderDebugging}
============

To debug a shader, one can configure a shader such that the shader compiler includes debugging information. To do so, include **DEBUG** as a platform in the **[PLATFORMS]** section of the shader:

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

