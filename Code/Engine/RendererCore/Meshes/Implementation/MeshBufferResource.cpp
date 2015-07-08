#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshBufferResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezMeshBufferResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshBufferResourceDescriptor::ezMeshBufferResourceDescriptor()
{
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
}

const ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetVertexBufferData() const
{
  return m_VertexStreamData;
}

const ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData;
}

ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetVertexBufferData()
{
  EZ_ASSERT_DEV(!m_VertexStreamData.IsEmpty(), "The vertex data must be allocated first");
  return m_VertexStreamData;
}

ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetIndexBufferData()
{
  EZ_ASSERT_DEV(!m_IndexBufferData.IsEmpty(), "The index data must be allocated first");
  return m_IndexBufferData;
}

ezUInt32 ezMeshBufferResourceDescriptor::AddStream(ezGALVertexAttributeSemantic::Enum Semantic, ezGALResourceFormat::Enum Format)
{
  EZ_ASSERT_DEV(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (ezUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    EZ_ASSERT_DEV(m_VertexDeclaration.m_VertexStreams[i].m_Semantic != Semantic, "The given semantic %u is already used by a previous stream", Semantic);
  }

  ezVertexStreamInfo si;

  si.m_Semantic = Semantic;
  si.m_Format = Format;
  si.m_uiOffset = 0;
  si.m_uiElementSize = ezGALResourceFormat::GetBitsPerElement(Format) / 8;
  m_uiVertexSize += si.m_uiElementSize;

  EZ_ASSERT_DEV(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_VertexDeclaration.m_VertexStreams.IsEmpty())
    si.m_uiOffset = m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiOffset + m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiElementSize;

  m_VertexDeclaration.m_VertexStreams.PushBack(si);

  return m_VertexDeclaration.m_VertexStreams.GetCount() - 1;
}

void ezMeshBufferResourceDescriptor::AllocateStreams(ezUInt32 uiNumVertices, ezUInt32 uiNumTriangles)
{
  EZ_ASSERT_DEV(!m_VertexDeclaration.m_VertexStreams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

  m_uiVertexCount = uiNumVertices;
  const ezUInt32 uiVertexStreamSize = m_uiVertexSize * uiNumVertices;

  m_VertexStreamData.SetCount(uiVertexStreamSize);

  if (uiNumTriangles > 0)
  {
    // use an index buffer at all
    ezUInt32 uiIndexBufferSize = uiNumTriangles * 3;

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(ezUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(ezUInt16);
    }

    m_IndexBufferData.SetCount(uiIndexBufferSize);
  }
}

void ezMeshBufferResourceDescriptor::SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2)
{
  if (Uses32BitIndices()) // 32 Bit indices
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(ezUInt32) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(ezUInt16) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
}

ezUInt32 ezMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(ezUInt32)) / 3;
    else
      return (m_IndexBufferData.GetCount() / sizeof(ezUInt16)) / 3;
  }
  else
  {
    return m_uiVertexCount / 3;
  }
}


ezMeshBufferResource::ezMeshBufferResource() : ezResource<ezMeshBufferResource, ezMeshBufferResourceDescriptor>(DoUpdate::OnMainThread, 1)
{
}

ezMeshBufferResource::~ezMeshBufferResource()
{
  EZ_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

ezResourceLoadDesc ezMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (!m_hVertexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  m_uiPrimitiveCount = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezMeshBufferResource::UpdateContent(ezStreamReaderBase* Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");

  return ezResourceLoadDesc();
}

void ezMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezMeshBufferResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

ezResourceLoadDesc ezMeshBufferResource::CreateResource(const ezMeshBufferResourceDescriptor& descriptor)
{
  EZ_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_VertexDeclaration = descriptor.GetVertexDeclaration();
  m_VertexDeclaration.ComputeHash();

  m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateVertexBuffer(descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), descriptor.GetVertexBufferData().GetData());

  if (descriptor.HasIndexBuffer())
    m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateIndexBuffer(descriptor.Uses32BitIndices() ? ezGALIndexType::UInt : ezGALIndexType::UShort, descriptor.GetPrimitiveCount() * 3, &(descriptor.GetIndexBufferData()[0]));

  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezVertexDeclarationInfo::ComputeHash()
{
  m_uiHash = 0;

  for (const auto& vs : m_VertexStreams)
  {
    m_uiHash += vs.CalculateHash();

    EZ_ASSERT_DEBUG(m_uiHash != 0, "Invalid Hash Value");
  }
}

ezResult ezRenderContext::BuildVertexDeclaration(ezGALShaderHandle hShader, const ezVertexDeclarationInfo& decl, ezGALVertexDeclarationHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader = hShader;
  svd.m_uiVertexDeclarationHash = decl.m_uiHash;

  bool bExisted = false;
  auto it = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    const ezGALShader* pShader = ezGALDevice::GetDefaultDevice()->GetShader(hShader);

    auto pBytecode = pShader->GetDescription().m_ByteCodes[ezGALShaderStage::VertexShader];

    ezGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;

    for (ezUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      //stream.m_Format
      ezGALVertexAttribute gal;
      gal.m_bInstanceData = false;
      gal.m_eFormat = stream.m_Format;
      gal.m_eSemantic = stream.m_Semantic;
      gal.m_uiOffset = stream.m_uiOffset;
      gal.m_uiVertexBufferSlot = 0;
      vd.m_VertexAttributes.PushBack(gal);
    }

    out_Declaration = ezGALDevice::GetDefaultDevice()->CreateVertexDeclaration(vd);

    if (out_Declaration.IsInvalidated())
    {
      /* This can happen when the resource system gives you a fallback resource, which then selects a shader that
         does not fit the mesh layout.
         E.g. when a material is not yet loaded and the fallback material is used, that fallback material may
         use another shader, that requires more data streams, than what the mesh provides.
         This problem will go away, once the proper material is loaded.

         This can be fixed by ensuring that the fallback material uses a shader that only requires data that is
         always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those
         data streams.

         Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is
         available, it will work.
      */

      ezLog::Error("Failed to create vertex declaration");
      return EZ_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferResource);

