#include <RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Fence.h>

ezUInt64 ezGALFence::GetCompletedValue() const
{
  return GetCompletedValuePlatform();
}

void ezGALFence::Wait(ezUInt64 value) const
{
  WaitPlatform(value);
}

void ezGALFence::Signal(ezUInt64 value) const
{
  SignalPlatform(value);
}

ezGALFence::ezGALFence() {}

ezGALFence::~ezGALFence() {}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Fence);
