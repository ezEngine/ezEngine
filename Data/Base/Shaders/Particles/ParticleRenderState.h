// clang-format off
BlendingEnabled0 = true
SourceBlend0 = Blend_SrcAlpha
DestBlendAlpha0 = Blend_One
SourceBlendAlpha0 = Blend_Zero
DepthTest = true
DepthTestFunc = CompareFunc_LessEqual
DepthWrite = false
CullMode = CullMode_None

#if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_NONE
      // this is the fallback permutation, which is used when the shader is used
      // on regular mesh materials (e.g. in the material preview)
      BlendingEnabled0 = false
      DepthWrite = true
#elif PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_ADDITIVE
      DestBlend0 = Blend_One
#elif PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_BLENDED
      DestBlend0 = Blend_InvSrcAlpha
#elif PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_OPAQUE
      BlendingEnabled0 = false
      DepthWrite = true
#endif

  // clang-format on