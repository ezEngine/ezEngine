#pragma once

#include <RendererCore/RendererCoreDLL.h>

class MaskedOcclusionCulling;

/// Creates a MaskedOcclusionCulling instance using ezEngine's portable SIMD abstraction.
/// Works on all platforms (x86 SSE, ARM NEON, scalar FPU).
EZ_RENDERERCORE_DLL MaskedOcclusionCulling* ezCreateGenericMOC();
