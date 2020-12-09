#include <RendererFoundationPCH.h>

#include <RendererFoundation/Device/Pass.h>

ezGALPass::ezGALPass(ezGALDevice& device, const char* szName)
  : m_Device(device)
{
}

ezGALPass::~ezGALPass() = default;
