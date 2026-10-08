#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipelineCVars.h>

ezCVarBool cvar_RenderingPipelineSSAO("Rendering.Pipeline.SSAO", true, ezCVarFlags::Save, "Whether to enable Screen Space Ambient Occlusion.");
ezCVarFloat cvar_RenderingPipelineSSAOMaxScreenSpaceRadius("Rendering.Pipeline.SSAO.MaxScreenSpaceRadius", 1.0f, ezCVarFlags::Save, "Max screen space radius for SSAO");
ezCVarBool cvar_RenderingPipelineSSS("Rendering.Pipeline.SSS", true, ezCVarFlags::Save, "Whether to enable Screen Space Shadows (SSS).");
