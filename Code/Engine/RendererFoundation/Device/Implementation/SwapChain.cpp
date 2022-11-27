#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALSwapChain, ezNoBase, 1, ezRTTINoAllocator)
{
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALWindowSwapChain, ezGALSwapChain, 1, ezRTTINoAllocator)
{
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezGALSwapChainCreationDescription CreateSwapChainCreationDescription(const ezRTTI* pType)
{
  ezGALSwapChainCreationDescription desc;
  desc.m_pSwapChainType = pType;
  return desc;
}

ezGALSwapChain::ezGALSwapChain(const ezRTTI* pSwapChainType)
  : ezGALObject(CreateSwapChainCreationDescription(pSwapChainType))
{
}

ezGALSwapChain::~ezGALSwapChain() {}

//////////////////////////////////////////////////////////////////////////

ezGALWindowSwapChain::Functor ezGALWindowSwapChain::s_Factory;


ezGALWindowSwapChain::ezGALWindowSwapChain(const ezGALWindowSwapChainCreationDescription& Description)
  : ezGALSwapChain(ezGetStaticRTTI<ezGALWindowSwapChain>())
  , m_WindowDesc(Description)
{
}

void ezGALWindowSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

ezGALSwapChainHandle ezGALWindowSwapChain::Create(const ezGALWindowSwapChainCreationDescription& desc)
{
  EZ_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for ezGALWindowSwapChain.");
  return s_Factory(desc);
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SwapChain);
