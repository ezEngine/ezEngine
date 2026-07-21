#pragma once

#include <RendererCore/RendererCoreDLL.h>

class ezRenderGraph;
class ezGALTextureHandle;
struct ezGALTextureRange;

class EZ_RENDERERCORE_DLL ezRenderGraphUtils
{
public:
  static ezRenderGraphTextureHandle GenerateMipMaps(ezGALTextureHandle hTexture, ezGALTextureRange range, ezRenderGraph& ref_renderGraph);
};
