#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>

namespace ezRmlUiInternal
{
  struct GeometryId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static GeometryId FromRml(Rml::Core::CompiledGeometryHandle hGeometry)
    {
      return GeometryId(static_cast<ezUInt32>(hGeometry));
    }

    Rml::Core::CompiledGeometryHandle ToRml() const
    {
      return m_Data;
    }
  };

  struct TextureId : public ezGenericId<24, 8>
  {
    using ezGenericId::ezGenericId;

    static TextureId FromRml(Rml::Core::TextureHandle hTexture)
    {
      return TextureId(static_cast<ezUInt32>(hTexture));
    }

    Rml::Core::TextureHandle ToRml() const
    {
      return m_Data;
    }
  };

  //////////////////////////////////////////////////////////////////////////

  class Extractor : public Rml::Core::RenderInterface
  {
  public:
    Extractor();
    virtual ~Extractor();

    virtual void RenderGeometry(Rml::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::Core::TextureHandle texture, const Rml::Core::Vector2f& translation) override;

    virtual Rml::Core::CompiledGeometryHandle CompileGeometry(Rml::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::Core::TextureHandle texture) override;
    virtual void RenderCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry, const Rml::Core::Vector2f& translation) override;
    virtual void ReleaseCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry) override;

    virtual void EnableScissorRegion(bool enable) override;
    virtual void SetScissorRegion(int x, int y, int width, int height) override;

    virtual bool LoadTexture(Rml::Core::TextureHandle& texture_handle, Rml::Core::Vector2i& texture_dimensions, const Rml::Core::String& source) override;
    virtual bool GenerateTexture(Rml::Core::TextureHandle& texture_handle, const Rml::Core::byte* source, const Rml::Core::Vector2i& source_dimensions) override;
    virtual void ReleaseTexture(Rml::Core::TextureHandle texture) override;

    virtual void SetTransform(const Rml::Core::Matrix4f* transform) override;

    void BeginExtraction();
    void EndExtraction();

    ezRenderData* GetRenderData();

  private:
    ezIdTable<GeometryId, CompiledGeometry> m_CompiledGeometry;
    ezIdTable<TextureId, ezTexture2DResourceHandle> m_Textures;
    ezTexture2DResourceHandle m_hFallbackTexture;

    ezMat4 m_Transform = ezMat4::IdentityMatrix();
    ezRectFloat m_ScissorRect = ezRectFloat(0, 0);
    bool m_bEnableScissorRect = false;

    ezDynamicArray<Batch> m_Batches[2];

    ezRmlUiRenderData* m_pCurrentRenderData = nullptr;
  };
} // namespace ezRmlUiInternal
