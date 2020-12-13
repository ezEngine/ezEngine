#include <RendererFoundationPCH.h>

#include <RendererFoundation/Device/Pass.h>

ezGALRenderCommandEncoder* ezGALPass::BeginRendering(ezGALRenderingSetup&& renderingSetup, const char* szName /*= ""*/)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  return BeginRenderingPlatform(std::move(renderingSetup), szName);
}

ezGALPass::ezGALPass(ezGALDevice& device, const char* szName)
  : m_Device(device)
{
}

ezGALPass::~ezGALPass() = default;
