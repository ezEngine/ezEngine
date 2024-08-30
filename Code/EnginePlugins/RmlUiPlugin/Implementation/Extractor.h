#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>

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

  class Extractor final : public Rml::RenderInterface
  {
  public:
    Extractor();
    virtual ~Extractor();

    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    virtual void RenderGeometry(Rml::CompiledGeometryHandle hGeometry, Rml::Vector2f translation, Rml::TextureHandle hTexture) override;
    virtual void ReleaseGeometry(Rml::CompiledGeometryHandle hGeometry) override;

    virtual Rml::TextureHandle LoadTexture(Rml::Vector2i& out_textureSize, const Rml::String& sSource) override;
    virtual Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i sourceSize) override;
    virtual void ReleaseTexture(Rml::TextureHandle hTexture) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(Rml::Rectanglei region) override;    

    virtual void SetTransform(const Rml::Matrix4f* pTransform) override;

    void BeginExtraction(const ezVec2I32& vOffset);
    void EndExtraction();

    ezRenderData* GetRenderData();

  private:
    void EndFrame(const ezGALDeviceEvent& e);
    void FreeReleasedGeometry(GeometryId id);

    ezIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;

    struct ReleasedGeometry
    {
      ezUInt64 m_uiFrame;
      GeometryId m_Id;
    };

    ezDeque<ReleasedGeometry> m_ReleasedCompiledGeometry;

    ezIdTable<TextureId, ezTexture2DResourceHandle> m_Textures;
    ezTexture2DResourceHandle m_hFallbackTexture;

    ezVec2 m_vOffset = ezVec2::MakeZero();

    ezMat4 m_mTransform = ezMat4::MakeIdentity();
    ezRectFloat m_ScissorRect = ezRectFloat(0, 0);
    bool m_bEnableScissorRect = false;

    ezDynamicArray<Batch> m_Batches;
  };
} // namespace ezRmlUiInternal
