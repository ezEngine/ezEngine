#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

class ezRenderGraph;

using ezTexture2DResourceHandle = ezTypedResourceHandle<class ezTexture2DResource>;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

namespace ezRmlUiInternal
{
  struct GeometryId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static GeometryId FromRml(Rml::CompiledGeometryHandle hGeometry) { return GeometryId(static_cast<ezUInt32>(hGeometry)); }

    Rml::CompiledGeometryHandle ToRml() const { return m_Data; }
  };

  struct TextureId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static TextureId FromRml(Rml::TextureHandle hTexture) { return TextureId(static_cast<ezUInt32>(hTexture)); }

    Rml::TextureHandle ToRml() const { return m_Data; }
  };

  struct ShaderId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static ShaderId FromRml(Rml::CompiledShaderHandle hShader) { return ShaderId(static_cast<ezUInt32>(hShader)); }

    Rml::CompiledShaderHandle ToRml() const { return m_Data; }
  };

  struct ShaderType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Invalid,
      Gradient,
      Custom,

      Default = Invalid
    };
  };

  //////////////////////////////////////////////////////////////////////////

  struct CompiledGeometry;
  struct CommandBuffer;
  struct CommandRenderGeometry;

  class RenderInterface final : public Rml::RenderInterface
  {
  public:
    RenderInterface();
    virtual ~RenderInterface();

    // Interface implementation
    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    virtual void RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture) override;
    virtual void ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry) override;

    virtual Rml::TextureHandle LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource) override;
    virtual Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize) override;
    virtual void ReleaseTexture(Rml::TextureHandle hTexture) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(Rml::Rectanglei region) override;

    virtual void EnableClipMask(bool bEnable) override;
    virtual void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation) override;

    virtual void SetTransform(const Rml::Matrix4f* pTransform) override;

    virtual Rml::LayerHandle PushLayer() override;
    virtual void CompositeLayers(Rml::LayerHandle hSource, Rml::LayerHandle hDestination, Rml::BlendMode blendMode, Rml::Span<const Rml::CompiledFilterHandle> filters) override;
    virtual void PopLayer() override;

    virtual Rml::TextureHandle SaveLayerAsTexture() override;
    virtual Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

    virtual Rml::CompiledFilterHandle CompileFilter(const Rml::String& sName, const Rml::Dictionary& parameters) override;
    virtual void ReleaseFilter(Rml::CompiledFilterHandle hFilter) override;

    virtual Rml::CompiledShaderHandle CompileShader(const Rml::String& sName, const Rml::Dictionary& parameters) override;
    virtual void RenderShader(Rml::CompiledShaderHandle hShader, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture) override;
    virtual void ReleaseShader(Rml::CompiledShaderHandle hShader) override;

    // EZ specific functions
    void BeginExtraction(const ezHashedString& sName, ezGALTextureHandle hTargetTexture);
    void EndExtraction();

  private:
    void GALEventHandler(const ezGALDeviceEvent& e);
    void BeginFrame();
    void EndFrame();
    void FreeReleasedGeometry(GeometryId id);

    void FillRenderCommand(CommandRenderGeometry& out_cmd, Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture);
    ezUniquePtr<CommandBuffer> AllocateCommandBuffer();
    void FreeCommandBuffer(ezUniquePtr<CommandBuffer>&& pBuffer);
    void SubmitCommandBuffer(ezUniquePtr<CommandBuffer>&& pBuffer);

    ezIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;

    struct ReleasedGeometry
    {
      ezUInt64 m_uiFrame;
      GeometryId m_Id;
    };

    ezMutex m_ReleasedCompiledGeometryMutex;
    ezDeque<ReleasedGeometry> m_ReleasedCompiledGeometry;

    struct TextureInfo
    {
      ezTexture2DResourceHandle m_hTexture;
      bool m_bHasPremultipliedAlpha = false;
    };

    ezIdTable<TextureId, TextureInfo> m_Textures;
    ezTexture2DResourceHandle m_hNoiseTexture;
    ezTexture2DResourceHandle m_hFallbackTexture;

    struct ShaderInfo
    {
      ezShaderResourceHandle m_hShader;
      ezGALBufferHandle m_hAdditionalConstantBuffer;
      ezEnum<ShaderType> m_Type;
    };

    ezIdTable<ShaderId, ShaderInfo> m_Shaders;

    ezMat4 m_mProjection = ezMat4::MakeIdentity();
    ezMat4 m_mTransform = ezMat4::MakeIdentity();
    bool m_bUseStencilTest = false;

    ezDynamicArray<ezUniquePtr<CommandBuffer>> m_FreeCommandBuffers;
    ezDynamicArray<ezUniquePtr<CommandBuffer>> m_SubmittedCommandBuffers[2];

    ezUniquePtr<CommandBuffer> m_pCurrentCommandBuffer;

    ezShaderResourceHandle m_hMainShader;
    ezConstantBufferStorageHandle m_hMainConstantBuffer;
    ezSmallArray<ezGALVertexAttribute, 3> m_VertexAttributes;

    ezSharedPtr<ezRenderGraph> m_pRenderGraph;
  };
} // namespace ezRmlUiInternal
