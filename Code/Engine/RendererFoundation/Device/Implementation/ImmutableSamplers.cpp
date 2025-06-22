#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, ImmutableSamplers)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezGALImmutableSamplers::OnEngineStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezGALImmutableSamplers::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool ezGALImmutableSamplers::s_bInitialized = false;
ezGALImmutableSamplers::ImmutableSamplers ezGALImmutableSamplers::s_ImmutableSamplers;
ezHashTable<ezHashedString, ezGALSamplerStateCreationDescription> ezGALImmutableSamplers::s_ImmutableSamplerDesc;

void ezGALImmutableSamplers::OnEngineStartup()
{
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezGALImmutableSamplers::GALDeviceEventHandler));
}

void ezGALImmutableSamplers::OnEngineShutdown()
{
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezGALImmutableSamplers::GALDeviceEventHandler));

  EZ_ASSERT_DEBUG(s_ImmutableSamplers.IsEmpty(), "ezGALDeviceEvent::BeforeShutdown should have been fired before engine shutdown");
  s_ImmutableSamplers.Clear();
  s_ImmutableSamplerDesc.Clear();
}

ezResult ezGALImmutableSamplers::RegisterImmutableSampler(ezHashedString sSamplerName, const ezGALSamplerStateCreationDescription& desc)
{
  EZ_ASSERT_DEBUG(!s_bInitialized, "Registering immutable samplers is only allowed at sub-system startup");
  if (s_ImmutableSamplerDesc.Contains(sSamplerName))
    return EZ_FAILURE;

  s_ImmutableSamplerDesc.Insert(sSamplerName, desc);
  return EZ_SUCCESS;
}

void ezGALImmutableSamplers::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case ezGALDeviceEvent::AfterInit:
      CreateSamplers(e.m_pDevice);
      break;
    case ezGALDeviceEvent::BeforeShutdown:
      DestroySamplers(e.m_pDevice);
      break;
    default:
      break;
  }
}

void ezGALImmutableSamplers::CreateSamplers(ezGALDevice* pDevice)
{
  EZ_ASSERT_DEBUG(s_ImmutableSamplers.IsEmpty(), "Creating more than one GAL device is not supported");
  for (auto it : s_ImmutableSamplerDesc)
  {
    ezGALSamplerStateHandle hSampler = pDevice->CreateSamplerState(it.Value());
    s_ImmutableSamplers.Insert(it.Key(), hSampler);
  }
  s_bInitialized = true;
}

void ezGALImmutableSamplers::DestroySamplers(ezGALDevice* pDevice)
{
  for (auto it : s_ImmutableSamplers)
  {
    pDevice->DestroySamplerState(it.Value());
  }
  s_ImmutableSamplers.Clear();
  s_bInitialized = false;
}

const ezGALImmutableSamplers::ImmutableSamplers& ezGALImmutableSamplers::GetImmutableSamplers()
{
  return s_ImmutableSamplers;
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_ImmutableSamplers);
