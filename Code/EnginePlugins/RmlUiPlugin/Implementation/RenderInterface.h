#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/IdTable.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

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

  //////////////////////////////////////////////////////////////////////////

  struct CompiledGeometry;
  struct CommandBuffer;

  class RenderInterface final : public Rml::RenderInterface
  {
  public:
    RenderInterface();
    virtual ~RenderInterface();

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

    void BeginExtraction(const ezHashedString& sName, ezGALTextureHandle hTargetTexture);
    void EndExtraction();

  private:
    void GALEventHandler(const ezGALDeviceEvent& e);
    void BeginFrame();
    void EndFrame();
    void FreeReleasedGeometry(GeometryId id);

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
    ezTexture2DResourceHandle m_hFallbackTexture;

    ezMat4 m_mTransform = ezMat4::MakeIdentity();

    ezDynamicArray<ezUniquePtr<CommandBuffer>> m_FreeCommandBuffers;
    ezDynamicArray<ezUniquePtr<CommandBuffer>> m_SubmittedCommandBuffers[2];

    ezUniquePtr<CommandBuffer> m_pCurrentCommandBuffer;

    ezShaderResourceHandle m_hShader;
    ezConstantBufferStorageHandle m_hConstantBuffer;
    ezVertexDeclarationInfo m_VertexDeclarationInfo;
  };
} // namespace ezRmlUiInternal
