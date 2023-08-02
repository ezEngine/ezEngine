#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>

namespace ezRmlUiInternal
{
  struct GeometryId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static GeometryId FromRml(Rml::CompiledGeometryHandle geometry) { return GeometryId(static_cast<ezUInt32>(geometry)); }

    Rml::CompiledGeometryHandle ToRml() const { return m_Data; }
  };

  struct TextureId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static TextureId FromRml(Rml::TextureHandle texture) { return TextureId(static_cast<ezUInt32>(texture)); }

    Rml::TextureHandle ToRml() const { return m_Data; }
  };

  //////////////////////////////////////////////////////////////////////////

  class Extractor final : public Rml::RenderInterface
  {
  public:
    Extractor();
    virtual ~Extractor();

    virtual void RenderGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override;

    virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle texture) override;
    virtual void RenderCompiledGeometry(Rml::CompiledGeometryHandle geometry_handle, const Rml::Vector2f& translation) override;
    virtual void ReleaseCompiledGeometry(Rml::CompiledGeometryHandle geometry_handle) override;

    virtual void EnableScissorRegion(bool bEnable) override;
    virtual void SetScissorRegion(int x, int y, int iWidth, int iHeight) override;

    virtual bool LoadTexture(Rml::TextureHandle& ref_texture_handle, Rml::Vector2i& ref_texture_dimensions, const Rml::String& sSource) override;
    virtual bool GenerateTexture(Rml::TextureHandle& ref_texture_handle, const Rml::byte* pSource, const Rml::Vector2i& source_dimensions) override;
    virtual void ReleaseTexture(Rml::TextureHandle texture_handle) override;

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

    ezMat4 m_mTransform = ezMat4::IdentityMatrix();
    ezRectFloat m_ScissorRect = ezRectFloat(0, 0);
    bool m_bEnableScissorRect = false;

    ezDynamicArray<Batch> m_Batches;
  };
} // namespace ezRmlUiInternal
