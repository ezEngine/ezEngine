

#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderGraph/RenderGraphUtils.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RmlUiPlugin/Implementation/RenderInterface.h>

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
      RenderShader,
      SetScissorRegion,
      RenderToClipMask,

      Default = Invalid
    };
  };

  struct alignas(4) CommandHeader
  {
    ezEnum<CommandType> m_Type;
    bool m_bNeedsPremultipliedAlpha = false;
    bool m_bUseStencilTest = false;
    ezUInt8 m_uiSize = 0;
  };

  struct CommandRenderGeometry : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::RenderGeometry;

    CompiledGeometry m_CompiledGeometry;
    ezGALTextureHandle m_hTexture;
    ezMat4 m_Transform = ezMat4::MakeIdentity();
    ezVec2 m_Translation = ezVec2::MakeZero();
  };

  struct CommandRenderShader : CommandRenderGeometry
  {
    static constexpr CommandType::Enum Type = CommandType::RenderShader;

    ezShaderResourceHandle m_hShader;
    ezGALBufferHandle m_hAdditionalConstantBuffer;
    ezEnum<ShaderType> m_ShaderType;
  };

  struct CommandSetScissorRegion : CommandHeader
  {
    static constexpr CommandType::Enum Type = CommandType::SetScissorRegion;

    ezRectU32 m_ScissorRect = {};
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
    CommandBuffer() = default;
    ~CommandBuffer()
    {
      Clear();
    }

    ezHashedString m_sName;
    ezDynamicArray<ezUInt8> m_Buffer;
    ezGALTextureHandle m_hTargetTexture;
    ezUInt32 m_uiTargetWidth = 0;
    ezUInt32 m_uiTargetHeight = 0;

    template <typename T>
    T& AddCommand()
    {
      static_assert(sizeof(T) <= 255, "Command size must fit into a single byte");

      T& cmd = *reinterpret_cast<T*>(m_Buffer.ExpandBy(sizeof(T)));
      cmd.m_Type = T::Type;
      cmd.m_uiSize = sizeof(T);
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

    const CommandHeader& PeekCommandHeader(ezUInt32 uiOffset) const
    {
      return *reinterpret_cast<const CommandHeader*>(m_Buffer.GetData() + uiOffset);
    }

    void Clear()
    {
      ezUInt32 uiOffset = 0;
      while (uiOffset < m_Buffer.GetCount())
      {
        const CommandHeader& header = PeekCommandHeader(uiOffset);
        if (header.m_Type == CommandType::RenderShader)
        {
          auto& cmd = *reinterpret_cast<CommandRenderShader*>(m_Buffer.GetData() + uiOffset);

          // Prevent resource leak by explicit invalidation since no destructors are called for command structs when the buffer is cleared below.
          cmd.m_hShader.Invalidate();
        }

        uiOffset += header.m_uiSize;
      }

      m_Buffer.Clear();
    }
  };

  //////////////////////////////////////////////////////////////////////////

  RenderInterface::RenderInterface()
  {
    ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&RenderInterface::GALEventHandler, this));

    m_hNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("{ ac614d7c-2b31-4a7b-aa0c-c5d8200b7b89 }"); // BlueNoise
    m_hFallbackTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
    m_hMainShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/RmlUi.ezShader");
    m_hMainConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezRmlUiConstants>();

    // Setup the vertex declaration
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::Position;
      va.m_eFormat = ezGALResourceFormat::XYFloat;
      va.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_Position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = ezGALResourceFormat::UVFloat;
      va.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_TexCoord);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::Color0;
      va.m_eFormat = ezGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(ezRmlUiInternal::Vertex, m_Color);
    }

    m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("RmlUi", ezRenderGraphPhase::PreRender);
  }

  RenderInterface::~RenderInterface()
  {
    ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&RenderInterface::GALEventHandler, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }
    m_CompiledGeometry.Clear();

    m_Textures.Clear();
    m_hFallbackTexture.Invalidate();

    m_Shaders.Clear();
    m_hMainShader.Invalidate();
  }

  Rml::CompiledGeometryHandle RenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
  {
    const ezUInt32 uiNumVertices = static_cast<ezUInt32>(vertices.size());
    const ezUInt32 uiNumIndices = static_cast<ezUInt32>(indices.size());

    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = uiNumIndices / 3;

    // vertices
    {
      ezTempArray<Vertex> vertexStorage;
      vertexStorage.SetCountUninitialized(uiNumVertices);

      for (ezUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = vertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = ezRmlUiConversionUtils::ToVec2(srcVertex.position);
        destVertex.m_TexCoord = ezRmlUiConversionUtils::ToVec2(srcVertex.tex_coord);
        destVertex.m_Color = ezRmlUiConversionUtils::ToColor(srcVertex.colour);
      }

      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;
      desc.m_ResourceAccess.m_bImmutable = true;

      geometry.m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezUInt32);
      desc.m_uiTotalSize = uiNumIndices * desc.m_uiStructSize;
      desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;
      desc.m_ResourceAccess.m_bImmutable = true;

      geometry.m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezMakeArrayPtr(indices.data(), uiNumIndices).ToByteArray());
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void RenderInterface::RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderGeometry>();
    FillRenderCommand(cmd, hGeometry, translation, hTexture);
  }

  void RenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry)
  {
    EZ_LOCK(m_ReleasedCompiledGeometryMutex);

    m_ReleasedCompiledGeometry.PushBack({ezRenderWorld::GetFrameCounter(), GeometryId::FromRml(hGeometry)});
  }

  Rml::TextureHandle RenderInterface::LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource)
  {
    ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(ezRmlUiConversionUtils::ToStringView(sSource));

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
    m_bUseStencilTest = bEnable;
  }

  void RenderInterface::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderToClipMask>();
    cmd.m_Operation = operation;
    cmd.m_CompiledGeometry = m_CompiledGeometry[GeometryId::FromRml(hGeometry)];
    cmd.m_Transform = m_mTransform;
    cmd.m_Translation = ezRmlUiConversionUtils::ToVec2(translation);
  }

  void RenderInterface::SetTransform(const Rml::Matrix4f* pTransform)
  {
    if (pTransform != nullptr)
    {
      constexpr bool bColumnMajor = std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value;

      if (bColumnMajor)
        m_mTransform = m_mProjection * ezMat4::MakeFromColumnMajorArray(pTransform->data());
      else
        m_mTransform = m_mProjection * ezMat4::MakeFromRowMajorArray(pTransform->data());
    }
    else
    {
      m_mTransform = m_mProjection;
    }
  }

  Rml::LayerHandle RenderInterface::PushLayer()
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  void RenderInterface::CompositeLayers(Rml::LayerHandle hSource, Rml::LayerHandle hDestination, Rml::BlendMode blendMode, Rml::Span<const Rml::CompiledFilterHandle> filters)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  void RenderInterface::PopLayer()
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::TextureHandle RenderInterface::SaveLayerAsTexture()
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  Rml::CompiledFilterHandle RenderInterface::SaveLayerAsMaskImage()
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  Rml::CompiledFilterHandle RenderInterface::CompileFilter(const Rml::String& sName, const Rml::Dictionary& parameters)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return {};
  }

  void RenderInterface::ReleaseFilter(Rml::CompiledFilterHandle hFilter)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::CompiledShaderHandle RenderInterface::CompileShader(const Rml::String& sName, const Rml::Dictionary& parameters)
  {
    auto ApplyColorStopList = [](ezRmlUiAdditionalConstants& out_data, const Rml::Dictionary& shader_parameters)
    {
      auto it = shader_parameters.find("color_stop_list");
      EZ_ASSERT_DEV(it != shader_parameters.end() && it->second.GetType() == Rml::Variant::COLORSTOPLIST, "Color stop list not found or invalid type");
      const Rml::ColorStopList& color_stop_list = it->second.GetReference<Rml::ColorStopList>();
      const ezUInt32 uiNumStops = ezMath::Min(static_cast<ezUInt32>(color_stop_list.size()), GRADIENT_MAX_NUM_STOPS);

      out_data.GradientNumStops = uiNumStops;
      float* pStopPositions = &out_data.GradientStopPositions[0].x;
      for (ezUInt32 i = 0; i < uiNumStops; i++)
      {
        const Rml::ColorStop& stop = color_stop_list[i];
        EZ_ASSERT_DEV(stop.position.unit == Rml::Unit::NUMBER, "Invalid color stop position unit");
        pStopPositions[i] = stop.position.number;
        out_data.GradientStopColors[i] = ezRmlUiConversionUtils::ToColor(stop.color);
      }
    };

    auto CreateGradientConstantBuffer = [](const ezRmlUiAdditionalConstants& data) -> ezGALBufferHandle
    {
      ezGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = 0;
      bufferDesc.m_BufferFlags = ezGALBufferUsageFlags::ConstantBuffer;
      bufferDesc.m_ResourceAccess.m_bImmutable = true;
      bufferDesc.m_uiTotalSize = sizeof(ezRmlUiAdditionalConstants);

      return ezGALDevice::GetDefaultDevice()->CreateBuffer(bufferDesc, ezMakeByteArrayPtr(&data, 1));
    };

    ShaderInfo shaderInfo;
    shaderInfo.m_hShader = m_hMainShader;

    const bool repeating = Rml::Get(parameters, "repeating", false);

    ezRmlUiAdditionalConstants data;
    ezMemoryUtils::ZeroFill(&data);

    if (sName == "linear-gradient")
    {
      data.GradientFunc = repeating ? GRADIENT_REPEATING_LINEAR : GRADIENT_LINEAR;
      data.GradientParams0 = ezRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "p0", Rml::Vector2f(0.f)));
      data.GradientParams1 = ezRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "p1", Rml::Vector2f(0.f))) - data.GradientParams0;
      ApplyColorStopList(data, parameters);

      shaderInfo.m_hAdditionalConstantBuffer = CreateGradientConstantBuffer(data);
      shaderInfo.m_Type = ShaderType::Gradient;
    }
    else if (sName == "radial-gradient")
    {
      data.GradientFunc = repeating ? GRADIENT_REPEATING_RADIAL : GRADIENT_RADIAL;
      data.GradientParams0 = ezRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "center", Rml::Vector2f(0.f)));
      data.GradientParams1 = ezRmlUiConversionUtils::ToVec2(Rml::Vector2f(1.f) / Rml::Get(parameters, "radius", Rml::Vector2f(1.f)));
      ApplyColorStopList(data, parameters);

      shaderInfo.m_hAdditionalConstantBuffer = CreateGradientConstantBuffer(data);
      shaderInfo.m_Type = ShaderType::Gradient;
    }
    else if (sName == "conic-gradient")
    {
      data.GradientFunc = repeating ? GRADIENT_REPEATING_CONIC : GRADIENT_CONIC;
      data.GradientParams0 = ezRmlUiConversionUtils::ToVec2(Rml::Get(parameters, "center", Rml::Vector2f(0.f)));

      const ezAngle angle = ezAngle::MakeFromRadian(Rml::Get(parameters, "angle", 0.f));
      data.GradientParams1 = ezVec2(ezMath::Cos(angle), ezMath::Sin(angle));
      ApplyColorStopList(data, parameters);

      shaderInfo.m_hAdditionalConstantBuffer = CreateGradientConstantBuffer(data);
      shaderInfo.m_Type = ShaderType::Gradient;
    }


    if (shaderInfo.m_Type != ShaderType::Invalid)
    {
      return m_Shaders.Insert(shaderInfo).ToRml();
    }

    ezLog::Warning("Unsupported shader type '{}'.", sName.c_str());
    return {};
  }

  void RenderInterface::RenderShader(Rml::CompiledShaderHandle hShader, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    auto& cmd = m_pCurrentCommandBuffer->AddCommand<CommandRenderShader>();
    FillRenderCommand(cmd, hGeometry, translation, hTexture);

    ShaderInfo shaderInfo;
    EZ_VERIFY(m_Shaders.TryGetValue(ShaderId::FromRml(hShader), shaderInfo), "Invalid shader handle");

    cmd.m_hShader = shaderInfo.m_hShader;
    cmd.m_hAdditionalConstantBuffer = shaderInfo.m_hAdditionalConstantBuffer;
    cmd.m_ShaderType = shaderInfo.m_Type;
  }

  void RenderInterface::ReleaseShader(Rml::CompiledShaderHandle hShader)
  {
    ShaderId shaderId = ShaderId::FromRml(hShader);
    if (shaderId.IsInvalidated() == false)
    {
      ShaderInfo shaderInfo;
      EZ_VERIFY(m_Shaders.Remove(shaderId, &shaderInfo), "Invalid shader handle");

      ezGALDevice::GetDefaultDevice()->DestroyBuffer(shaderInfo.m_hAdditionalConstantBuffer);
    }
  }

  void RenderInterface::BeginExtraction(const ezHashedString& sName, ezGALTextureHandle hTargetTexture)
  {
    m_pCurrentCommandBuffer = AllocateCommandBuffer();
    m_pCurrentCommandBuffer->m_sName = sName;
    m_pCurrentCommandBuffer->m_hTargetTexture = hTargetTexture;

    const ezGALTexture* pTargetTexture = ezGALDevice::GetDefaultDevice()->GetTexture(hTargetTexture);
    const ezUInt32 uiTargetWidth = pTargetTexture->GetDescription().m_uiWidth;
    const ezUInt32 uiTargetHeight = pTargetTexture->GetDescription().m_uiHeight;

    m_pCurrentCommandBuffer->m_uiTargetWidth = uiTargetWidth;
    m_pCurrentCommandBuffer->m_uiTargetHeight = uiTargetHeight;

    m_mProjection = ezGraphicsUtils::CreateOrthographicProjectionMatrix(0.0f, (float)uiTargetWidth, (float)uiTargetHeight, 0.0f, -1.0f, 1.0f);
    SetTransform(nullptr);
    m_bUseStencilTest = false;
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
    else if (e.m_Type == ezGALDeviceEvent::AfterEndFrame)
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

    const ezGALResourceFormat::Enum tempTargetFormat = ezGALResourceFormat::RGBAUByteNormalized;
    const ezGALResourceFormat::Enum tempStencilFormat = ezGALResourceFormat::D24S8;
    const ezGALMSAASampleCount::Enum msaaSampleCount = ezGALMSAASampleCount::FourSamples;

    m_pRenderGraph->Reset();

    for (auto& pCommandBuffer : submittedCommandBuffers)
    {
      const ezGALTexture* pTargetTexture = pDevice->GetTexture(pCommandBuffer->m_hTargetTexture);
      if (pTargetTexture == nullptr)
      {
        FreeCommandBuffer(std::move(pCommandBuffer));
        continue;
      }

      auto& textureDesc = pTargetTexture->GetDescription();

      ezRenderGraphTextureHandle hTarget = m_pRenderGraph->ImportTexture(pCommandBuffer->m_hTargetTexture);
      ezGALTextureHandle hTargetTexture = pCommandBuffer->m_hTargetTexture;

      ezGALTextureCreationDescription tempColorDesc;
      tempColorDesc.SetAsRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempTargetFormat, msaaSampleCount);
      ezRenderGraphTextureHandle hTempColor = m_pRenderGraph->CreateTexture(tempColorDesc);

      ezGALTextureCreationDescription tempStencilDesc;
      tempStencilDesc.SetAsRenderTarget(textureDesc.m_uiWidth, textureDesc.m_uiHeight, tempStencilFormat, msaaSampleCount);
      ezRenderGraphTextureHandle hTempStencil = m_pRenderGraph->CreateTexture(tempStencilDesc);

      {
        auto pass = m_pRenderGraph->AddGraphicsPass(pCommandBuffer->m_sName);
        pass.AddColorTarget(hTempColor, {}, ezGALRenderTargetLoadOp::Clear);
        pass.SetClearColor(0);
        pass.AddDepthStencilTarget(hTempStencil, {},
          ezGALRenderTargetLoadOp::Clear, ezGALRenderTargetStoreOp::Discard,
          ezGALRenderTargetLoadOp::Clear, ezGALRenderTargetStoreOp::Discard);
        pass.SetClearDepth();
        pass.SetClearStencil();

        pass.SetExecuteCallback([this, pCommandBuffer = pCommandBuffer.Borrow()](const ezRenderGraphContext& ctx)
          {
            auto* pCommandEncoder = ctx.GetCommandEncoder();
            auto* pRenderContext = ctx.GetRenderContext();

            bool bAllowAsyncShaderLoading = pRenderContext->GetAllowAsyncShaderLoading();
            pRenderContext->SetAllowAsyncShaderLoading(false);
            EZ_SCOPE_EXIT(pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

            ezRectFloat viewport(static_cast<float>(pCommandBuffer->m_uiTargetWidth), static_cast<float>(pCommandBuffer->m_uiTargetHeight));
            pCommandEncoder->SetViewport(viewport);

            ezRectU32 scissorRect(0, 0, pCommandBuffer->m_uiTargetWidth, pCommandBuffer->m_uiTargetHeight);
            pCommandEncoder->SetScissorRect(scissorRect);

            pRenderContext->BindShader(m_hMainShader);
            ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
            bindGroup.BindBuffer("ezRmlUiConstants", m_hMainConstantBuffer);
            bindGroup.BindTexture("NoiseTexture", m_hNoiseTexture);

            auto Draw = [&](const CommandRenderGeometry& cmd, bool bGradient)
            {
              pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", cmd.m_bUseStencilTest ? ezTempHashedString("RMLUI_MODE_STENCIL_TEST") : ezTempHashedString("RMLUI_MODE_NORMAL"));
              pRenderContext->SetShaderPermutationVariable("RMLUI_GRADIENT", bGradient ? ezTempHashedString("TRUE") : ezTempHashedString("FALSE"));

              ezRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiConstants>(m_hMainConstantBuffer);
              pConstants->UiTransform = cmd.m_Transform;
              pConstants->UiTranslation = cmd.m_Translation;
              pConstants->TextureNeedsAlphaMultiplication = cmd.m_bNeedsPremultipliedAlpha;

              pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&cmd.m_CompiledGeometry.m_hVertexBuffer, 1), cmd.m_CompiledGeometry.m_hIndexBuffer, m_VertexAttributes, ezGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

              bindGroup.BindTexture("BaseTexture", cmd.m_hTexture);

              pRenderContext->DrawMeshBuffer().IgnoreResult();
            };

            ezUInt32 uiCommandOffset = 0;
            while (uiCommandOffset < pCommandBuffer->m_Buffer.GetCount())
            {
              CommandType::Enum cmdType = pCommandBuffer->PeekCommandType(uiCommandOffset);
              switch (cmdType)
              {
                case CommandType::RenderGeometry:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderGeometry>(uiCommandOffset);

                  pRenderContext->BindShader(m_hMainShader);
                  Draw(cmd, false);
                }
                break;

                case CommandType::RenderShader:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderShader>(uiCommandOffset);

                  pRenderContext->BindShader(cmd.m_hShader);
                  bindGroup.BindBuffer("ezRmlUiAdditionalConstants", cmd.m_hAdditionalConstantBuffer);
                  Draw(cmd, cmd.m_ShaderType == ShaderType::Gradient);
                }
                break;

                case CommandType::SetScissorRegion:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandSetScissorRegion>(uiCommandOffset);

                  pCommandEncoder->SetScissorRect(cmd.m_ScissorRect);
                }
                break;

                case CommandType::RenderToClipMask:
                {
                  auto& cmd = pCommandBuffer->ConsumeCommand<CommandRenderToClipMask>(uiCommandOffset);

                  EZ_ASSERT_DEV(cmd.m_Operation == Rml::ClipMaskOperation::Set, "Only 'Set' clip mask operation is implemented.");
                  pRenderContext->SetShaderPermutationVariable("RMLUI_MODE", "RMLUI_MODE_STENCIL_SET");

                  ezRmlUiConstants* pConstants = pRenderContext->GetConstantBufferData<ezRmlUiConstants>(m_hMainConstantBuffer);
                  pConstants->UiTransform = cmd.m_Transform;
                  pConstants->UiTranslation = cmd.m_Translation;

                  pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&cmd.m_CompiledGeometry.m_hVertexBuffer, 1), cmd.m_CompiledGeometry.m_hIndexBuffer, m_VertexAttributes, ezGALPrimitiveTopology::Triangles, cmd.m_CompiledGeometry.m_uiTriangleCount);

                  pRenderContext->DrawMeshBuffer().IgnoreResult();
                }
                break;

                default:
                {
                  EZ_ASSERT_ALWAYS(false, "RmlUI: Command Type '{}' is not implemented.", cmdType);
                  break;
                }
              }
            } //
          });
      }

      // Transfer pass: resolve MSAA to target texture
      {
        auto resolvePass = m_pRenderGraph->AddTransferPass("RmlUi Resolve");
        resolvePass.ReadTexture(hTempColor, {}, ezGALResourceState::ResolveSource);
        resolvePass.WriteTexture(hTarget, {}, ezGALResourceState::ResolveDestination);
        resolvePass.SetExecuteCallback(
          [hTempColor, hTarget](const ezRenderGraphContext& ctx)
          {
            ctx.GetCommandEncoder()->ResolveTexture(
              ctx.ResolveTexture(hTarget), ezGALTextureSubresource(),
              ctx.ResolveTexture(hTempColor), ezGALTextureSubresource());
          });
      }

      // Optional: generate mipmaps for the target
      if (textureDesc.m_uiMipLevelCount > 1 && textureDesc.m_TextureFlags.IsSet(ezGALTextureUsageFlags::RenderTarget))
      {
        ezRenderGraphUtils::GenerateMipMaps(hTargetTexture, {}, *m_pRenderGraph);
      }
    }

    ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
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

    auto& submittedCommandBuffers = m_SubmittedCommandBuffers[ezRenderWorld::GetDataIndexForRendering()];
    for (auto& pCommandBuffer : submittedCommandBuffers)
    {
      if (pCommandBuffer != nullptr)
      {
        FreeCommandBuffer(std::move(pCommandBuffer));
      }
    }
    submittedCommandBuffers.Clear();
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

  void RenderInterface::FillRenderCommand(CommandRenderGeometry& out_cmd, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture)
  {
    EZ_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(hGeometry), out_cmd.m_CompiledGeometry), "Invalid compiled geometry");

    TextureInfo textureInfo;
    if (m_Textures.TryGetValue(TextureId::FromRml(hTexture), textureInfo) == false)
    {
      textureInfo.m_hTexture = m_hFallbackTexture;
    }

    ezResourceLock<ezTexture2DResource> pTexture(textureInfo.m_hTexture, ezResourceAcquireMode::BlockTillLoaded);
    out_cmd.m_hTexture = pTexture->GetGALTexture();

    out_cmd.m_Transform = m_mTransform;
    out_cmd.m_Translation = ezRmlUiConversionUtils::ToVec2(translation);
    out_cmd.m_bNeedsPremultipliedAlpha = textureInfo.m_bHasPremultipliedAlpha == false;
    out_cmd.m_bUseStencilTest = m_bUseStencilTest;
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
