#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipelineCVars.h>

ezCVarInt cvar_RenderingPipelineSSAO("Rendering.Pipeline.SSAO", -1, ezCVarFlags::Save, "Selects the SSAO implementation. 0 for OFF, -1 for default.");
ezCVarFloat cvar_RenderingPipelineSSAOMaxScreenSpaceRadius("Rendering.Pipeline.SSAO.MaxScreenSpaceRadius", 1.0f, ezCVarFlags::Save, "Max screen space radius for SSAO");
ezCVarBool cvar_RenderingPipelineSSS("Rendering.Pipeline.SSS", true, ezCVarFlags::Save, "Whether to enable Screen Space Shadows (SSS).");
ezCVarInt cvar_RenderingPipelineMSAA("Rendering.Pipeline.MSAA", 4, ezCVarFlags::Save, "MSAA sample count: 1 (off), 2, 4 or 8. Only has an effect if the render pipeline selects its render targets with a switch on this value.");
