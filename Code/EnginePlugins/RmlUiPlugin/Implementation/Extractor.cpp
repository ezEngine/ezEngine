#include <RmlUiPlugin/RmlUiPluginPCH.h>

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

    ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&Extractor::EndFrame, this));
  }

  Extractor::~Extractor()
  {
    ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&Extractor::EndFrame, this));

    for (auto it = m_CompiledGeometry.GetIterator(); it.IsValid(); ++it)
    {
      FreeReleasedGeometry(it.Id());
    }
  }

  void Extractor::RenderGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle hTexture, const Rml::Vector2f& translation)
  {
    // Should never be called since we are using compiled geometry
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  Rml::CompiledGeometryHandle Extractor::CompileGeometry(Rml::Vertex* pVertices, int iNum_vertices, int* pIndices, int iNum_indices, Rml::TextureHandle hTexture)
  {
    CompiledGeometry geometry;
    geometry.m_uiTriangleCount = iNum_indices / 3;

    // vertices
    {
      ezDynamicArray<Vertex> vertexStorage(ezFrameAllocator::GetCurrentAllocator());
      vertexStorage.SetCountUninitialized(iNum_vertices);

      for (ezUInt32 i = 0; i < vertexStorage.GetCount(); ++i)
      {
        auto& srcVertex = pVertices[i];
        auto& destVertex = vertexStorage[i];
        destVertex.m_Position = ezVec3(srcVertex.position.x, srcVertex.position.y, 0);
        destVertex.m_TexCoord = ezVec2(srcVertex.tex_coord.x, srcVertex.tex_coord.y);
        destVertex.m_Color = reinterpret_cast<const ezColorGammaUB&>(srcVertex.colour);
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
      desc.m_uiTotalSize = iNum_indices * desc.m_uiStructSize;
      desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;

      geometry.m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc, ezMakeArrayPtr(pIndices, iNum_indices).ToByteArray());
    }

    // texture
    {
      ezTexture2DResourceHandle* phTexture = nullptr;
      if (m_Textures.TryGetValue(TextureId::FromRml(hTexture), phTexture))
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

  void Extractor::RenderCompiledGeometry(Rml::CompiledGeometryHandle hGeometry_handle, const Rml::Vector2f& translation)
  {
    auto& batch = m_Batches.ExpandAndGetRef();

    EZ_VERIFY(m_CompiledGeometry.TryGetValue(GeometryId::FromRml(hGeometry_handle), batch.m_CompiledGeometry), "Invalid compiled geometry");

    ezMat4 offsetMat = ezMat4::MakeTranslation(m_vOffset.GetAsVec3(0));

    batch.m_Transform = offsetMat * m_mTransform;
    batch.m_Translation = ezVec2(translation.x, translation.y);

    batch.m_ScissorRect = m_ScissorRect;
    batch.m_bEnableScissorRect = m_bEnableScissorRect;
    batch.m_bTransformScissorRect = (m_bEnableScissorRect && m_mTransform.IsIdentity() == false);

    if (!batch.m_bTransformScissorRect)
    {
      batch.m_ScissorRect.x += m_vOffset.x;
      batch.m_ScissorRect.y += m_vOffset.y;
    }
  }

  void Extractor::ReleaseCompiledGeometry(Rml::CompiledGeometryHandle hGeometry_handle)
  {
    m_ReleasedCompiledGeometry.PushBack({ezRenderWorld::GetFrameCounter(), GeometryId::FromRml(hGeometry_handle)});
  }

  void Extractor::EnableScissorRegion(bool bEnable)
  {
    m_bEnableScissorRect = bEnable;
  }

  void Extractor::SetScissorRegion(int x, int y, int iWidth, int iHeight)
  {
    m_ScissorRect = ezRectFloat(static_cast<float>(x), static_cast<float>(y), static_cast<float>(iWidth), static_cast<float>(iHeight));
  }

  bool Extractor::LoadTexture(Rml::TextureHandle& ref_hTexture_handle, Rml::Vector2i& ref_texture_dimensions, const Rml::String& sSource)
  {
    ezTexture2DResourceHandle hTexture = ezResourceManager::LoadResource<ezTexture2DResource>(sSource.c_str());

    ezResourceLock<ezTexture2DResource> pTexture(hTexture, ezResourceAcquireMode::BlockTillLoaded);
    if (pTexture.GetAcquireResult() == ezResourceAcquireResult::Final)
    {
      ref_hTexture_handle = m_Textures.Insert(hTexture).ToRml();
      ref_texture_dimensions = Rml::Vector2i(pTexture->GetWidth(), pTexture->GetHeight());

      return true;
    }

    return false;
  }

  bool Extractor::GenerateTexture(Rml::TextureHandle& ref_hTexture_handle, const Rml::byte* pSource, const Rml::Vector2i& source_dimensions)
  {
    ezUInt32 uiWidth = source_dimensions.x;
    ezUInt32 uiHeight = source_dimensions.y;
    ezUInt32 uiSizeInBytes = uiWidth * uiHeight * 4;

    ezUInt64 uiHash = ezHashingUtils::xxHash64(pSource, uiSizeInBytes);

    ezStringBuilder sTextureName;
    sTextureName.SetFormat("RmlUiGeneratedTexture_{}x{}_{}", uiWidth, uiHeight, uiHash);

    ezTexture2DResourceHandle hTexture = ezResourceManager::GetExistingResource<ezTexture2DResource>(sTextureName);

    if (!hTexture.IsValid())
    {
      ezGALSystemMemoryDescription memoryDesc;
      memoryDesc.m_pData = ezMakeByteBlobPtr(pSource, uiSizeInBytes);
      memoryDesc.m_uiRowPitch = uiWidth * 4;
      memoryDesc.m_uiSlicePitch = uiSizeInBytes;

      ezTexture2DResourceDescriptor desc;
      desc.m_DescGAL.m_uiWidth = uiWidth;
      desc.m_DescGAL.m_uiHeight = uiHeight;
      desc.m_DescGAL.m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
      desc.m_InitialContent = ezMakeArrayPtr(&memoryDesc, 1);

      hTexture = ezResourceManager::GetOrCreateResource<ezTexture2DResource>(sTextureName, std::move(desc));
    }

    ref_hTexture_handle = m_Textures.Insert(hTexture).ToRml();
    return true;
  }

  void Extractor::ReleaseTexture(Rml::TextureHandle hTexture_handle)
  {
    EZ_VERIFY(m_Textures.Remove(TextureId::FromRml(hTexture_handle)), "Invalid texture handle");
  }

  void Extractor::SetTransform(const Rml::Matrix4f* pTransform)
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

  void Extractor::BeginExtraction(const ezVec2I32& vOffset)
  {
    m_vOffset = ezVec2(static_cast<float>(vOffset.x), static_cast<float>(vOffset.y));
    m_mTransform = ezMat4::MakeIdentity();

    m_Batches.Clear();
  }

  void Extractor::EndExtraction() {}

  ezRenderData* Extractor::GetRenderData()
  {
    if (m_Batches.IsEmpty() == false)
    {
      ezRmlUiRenderData* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezRmlUiRenderData, ezFrameAllocator::GetCurrentAllocator());
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds = ezBoundingBoxSphere::MakeInvalid();
      pRenderData->m_Batches = m_Batches;

      return pRenderData;
    }

    return nullptr;
  }

  void Extractor::EndFrame(const ezGALDeviceEvent& e)
  {
    if (e.m_Type != ezGALDeviceEvent::BeforeEndFrame)
      return;

    ezUInt64 uiFrameCounter = ezRenderWorld::GetFrameCounter();

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

  void Extractor::FreeReleasedGeometry(GeometryId id)
  {
    CompiledGeometry* pGeometry = nullptr;
    if (!m_CompiledGeometry.TryGetValue(id, pGeometry))
      return;

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hVertexBuffer);
    pGeometry->m_hVertexBuffer.Invalidate();

    ezGALDevice::GetDefaultDevice()->DestroyBuffer(pGeometry->m_hIndexBuffer);
    pGeometry->m_hIndexBuffer.Invalidate();

    pGeometry->m_hTexture.Invalidate();
  }

} // namespace ezRmlUiInternal
