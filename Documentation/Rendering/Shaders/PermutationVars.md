Permutation Variables {#PermutationVars}
============

Permutation variables are global variables set either through C++ code or exposed through materials. The value of a permutation variable at the time of a drawcall affects which permutation of a shader is used for rendering.

Permutation variables allow to create different variants of the same shader, without creating different shader files. Since their state is global, they decouple the decision which shader to use from the code that actually has this information at hand. 

For instance materials support different rendering modes. By default they using proper lighting, but for debugging purposes we might want to override this and always output unlit diffuse color (or normals, UV coordinates, etc). The information which shader to use to render a certain object is stored either in a material or directly set through code. Without permutation variables we would either need to use an entirely different shader to get our debug output, which would mean that everything would need to support this functionality, or the shader would need to decide the final output mode dynamically, adding a large performance hit for a feature that is not used in the final game.

Permutation variables solve this problem by creating different variants of the shader, and letting the engine pick the correct one depending on the current values.

In shader code, permutation variables are exposed as #define'd preprocessor variables and therefore can be evaluated like any other preprocessor directive.


The Shader Render State Section
-------------------------------

Each shader is made up of several **sections**.

```{.c}
[PLATFORMS] 
ALL
    
[PERMUTATIONS]
    
ALPHATEST
WIREFRAME

[MATERIALPARAMETER]

Permutation ALPHATEST;

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

In the **[PERMUTATIONS]** section the shader author has to list all permutation variables that are going to be evaluated inside the shader code. If a variable is used without being mentioned in this section, your shader might compile and work, but the result will always be the same. 


ezPermVar Files
-------------

Every permutation variable must be defined in a file that has the exact name of the permutation variable and the .ezPermVar extension.

All ezPermVar files must reside in a specific subfolder in any data directory. By default the subfolder is **"Shaders/PermutationVars"**. 


bool Permutation variables
--------------------------

The definition of a boolean permutation variable in its ezPermVar file simply looks like this:

```  
bool TWO_SIDED;
```   

A boolean permutation variable is permuted over the values **TRUE** and **FALSE**. In a shader it would be evaluate like this: 

```
#if defined(PIXEL_SHADER) && TWO_SIDED == TRUE
    uint FrontFace : SV_IsFrontFace;
#endif
```

In C++ code the variable is set like this:

```
ezRenderContext::SetShaderPermutationVariable("TWO_SIDED", "TRUE");
```

enum Permutation Variables
--------------------------

Enum permutation variables allow to use more than two permutation values and they can have more descriptive names.

The definition of an enum variable in its ezPermVar file looks like this:

```  
enum BLEND_MODE
{
    OPAQUE,
    MASKED,
    TRANSPARENT,
    ADDITIVE,
    MODULATE
};
```

**Note:** When evaluating an enum variable in a shader, the value must be prefixed with the name of the variable and an underscore:

```
#if BLEND_MODE == BLEND_MODE_MASKED
    return opacity - MaskThreshold;
#else
    return opacity;
#endif
```

As you can see the name used for comparison is **BLEND_MODE_MASKED** although in the definition it was named **MASKED**. In C++ code we use the actual name though:

```
ezRenderContext::SetShaderPermutationVariable("BLEND_MODE", "MASKED");
```


Exposing Permutations to materials
----------------------------------

By default permutation variables do not show up in materials and therefore cannot be manually specified by artists. If you want a variable to show up, simply list it in the **[MATERIALPARAMETER]** section:

```
[MATERIALPARAMETER]

Permutation ALPHATEST;

```

The type (bool or enum) and the available values are automatically read from the ezPermVar file that defines the variable and will show up in the material properties accordingly.