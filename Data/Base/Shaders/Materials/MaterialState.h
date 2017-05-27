#pragma once

// rasterizer state
#if defined(RENDER_PASS) && RENDER_PASS == RENDER_PASS_WIREFRAME
  WireFrame = true
#endif

#if TWO_SIDED == TRUE
  CullMode = CullMode_None
#else
  #if FLIP_WINDING == TRUE
    CullMode = CullMode_Front
  #endif
#endif


// depth stencil state
DepthTest = true
DepthWrite = true
DepthTestFunc = CompareFunc_LessEqual

#if defined(BLEND_MODE) && (BLEND_MODE == BLEND_MODE_OPAQUE || BLEND_MODE == BLEND_MODE_MASKED)
  
  #if defined(RENDER_PASS) && (RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_EDITOR)
  
    #if !WRITE_DEPTH
      DepthWrite = false
      DepthTestFunc = CompareFunc_Equal
    #endif
    
  #endif
  
#else
  DepthWrite = false
#endif


// blend state
#if defined(BLEND_MODE)

  #if BLEND_MODE == BLEND_MODE_TRANSPARENT
    BlendingEnabled0 = true
    BlendOp0 = BlendOp_Add
    DestBlend0 = Blend_InvSrcAlpha
    SourceBlend0 = Blend_SrcAlpha

  #elif BLEND_MODE == BLEND_MODE_ADDITIVE
    BlendingEnabled0 = true
    BlendOp0 = BlendOp_Add
    DestBlend0 = Blend_One
    SourceBlend0 = Blend_One

  #elif BLEND_MODE == BLEND_MODE_MODULATE
    BlendingEnabled0 = true
    BlendOp0 = BlendOp_Add
    DestBlend0 = Blend_SrcColor
    SourceBlend0 = Blend_Zero
  #endif

#endif
