#include <RendererCore/PCH.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>

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
  EZ_ASSERT(!m_VertexStreamData.IsEmpty(), "The vertex data must be allocated first");
  return m_VertexStreamData;
}

ezDynamicArray<ezUInt8>& ezMeshBufferResourceDescriptor::GetIndexBufferData()
{
  EZ_ASSERT(!m_IndexBufferData.IsEmpty(), "The index data must be allocated first");
  return m_IndexBufferData;
}

ezUInt32 ezMeshBufferResourceDescriptor::AddStream(ezGALVertexAttributeSemantic::Enum Semantic, ezGALResourceFormat::Enum Format)
{
  EZ_ASSERT(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (ezUInt32 i = 0; i < m_Streams.GetCount(); ++i)
  {
    EZ_ASSERT(m_Streams[i].m_Semantic != Semantic, "The given semantic %u is already used by a previous stream", Semantic);
  }

  ezMeshBufferResourceDescriptor::StreamInfo si;

  si.m_Semantic = Semantic;
  si.m_Format = Format;
  si.m_uiOffset = 0;
  si.m_uiElementSize = ezGALResourceFormat::GetSize(Format);
  m_uiVertexSize += si.m_uiElementSize;

  EZ_ASSERT(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_Streams.IsEmpty())
    si.m_uiOffset = m_Streams.PeekBack().m_uiOffset + m_Streams.PeekBack().m_uiElementSize;

  m_Streams.PushBack(si);

  return m_Streams.GetCount() - 1;
}

void ezMeshBufferResourceDescriptor::AllocateStreams(ezUInt32 uiNumVertices, ezUInt32 uiNumTriangles)
{
  EZ_ASSERT(!m_Streams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

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


ezMeshBufferResource::ezMeshBufferResource()
{
  m_uiLoadedQualityLevel = 0;
  m_uiMaxQualityLevel = 1;
  m_Flags.Add(ezResourceFlags::UpdateOnMainThread);
}

ezMeshBufferResource::~ezMeshBufferResource()
{
  EZ_ASSERT(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

void ezMeshBufferResource::UnloadData(bool bFullUnload)
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

  m_uiMaxQualityLevel = 0; // cannot be reloaded
  m_uiLoadedQualityLevel = 0;
  m_LoadingState = ezResourceLoadState::Uninitialized;

  SetMemoryUsageGPU(0);
}

void ezMeshBufferResource::UpdateContent(ezStreamReaderBase& Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");
}

void ezMeshBufferResource::UpdateMemoryUsage()
{
}

void ezMeshBufferResource::CreateResource(const ezMeshBufferResourceDescriptor& descriptor)
{
  EZ_ASSERT(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateVertexBuffer(descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), &(descriptor.GetVertexBufferData()[0]));

  if (descriptor.HasIndexBuffer())
    m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateIndexBuffer(descriptor.Uses32BitIndices() ? ezGALIndexType::UInt : ezGALIndexType::UShort, descriptor.GetPrimitiveCount() * 3, &(descriptor.GetIndexBufferData()[0]));

  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();

  m_uiMaxQualityLevel = 1;
  m_uiLoadedQualityLevel = 1;

  m_LoadingState = ezResourceLoadState::Loaded;

  SetMemoryUsageCPU(0);
  SetMemoryUsageGPU(descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount());
}
