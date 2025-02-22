#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/RenderInterface.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Plugins/Shaders/RmlUiConstants.h>

namespace ezRmlUiInternal
{
  struct Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec2 m_Position;
    ezVec2 m_TexCoord;
    ezColorLinearUB m_Color;
  };

  struct CompiledGeometry
  {
    ezUInt32 m_uiTriangleCount = 0;
    ezGALBufferHandle m_hVertexBuffer;
    ezGALBufferHandle m_hIndexBuffer;
  };

  struct CommandType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Invalid,
      RenderGeometry,
      SetScissorRegion,
      EnableClipMask,
      RenderToClipMask,

      Default = Invalid
    };
  };

  struct alignas(4) CommandHeader
  {
    ezEnum<CommandType> m_Type;
    bool m_bValue = false;
    ezUInt8 m_uiPadding[2] = {};
  };

  struct CommandRenderGeometry : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderGeometry;

    CompiledGeometry m_CompiledGeometry;
    ezGALTextureHandle m_hTexture;
    ezMat4 m_Transform = ezMat4::MakeIdentity();
    ezVec2 m_Translation = ezVec2::MakeZero();
  };

  struct CommandSetScissorRegion : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::SetScissorRegion;

    ezRectU32 m_ScissorRect = {};
  };

  struct CommandEnableClipMask : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::EnableClipMask;
  };

  struct CommandRenderToClipMask : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderToClipMask;

    Rml::ClipMaskOperation m_Operation;
    CompiledGeometry m_CompiledGeometry;
    ezMat4 m_Transform = ezMat4::MakeIdentity();
    ezVec2 m_Translation = ezVec2::MakeZero();
  };

  struct CommandBuffer
  {
    ezHashedString m_sName;
    ezDynamicArray<ezUInt8> m_Buffer;
    ezGALTextureHandle m_hTargetTexture;
    ezUInt32 m_uiTargetWidth = 0;
    ezUInt32 m_uiTargetHeight = 0;

    template <typename T>
    T& AddCommand()
    {
      T& cmd = *reinterpret_cast<T*>(m_Buffer.ExpandBy(sizeof(T)));
      cmd.m_Type = T::Type;
      return cmd;
    }

    template <typename T>
    const T& ConsumeCommand(ezUInt32& inout_uiOffset) const
    {
      const T& cmd = *reinterpret_cast<const T*>(m_Buffer.GetData() + inout_uiOffset);
      inout_uiOffset += sizeof(T);
      return cmd;
    }

    CommandType::Enum PeekCommandType(ezUInt32 uiOffset) const
    {
      return static_cast<CommandType::Enum>(*(m_Buffer.GetData() + uiOffset));
    }

    void Clear()
    {
      m_Buffer.Clear();
    }
  };

  //////////////////////////////////////////////////////////////////////////

  RenderInterface::RenderInterface()
  {
    ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&RenderInterface::GALEventHandler, this));

    m_hFallbackTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RmlUi.ezShader");
    m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezRmlUiConstants>();

    // Setup the vertex declaration
    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Position;
      si.m_Format = ezGALResourceFormat::XYFloat;
      si.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_Position);
      si.m_uiElementSize = sizeof(ezRmlUiInternal::Vertex::m_Position);
    }

    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
      si.m_Format = ezGALResourceFormat::UVFloat;
      si.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_TexCoord);
      si.m_uiElementSize = sizeof(ezRmlUiInternal::Vertex::m_TexCoord);
    }

    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Color0;
      si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_Color);
      si.m_uiElementSize = sizeof(ezRmlUiInternal::Vertex::m_Color);
    }
  }

  RenderInterface::~RenderInterface()
  {
    ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&RenderInterface::GALEventHandler, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }

    m_Textures.Clear();
    m_hFallbackTexture.Invalidate();
  }

  Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
  {
    const ezUInt32 uiNumVertices = static_cast<ezUInt32>(vertices.size());
    const ezUInt32 uiNumIndices = static_cast<ezUInt32>(indices.size());

    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = uiNumIndices / 3;

    // vertices
    {
      ezDynamicArray<Vertex> vertexStorage(ezFrameAllocator::GetCurrentAllocator());
      vertexStorage.SetCountUninitialized(uiNumVertices);

      for (ezUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = vertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = ezVec2(srcVertex.position.x, srcVertex.position.y);
        destVertex.m_TexCoord = ezVec2(srcVertex.tex_coord.x, srcVertex.tex_coord.y);
        destVertex.m_Color = reinterpret_cast<const ezColorLinearUB&>(srcVertex.colour);
      }

      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;

      geometry.m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezUInt32);
      desc.m_uiTotalSize = uiNumIndices * desc.m_uiStructSize;
      desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;

      geometry.m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezMakeArrayPtr(indices.data(), uiNumIndices).ToByteArray());
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void RenderInterface::RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderGeometry>();
    EZ_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(hGeometry), cmd.m_CompiledGeometry), "Invalid compiled geometry");

    TextureInfo textureInfo;
    if (m_Textures.TryGetValue(TextureId::FromRml(hTexture), textureInfo) == false)
    {
      textureInfo.m_hTexture = m_hFallbackTexture;
    }

    ezResourceLock<ezTexture2DResource> pTexture(textureInfo.m_hTexture, ezResourceAcquireMode::BlockTillLoaded);
    cmd.m_hTexture = pTexture->GetGALTexture();

    cmd.m_Transform = m_mTransform;
    cmd.m_Translation = ezVec2(translation.x, translation.y);
    cmd.m_bValue = textureInfo.m_bHasPremultipliedAlpha == false;
  }

  void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry)
  {
    EZ_LOCK(m_ReleasedCompiledGeometryMutex);

    m_ReleasedCompiledGeometry.PushBack({ezRenderWorld::GetFrameCounter(), GeometryId::FromRml(hGeometry)});
  }

  Rml::TextureHandle RenderInterface::LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource)
  {
    ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sSource.c_str());

    ezResourceLock<ezTexture2DResource> pTexture(hTexture, ezResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      out_textureSize = Rml::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      TextureInfo textureInfo;
      textureInfo.m_hTexture = hTexture;
      textureInfo.m_bHasPremultipliedAlpha = false;

      return m_Textures.Insert(textureInfo).ToRml();
    }

    return ezRmlUiInternal::TextureId().ToRml();
  }

  Rml::TextureHandle RenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize)
  {
    ezUInt32 uiWidth = sourceSize.x;
    ezUInt32 uiHeight = sourceSize.y;
    ezUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;
    EZ_ASSERT_DEV(uiSizeInBytes == source.size(), "Invalid source size");

    ezUInt64 uiHash = ezHashingUtils::xxHash64(source.data(), uiSizeInBytes);

    ezStringBuilder sTextureName;
    sTextureName.SetFormat("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    ezTexture2DResourceHandle hTexture = ezResourceManager::GetExistingResource<ezTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      ezGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData = ezMakeByteBlobPtr(source.data(), uiSizeInBytes);
      memoryDesc.m_uiRowPitch = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      ezTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

      hTexture = ezResourceManager::GetOrCreateResource<ezTexture2DResource>(sTextureName, std::move(desc));
    }

    TextureInfo textureInfo;
    textureInfo.m_hTexture = hTexture;
    textureInfo.m_bHasPremultipliedAlpha = true;

    return m_Textures.Insert(textureInfo).ToRml();
  }

  void RenderInterface::ReleaseTexture(Rml::TextureHandle hTexture)
  {
    TextureId textureId = TextureId::FromRml(hTexture);
    if (textureId.IsInvalidated() == false)
    {
      EZ_VERIFY(m_Textures.Remove(textureId), "Invalid texture handle");
    }
  }

  void RenderInterface::EnableScissorRegion(bool bEnable)
  {
    // Scissor is always enabled in our shaders, set the full viewport if disabled
    if (bEnable == false)
    {
      SetScissorRegion(Rml::Rectanglei::FromSize({(int)m_pCurrentCommandBuffer->m_uiTargetWidth, (int)m_pCurrentCommandBuffer->m_uiTargetHeight}));
    }
  }

  void RenderInterface::SetScissorRegion(Rml::Rectanglei region)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandSetScissorRegion>();
    cmd.m_ScissorRect = ezRectU32(region.Left(), region.Top(), region.Width(), region.Height());
  }

  void RenderInterface::EnableClipMask(bool bEnable)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandEnableClipMask>();
    cmd.m_bValue = bEnable;
  }

  void RenderInterface::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderToClipMask>();
    cmd.m_Operation = operation;
    cmd.m_CompiledGeometry = m_CompiledGeometry[GeometryId::FromRml(hGeometry)];
    cmd.m_Transform = m_mTransform;
    cmd.m_Translation = ezVec2(translation.x, translation.y);
  }

  void RenderInterface::SetTransform(const Rml::Matrix4f* pTransform)
  {
    if (pTransform != nullptr)
    {
      constexpr bool bColumnMajor = std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value;

      if (bColumnMajor)
        m_mTransform = ezMat4::MakeFromColumnMajorArray(pTransform->data());
      else
        m_mTransform = ezMat4::MakeFromRowMajorArray(pTransform->data());
    }
    else
    {
      m_mTransform.SetIdentity();
    }
  }

  void RenderInterface::BeginExtraction(const ezHashedString& sName, ezGALTextureHandle hTargetTexture)
  {
    m_mTransform = ezMat4::MakeIdentity();

    m_pCurrentCommandBuffer = AllocateCommandBuffer();
    m_pCurrentCommandBuffer->m_sName = sName;
    m_pCurrentCommandBuffer->m_hTargetTexture = hTargetTexture;

    const ezGALTexture* pTargetTexture = ezGALDevice::GetDefaultDevice()->GetTexture(hTargetTexture);
    m_pCurrentCommandBuffer->m_uiTargetWidth = pTargetTexture->GetDescription().m_uiWidth;
    m_pCurrentCommandBuffer->m_uiTargetHeight = pTargetTexture->GetDescription().m_uiHeight;
  }

  void RenderInterface::EndExtraction()
  {
    SubmitCommandBuffer(std::move(m_pCurrentCommandBuffer));
  }

  void RenderInterface::GALEventHandler(const ezGALDeviceEvent& e)
  {
    if (e.m_Type == ezGALDeviceEvent::AfterBeginFrame)
    {
      BeginFrame();
    }
    else if (e.m_Type == ezGALDeviceEvent::BeforeEndFrame)
    {
      EndFrame();
    }
  }

  void RenderInterface::BeginFrame()
  {
    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[ezRenderWorld::GetDataIndexForRendering()];
    if (submittedCommandBuffers.IsEmpty())
      return;

    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALCommandEncoder* pCommandEncoder = pDevice->BeginCommands("RmlUi");

    ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();
    ezGPUResourcePool* pGpuResourcePool = ezGPUResourcePool::GetDefaultInstance();

    // Force shader loading, otherwise we could end up rendering nothing when lazy update is enabled on the context
    bool bAllowAsyncShaderLoading = pRenderContext->GetAllowAsyncShaderLoading();
    pRenderContext->SetAllowAsyncShaderLoading(false);
    EZ_SCOPE_EXIT(pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

    const ezGALResourceFormat::Enum tempTargetFormat = ezGALResourceFormat::RGBAUByteNormalized;
    const ezGALResourceFormat::Enum tempStencilFormat = ezGALResourceFormat::D24S8;
    const ezGALMSAASampleCount::Enum msaaSampleCount = ezGALMSAASampleCount::FourSamples;

    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_uiRenderTargetClearMask = 0x1;
    renderingSetup.m_bClearStencil = true;
    renderingSetup.m_bDiscardDepth = true;

    pRenderContext->BindShader(m_hShader);
    pRenderContext->BindConstantBuffer("ezRmlUiConstants", m_hConstantBuffer);

    for (auto& pCommandBuffer : submittedCommandBuffers)
    {
      const ezGALTexture* pTargetTexture = pDevice->GetTexture(pCommandBuffer->m_hTargetTexture);
      if (pTargetTexture == nullptr)
        continue;

      auto& textureDesc = pTargetTexture->GetDescription();
      ezRectFloat viewport(static_cast<float>(textureDesc.m_uiWidth), static_cast<float>(textureDesc.m_uiHeight));
      ezRectU32 scissorRect(0, 0, textureDesc.m_uiWidth, textureDesc.m_uiHeight);

      ezGALTextureHandle hTempTarget = pGpuResourcePool->GetRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempTargetFormat, msaaSampleCount);
      ezGALTextureHandle hTempStencil = pGpuResourcePool->GetRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempStencilFormat, msaaSampleCount);
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hTempTarget));
      renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(hTempStencil));

      pRenderContext->BeginRendering(renderingSetup, viewport, pCommandBuffer->m_sName, false);
      pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_NORMAL");

      pCommandEncoder->SetScissorRect(scissorRect);

      ezUInt32 uiCommandOffset = 0;
      while (uiCommandOffset < pCommandBuffer->m_Buffer.GetCount())
      {
        CommandType::Enum cmdType = pCommandBuffer->PeekCommandType(uiCommandOffset);
        switch (cmdType)
        {
          case CommandType::RenderGeometry:
          {
            auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderGeometry>(uiCommandOffset);

            ezRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiConstants>(m_hConstantBuffer);
            pConstants->UiTransform = cmd.m_Transform;
            pConstants->UiTranslation = cmd.m_Translation.GetAsVec4(0, 1);
            pConstants->TextureNeedsAlphaMultiplication = cmd.m_bValue;

            pRenderContext->BindMeshBuffer(cmd.m_CompiledGeometry.m_hVertexBuffer, cmd.m_CompiledGeometry.m_hIndexBuffer, &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

            pRenderContext->BindTexture2D("BaseTexture", pDevice->GetDefaultResourceView(cmd.m_hTexture));

            pRenderContext->DrawMeshBuffer().IgnoreResult();
          }
          break;

          case CommandType::SetScissorRegion:
          {
            auto& cmd = pCommandBuffer->ConsumeCommand<CommandSetScissorRegion>(uiCommandOffset);

            pCommandEncoder->SetScissorRect(cmd.m_ScissorRect);
          }
          break;

            EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }

      pRenderContext->EndRendering();

      pCommandEncoder->ResolveTexture(pCommandBuffer->m_hTargetTexture, ezGALTextureSubresource(), hTempTarget, ezGALTextureSubresource());

      pGpuResourcePool->ReturnRenderTarget(hTempTarget);
      pGpuResourcePool->ReturnRenderTarget(hTempStencil);

      FreeCommandBuffer(std::move(pCommandBuffer));
    }

    pDevice->EndCommands(pCommandEncoder);

    submittedCommandBuffers.Clear();
  }

  void RenderInterface::EndFrame()
  {
    ezUInt64 uiFrameCounter = ezRenderWorld::GetFrameCounter();

    EZ_LOCK(m_ReleasedCompiledGeometryMutex);

    while (!m_ReleasedCompiledGeometry.IsEmpty())
    {
      auto& releasedGeometry = m_ReleasedCompiledGeometry.PeekFront();

      if (releasedGeometry.m_uiFrame >= uiFrameCounter)
        break;

      FreeReleasedGeometry(releasedGeometry.m_Id);

      m_CompiledGeometry.Remove(releasedGeometry.m_Id);
      m_ReleasedCompiledGeometry.PopFront();
    }
  }

  void RenderInterface::FreeReleasedGeometry(GeometryId id)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(id, pGeometry))
      return;

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();
  }

  ezUniquePtr<CommandBuffer> RenderInterface::AllocateCommandBuffer()
  {
    if (m_FreeCommandBuffers.IsEmpty() == false)
    {
      ezUniquePtr<CommandBuffer> cmdBuffer = std::move(m_FreeCommandBuffers.PeekBack());
      m_FreeCommandBuffers.PopBack();
      return cmdBuffer;
    }

    return EZ_DEFAULT_NEW(CommandBuffer);
  }

  void RenderInterface::FreeCommandBuffer(ezUniquePtr<CommandBuffer>&& pBuffer)
  {
    pBuffer->Clear();

    m_FreeCommandBuffers.PushBack(std::move(pBuffer));
  }

  void RenderInterface::SubmitCommandBuffer(ezUniquePtr<CommandBuffer>&& pBuffer)
  {
    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[ezRenderWorld::GetDataIndexForExtraction()];
    submittedCommandBuffers.PushBack(std::move(pBuffer));
  }

} // namespace ezRmlUiInternal
