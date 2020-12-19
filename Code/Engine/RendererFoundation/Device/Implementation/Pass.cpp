#include <RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Pass.h>

ezGALRenderCommandEncoder* ezGALPass::BeginRendering(const ezGALRenderingSetup& renderingSetup, const char* szName /*= ""*/)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  ezGALRenderCommandEncoder* pCommandEncoder = BeginRenderingPlatform(renderingSetup, szName);

  if (m_bMarker = !ezStringUtils::IsNullOrEmpty(szName))
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void ezGALPass::EndRendering(ezGALRenderCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndRenderingPlatform(pCommandEncoder);
}

ezGALComputeCommandEncoder* ezGALPass::BeginCompute(const char* szName /*= ""*/)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  ezGALComputeCommandEncoder* pCommandEncoder = BeginComputePlatform(szName);

  if (m_bMarker = !ezStringUtils::IsNullOrEmpty(szName))
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void ezGALPass::EndCompute(ezGALComputeCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndComputePlatform(pCommandEncoder);
}

ezGALPass::ezGALPass(ezGALDevice& device)
  : m_Device(device)
{
}

ezGALPass::~ezGALPass() = default;
