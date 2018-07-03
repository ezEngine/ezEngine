#include <PCH.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleRenderer, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezParticleRenderer::ezParticleRenderer()
{

}

ezParticleRenderer::~ezParticleRenderer()
{

}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Renderer_ParticleRenderer);

