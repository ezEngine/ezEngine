#include <RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RmlUiPlugin/Implementation/Extractor.h>

namespace ezRmlUiInternal
{
  Extractor::Extractor()
  {
    m_hFallbackTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
  }

  Extractor::~Extractor() = default;

  void Extractor::RenderGeometry(Rml::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::Core::TextureHandle texture, const Rml::Core::Vector2f& translation)
  {
    // Should never be called since we are using compiled geometry
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::Core::CompiledGeometryHandle Extractor::CompileGeometry(Rml::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::Core::TextureHandle texture)
  {
    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = num_indices / 3;

    // vertices
    {
      ezDynamicArray<Vertex> vertexStorage(ezFrameAllocator::GetCurrentAllocator());
      vertexStorage.SetCountUninitialized(num_vertices);

      for (ezUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = vertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = ezVec3(srcVertex.position.x, srcVertex.position.y, 0);
        destVertex.m_TexCoord = ezVec2(srcVertex.tex_coord.x, srcVertex.tex_coord.y);
        destVertex.m_Color = reinterpret_cast<const ezColorGammaUB&>(srcVertex.colour);
      }

      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(Vertex);
      desc.m_uiTotalSize = vertexStorage.GetCount() * desc.m_uiStructSize;
      desc.m_BufferType = ezGALBufferType::VertexBuffer;

      geometry.m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, vertexStorage.GetByteArrayPtr());
    }

    // indices
    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezUInt32);
      desc.m_uiTotalSize = num_indices * desc.m_uiStructSize;
      desc.m_BufferType = ezGALBufferType::IndexBuffer;

      geometry.m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezMakeArrayPtr(indices, num_indices).ToByteArray());
    }

    // texture
    {
      ezTexture2DResourceHandle* phTexture = nullptr;
      if (m_Textures.TryGetValue(TextureId::FromRml(texture), phTexture))
      {
        geometry.m_hTexture = *phTexture;
      }
      else
      {
        geometry.m_hTexture = m_hFallbackTexture;
      }
    }

    return m_CompiledGeometry.Insert(std::move(geometry)).ToRml();
  }

  void Extractor::RenderCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry, const Rml::Core::Vector2f& translation)
  {
    EZ_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(geometry), m_CurrentBatch.m_CompiledGeometry), "Invalid compiled geometry");

    ezMat4 translationMat;
    translationMat.SetTranslationMatrix(ezVec3(translation.x, translation.y, 0.0f));
    m_CurrentBatch.m_Transform = translationMat;

    if (m_bEnableScissor == false)
    {
      m_CurrentBatch.m_ScissorRect.width = 0.0f;
      m_CurrentBatch.m_ScissorRect.height = 0.0f;
    }

    m_Batches[ezRenderWorld::GetDataIndexForExtraction()].PushBack(m_CurrentBatch);
  }

  void Extractor::ReleaseCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry)
  {
    //TODO
  }

  void Extractor::EnableScissorRegion(bool enable)
  {
    m_bEnableScissor = enable;
  }

  void Extractor::SetScissorRegion(int x, int y, int width, int height)
  {
    m_CurrentBatch.m_ScissorRect = ezRectFloat(x, y, width, height);
  }

  bool Extractor::LoadTexture(Rml::Core::TextureHandle& texture_handle, Rml::Core::Vector2i& texture_dimensions, const Rml::Core::String& source)
  {
    throw std::logic_error("The method or operation is not implemented.");
  }

  bool Extractor::GenerateTexture(Rml::Core::TextureHandle& texture_handle, const Rml::Core::byte* source, const Rml::Core::Vector2i& source_dimensions)
  {
    throw std::logic_error("The method or operation is not implemented.");
  }

  void Extractor::ReleaseTexture(Rml::Core::TextureHandle texture)
  {
    //TODO
  }

  void Extractor::SetTransform(const Rml::Core::Matrix4f* transform)
  {
    if (transform != nullptr)
    {
      constexpr ezMatrixLayout::Enum matrixLayout = std::is_same<Rml::Core::Matrix4f, Rml::Core::ColumnMajorMatrix4f>::value ? ezMatrixLayout::ColumnMajor : ezMatrixLayout::RowMajor;
      m_CurrentBatch.m_Transform.SetFromArray(transform->data(), matrixLayout);
    }
    else
    {
      m_CurrentBatch.m_Transform.SetIdentity();
    }
  }

  void Extractor::BeginExtraction()
  {
    m_Batches[ezRenderWorld::GetDataIndexForExtraction()].Clear();
    m_pCurrentRenderData = nullptr;
  }

  void Extractor::EndExtraction()
  {
    auto& batches = m_Batches[ezRenderWorld::GetDataIndexForExtraction()];

    if (batches.IsEmpty() == false)
    {
      m_pCurrentRenderData = ezCreateRenderDataForThisFrame<ezRmlUiRenderData>(nullptr);
      m_pCurrentRenderData->m_GlobalTransform.SetIdentity();
      m_pCurrentRenderData->m_GlobalBounds.SetInvalid();
      m_pCurrentRenderData->m_Batches = batches;
    }
  }

  ezRenderData* Extractor::GetRenderData()
  {
    return m_pCurrentRenderData;
  }

} // namespace ezRmlUiInternal
