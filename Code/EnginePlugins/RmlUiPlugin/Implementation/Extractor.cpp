#include <RmlUiPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RmlUiPlugin/Implementation/Extractor.h>

namespace ezRmlUiInternal
{
  Extractor::Extractor()
  {
    m_hFallbackTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
  }

  Extractor::~Extractor()
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      ReleaseCompiledGeometry(it.Id().ToRml());
    }
  }

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

  void Extractor::RenderCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry_handle, const Rml::Core::Vector2f& translation)
  {
    auto& batch = m_Batches.ExpandAndGetRef();

    EZ_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(geometry_handle), batch.m_CompiledGeometry), "Invalid compiled geometry");

    ezMat4 offsetMat;
    offsetMat.SetTranslationMatrix(m_Offset.GetAsVec3(0));

    batch.m_Transform = offsetMat * m_Transform;
    batch.m_Translation = ezVec2(translation.x, translation.y);

    batch.m_ScissorRect = m_ScissorRect;
    batch.m_bEnableScissorRect = m_bEnableScissorRect;
    batch.m_bTransformScissorRect = (m_bEnableScissorRect && m_Transform.IsIdentity() == false);

    if (!batch.m_bTransformScissorRect)
    {
      batch.m_ScissorRect.x += m_Offset.x;
      batch.m_ScissorRect.y += m_Offset.y;
    }
  }

  void Extractor::ReleaseCompiledGeometry(Rml::Core::CompiledGeometryHandle geometry_handle)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(GeometryId::FromRml(geometry_handle), pGeometry))
      return;

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();

    pGeometry->m_hTexture.Invalidate();

    m_CompiledGeometry.Remove(GeometryId::FromRml(geometry_handle));
  }

  void Extractor::EnableScissorRegion(bool enable)
  {
    m_bEnableScissorRect = enable;
  }

  void Extractor::SetScissorRegion(int x, int y, int width, int height)
  {
    m_ScissorRect = ezRectFloat(x, y, width, height);
  }

  bool Extractor::LoadTexture(Rml::Core::TextureHandle& texture_handle, Rml::Core::Vector2i& texture_dimensions, const Rml::Core::String& source)
  {
    ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(source.c_str());

    ezResourceLock<ezTexture2DResource> pTexture(hTexture, ezResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      texture_handle = m_Textures.Insert(hTexture).ToRml();
      texture_dimensions = Rml::Core::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      return true;
    }

    return false;
  }

  bool Extractor::GenerateTexture(Rml::Core::TextureHandle& texture_handle, const Rml::Core::byte* source, const Rml::Core::Vector2i& source_dimensions)
  {
    ezUInt32 uiWidth = source_dimensions.x;
    ezUInt32 uiHeight = source_dimensions.y;
    ezUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;

    ezUInt64 uiHash = ezHashingUtils::xxHash64(source, uiSizeInBytes);

    ezStringBuilder sTextureName;
    sTextureName.Format("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    ezTexture2DResourceHandle hTexture = ezResourceManager::GetExistingResource<ezTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      ezGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData = const_cast<Rml::Core::byte*>(source);
      memoryDesc.m_uiRowPitch = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      ezTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

      hTexture = ezResourceManager::CreateResource<ezTexture2DResource>(sTextureName, std::move(desc));
    }

    texture_handle = m_Textures.Insert(hTexture).ToRml();
    return true;
  }

  void Extractor::ReleaseTexture(Rml::Core::TextureHandle texture_handle)
  {
    EZ_VERIFY(m_Textures.Remove(TextureId::FromRml(texture_handle)), "Invalid texture handle");
  }

  void Extractor::SetTransform(const Rml::Core::Matrix4f* transform)
  {
    if (transform != nullptr)
    {
      constexpr ezMatrixLayout::Enum matrixLayout = std::is_same<Rml::Core::Matrix4f, Rml::Core::ColumnMajorMatrix4f>::value ? ezMatrixLayout::ColumnMajor : ezMatrixLayout::RowMajor;
      m_Transform.SetFromArray(transform->data(), matrixLayout);
    }
    else
    {
      m_Transform.SetIdentity();
    }
  }

  void Extractor::BeginExtraction(const ezVec2I32& offset)
  {
    m_Offset = ezVec2(offset.x, offset.y);
    m_Transform = ezMat4::IdentityMatrix();

    m_Batches.Clear();
  }

  void Extractor::EndExtraction()
  {
  }

  ezRenderData* Extractor::GetRenderData()
  {
    if (m_Batches.IsEmpty() == false)
    {
      ezRmlUiRenderData* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezRmlUiRenderData, ezFrameAllocator::GetCurrentAllocator());
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds.SetInvalid();
      pRenderData->m_Batches = m_Batches;

      return pRenderData;
    }

    return nullptr;
  }

} // namespace ezRmlUiInternal
