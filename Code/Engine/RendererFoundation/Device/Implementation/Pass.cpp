#include <RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Pass.h>

ezGALRenderCommandEncoder* ezGALPass::BeginRendering(const ezGALRenderingSetup& renderingSetup, const char* szName /*= ""*/)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  ezGALRenderCommandEncoder* pCommandEncoder = BeginRenderingPlatform(std::move(renderingSetup), szName);

  pCommandEncoder->PushMarker(szName);

  return pCommandEncoder;
}

ezGALPass::ezGALPass(ezGALDevice& device)
  : m_Device(device)
{
}

ezGALPass::~ezGALPass() = default;
