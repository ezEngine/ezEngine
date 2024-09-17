#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <Core/System/Window.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Device/SwapChainWebGPU.h>

void ezGALSwapChainWebGPU::AcquireNextRenderTarget(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
}

void ezGALSwapChainWebGPU::PresentRenderTarget(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();

  // TODO WebGPU: move somewhere else
  //{
  //  ezGALDeviceWebGPU* pRealDevice = static_cast<ezGALDeviceWebGPU*>(pDevice);

  //  wgpu::SurfaceTexture surfaceTexture;
  //  m_Surface.GetCurrentTexture(&surfaceTexture);

  //  wgpu::RenderPassColorAttachment attachment;
  //  attachment.view = surfaceTexture.texture.CreateView();
  //  attachment.loadOp = wgpu::LoadOp::Clear;
  //  attachment.storeOp = wgpu::StoreOp::Store;
  //  attachment.clearValue.g = 0.5f;

  //  wgpu::RenderPassDescriptor renderpass;
  //  renderpass.colorAttachmentCount = 1;
  //  renderpass.colorAttachments = &attachment;

  //  wgpu::CommandEncoder encoder = pRealDevice->GetDevice().CreateCommandEncoder();
  //  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
  //  pass.End();
  //  wgpu::CommandBuffer commands = encoder.Finish();
  //  pRealDevice->GetDevice().GetQueue().Submit(1, &commands);
  //}

#if EZ_DISABLED(EZ_PLATFORM_WEB)
  m_Surface.Present();
#endif
}

ezResult ezGALSwapChainWebGPU::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezGALSwapChainWebGPU::ezGALSwapChainWebGPU(const ezGALWindowSwapChainCreationDescription& Description)
  : ezGALWindowSwapChain(Description)
{
}

ezGALSwapChainWebGPU::~ezGALSwapChainWebGPU() = default;

ezResult ezGALSwapChainWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();

  ezGALDeviceWebGPU* pRealDevice = static_cast<ezGALDeviceWebGPU*>(pDevice);

#if __EMSCRIPTEN__
  wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc{};
  canvasDesc.selector = "#framebuffer";

  wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &canvasDesc};
  m_Surface = pRealDevice->GetInstance().CreateSurface(&surfaceDesc);
  EZ_ASSERT_DEV(m_Surface != nullptr, "No canvas named '#framebuffer' found to create WebGPU swapchain.");

#else

  {
    wgpu::SurfaceDescriptorFromWindowsHWND* desc = new wgpu::SurfaceDescriptorFromWindowsHWND();
    desc->hwnd = m_WindowDesc.m_pWindow->GetNativeWindowHandle();
    desc->hinstance = GetModuleHandle(nullptr);
    std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)> chainedDescriptor = {desc, [](wgpu::ChainedStruct* desc)
      {
        delete reinterpret_cast<wgpu::SurfaceDescriptorFromWindowsHWND*>(desc);
      }};

    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = chainedDescriptor.get();
    m_Surface = pRealDevice->GetInstance().CreateSurface(&descriptor);
  }
#endif

  wgpu::SurfaceCapabilities capabilities;
  m_Surface.GetCapabilities(pRealDevice->GetAdapter(), &capabilities);

  // TODO WebGPU: pick desired format
  m_Format = capabilities.formats[0];
  // m_WindowDesc.m_BackBufferFormat;
  // m_WindowDesc.m_InitialPresentMode;
  // m_WindowDesc.m_bDoubleBuffered;
  // m_WindowDesc.m_SampleCount;

  wgpu::SurfaceConfiguration config = {};
  config.device = pRealDevice->GetDevice();
  config.format = m_Format;

  if (m_WindowDesc.m_pWindow)
  {
    config.width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
    config.height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  }
  else
  {
    config.width = 1024;
    config.height = 768;
  }

  ezLog::Info("Setting surface dimensions to {}x{} - {}", config.width, config.height, (int)config.format);

  m_Surface.Configure(&config);
  //  m_RenderTargets.m_hRTs[0]

  pRealDevice->m_MainSurface = m_Surface;
  EZ_ASSERT_DEV(pRealDevice->m_MainSurface != nullptr, "Invalid surface");
  ezLog::Success("Set device main surface.");

  return EZ_SUCCESS;
}

ezResult ezGALSwapChainWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  m_Surface = nullptr;
  return EZ_SUCCESS;
}
