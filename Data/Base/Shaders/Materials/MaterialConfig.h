#pragma once

#if (BLEND_MODE == BLEND_MODE_OPAQUE)
RenderDataCategory = LitOpaque
#elif (BLEND_MODE == BLEND_MODE_MASKED)
RenderDataCategory = LitMasked
#else
RenderDataCategory = LitTransparent
#endif
