#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

struct ezGALDeviceEvent;
class ezGALDevice;

/// \brief Manages and creates immutable samplers.
/// Once registered, the immutable sampler will be automatically be bound to a shader and can't be changed at runtime.
/// Besides convenience of not having to set these in code, GAL implementations like Vulkan can use this to optimize resource binding.
/// Immutable samplers should be registered at Core Startup via a subsystem, see ezRenderContext::RegisterImmutableSamplers for example.
/// There is no unregister, these samples are immutable after all. The list is cleared on Core Shutdown.
class EZ_RENDERERFOUNDATION_DLL ezGALImmutableSamplers
{
public:
  using ImmutableSamplers = ezHashTable<ezHashedString, ezGALSamplerStateHandle>;

  /// Registers an immutable sampler. Should be called during Core Startup via a subsystem.
  /// \param sSamplerName The shader resource name of the sampler to be added.
  /// \param desc The description of the sampler. It will be created on GAL device init.
  /// \return Returns success if the sampler was not already registered.
  static ezResult RegisterImmutableSampler(ezHashedString sSamplerName, const ezGALSamplerStateCreationDescription& desc);

  /// Returns the table of immutable samplers. Only valid to call after GAL device init.
  /// \return Table of sampler name to sampler handle.
  static const ImmutableSamplers& GetImmutableSamplers();

private:
  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void GALDeviceEventHandler(const ezGALDeviceEvent& e);
  static void CreateSamplers(ezGALDevice* m_pDevice);
  static void DestroySamplers(ezGALDevice* m_pDevice);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, ImmutableSamplers);

  static bool s_bInitialized;
  static ImmutableSamplers s_ImmutableSamplers;
  static ezHashTable<ezHashedString, ezGALSamplerStateCreationDescription> s_ImmutableSamplerDesc;
};
