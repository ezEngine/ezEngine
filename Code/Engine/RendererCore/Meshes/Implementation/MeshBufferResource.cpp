#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/IterateBits.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

namespace
{
  const char* s_szMeshVertexStreamTypeNames[] = {
    "Position",
    "NormalTangentAndTexCoord0",
    "TexCoord1",
    "Color0",
    "Color1",
    "SkinningData",
  };

  static_assert(EZ_ARRAY_SIZE(s_szMeshVertexStreamTypeNames) == ezMeshVertexStreamType::Count);
} // namespace

// static
const char* ezMeshVertexStreamType::GetName(Enum type)
{
  return s_szMeshVertexStreamTypeNames[type];
}

////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE static ezUInt32 GetElementSize(ezGALResourceFormat::Enum format)
{
  return ezGALResourceFormat::GetBitsPerElement(format) / 8;
}

constexpr ezGALResourceFormat::Enum s_PositionFormat = ezGALResourceFormat::XYZFloat;

constexpr ezGALResourceFormat::Enum s_NormalFormat_lp = ezGALResourceFormat::RGB10A2UIntNormalized;
constexpr ezGALResourceFormat::Enum s_NormalFormat_hp = ezGALResourceFormat::XYZFloat;

constexpr ezGALResourceFormat::Enum s_TangentFormat_lp = ezGALResourceFormat::RGB10A2UIntNormalized;
constexpr ezGALResourceFormat::Enum s_TangentFormat_hp = ezGALResourceFormat::XYZWFloat;

constexpr ezGALResourceFormat::Enum s_TexCoordFormat_lp = ezGALResourceFormat::UVHalf;
constexpr ezGALResourceFormat::Enum s_TexCoordFormat_hp = ezGALResourceFormat::UVFloat;

constexpr ezGALResourceFormat::Enum s_ColorFormat_lp = ezGALResourceFormat::RGBAUByteNormalized;
constexpr ezGALResourceFormat::Enum s_ColorFormat_hp = ezGALResourceFormat::RGBAHalf;

constexpr ezGALResourceFormat::Enum s_BoneIndicesFormat = ezGALResourceFormat::RGBAUShort;
constexpr ezGALResourceFormat::Enum s_BoneWeightsFormat_lp = ezGALResourceFormat::RGBAUByteNormalized;
constexpr ezGALResourceFormat::Enum s_BoneWeightsFormat_hp = ezGALResourceFormat::RGBAUShortNormalized;

static ezGALVertexAttribute s_VertexAttributes_lp[] = {
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, s_PositionFormat, 0, 0),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Normal, s_NormalFormat_lp, 0, 1),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Tangent, s_TangentFormat_lp, GetElementSize(s_NormalFormat_lp), 1),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, s_TexCoordFormat_lp, GetElementSize(s_NormalFormat_lp) + GetElementSize(s_TangentFormat_lp), 1),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord1, s_TexCoordFormat_lp, 0, 2),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color0, s_ColorFormat_lp, 0, 3),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color1, s_ColorFormat_lp, 0, 4),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::BoneIndices0, s_BoneIndicesFormat, 0, 5),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::BoneWeights0, s_BoneWeightsFormat_lp, GetElementSize(s_BoneIndicesFormat), 5),
};

static ezGALVertexAttribute s_VertexAttributes_hp[] = {
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Position, s_PositionFormat, 0, 0),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Normal, s_NormalFormat_hp, 0, 1),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Tangent, s_TangentFormat_hp, GetElementSize(s_NormalFormat_hp), 1),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord0, s_TexCoordFormat_hp, GetElementSize(s_NormalFormat_hp) + GetElementSize(s_TangentFormat_hp), 1),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::TexCoord1, s_TexCoordFormat_hp, 0, 2),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color0, s_ColorFormat_hp, 0, 3),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::Color1, s_ColorFormat_hp, 0, 4),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::BoneIndices0, s_BoneIndicesFormat, 0, 5),
  ezGALVertexAttribute(ezGALVertexAttributeSemantic::BoneWeights0, s_BoneWeightsFormat_hp, GetElementSize(s_BoneIndicesFormat), 5),
};

static ezUInt32 s_StreamSizes_lp[] = {
  GetElementSize(s_PositionFormat),
  GetElementSize(s_NormalFormat_lp) + GetElementSize(s_TangentFormat_lp) + GetElementSize(s_TexCoordFormat_lp),
  GetElementSize(s_TexCoordFormat_lp),
  GetElementSize(s_ColorFormat_lp),
  GetElementSize(s_ColorFormat_lp),
  GetElementSize(s_BoneIndicesFormat) + GetElementSize(s_BoneWeightsFormat_lp),
};

static_assert(EZ_ARRAY_SIZE(s_StreamSizes_lp) == ezMeshVertexStreamType::Count);

static ezUInt32 s_StreamSizes_hp[] = {
  GetElementSize(s_PositionFormat),
  GetElementSize(s_NormalFormat_hp) + GetElementSize(s_TangentFormat_hp) + GetElementSize(s_TexCoordFormat_hp),
  GetElementSize(s_TexCoordFormat_hp),
  GetElementSize(s_ColorFormat_hp),
  GetElementSize(s_ColorFormat_hp),
  GetElementSize(s_BoneIndicesFormat) + GetElementSize(s_BoneWeightsFormat_hp),
};

static_assert(EZ_ARRAY_SIZE(s_StreamSizes_hp) == ezMeshVertexStreamType::Count);

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetPositionFormat() const
{
  return s_PositionFormat;
}

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetNormalFormat() const
{
  return m_bUseHighPrecision ? s_NormalFormat_hp : s_NormalFormat_lp;
}

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetTangentFormat() const
{
  return m_bUseHighPrecision ? s_TangentFormat_hp : s_TangentFormat_lp;
}

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetTexCoordFormat() const
{
  return m_bUseHighPrecision ? s_TexCoordFormat_hp : s_TexCoordFormat_lp;
}

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetColorFormat() const
{
  return m_bUseHighPrecision ? s_ColorFormat_hp : s_ColorFormat_lp;
}

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetBoneIndicesFormat() const
{
  return s_BoneIndicesFormat;
}

ezGALResourceFormat::Enum ezMeshVertexStreamConfig::GetBoneWeightsFormat() const
{
  return m_bUseHighPrecision ? s_BoneWeightsFormat_hp : s_BoneWeightsFormat_lp;
}

ezUInt32 ezMeshVertexStreamConfig::GetNormalDataOffset() const
{
  EZ_ASSERT_DEBUG(s_VertexAttributes_lp[1].m_eSemantic == ezGALVertexAttributeSemantic::Normal && s_VertexAttributes_hp[1].m_eSemantic == ezGALVertexAttributeSemantic::Normal, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[1].m_uiOffset : s_VertexAttributes_lp[1].m_uiOffset;
}

ezUInt32 ezMeshVertexStreamConfig::GetTangentDataOffset() const
{
  EZ_ASSERT_DEBUG(s_VertexAttributes_lp[2].m_eSemantic == ezGALVertexAttributeSemantic::Tangent && s_VertexAttributes_hp[2].m_eSemantic == ezGALVertexAttributeSemantic::Tangent, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[2].m_uiOffset : s_VertexAttributes_lp[2].m_uiOffset;
}

ezUInt32 ezMeshVertexStreamConfig::GetTexCoord0DataOffset() const
{
  EZ_ASSERT_DEBUG(s_VertexAttributes_lp[3].m_eSemantic == ezGALVertexAttributeSemantic::TexCoord0 && s_VertexAttributes_hp[3].m_eSemantic == ezGALVertexAttributeSemantic::TexCoord0, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[3].m_uiOffset : s_VertexAttributes_lp[3].m_uiOffset;
}

ezUInt32 ezMeshVertexStreamConfig::GetBoneIndicesDataOffset() const
{
  EZ_ASSERT_DEBUG(s_VertexAttributes_lp[7].m_eSemantic == ezGALVertexAttributeSemantic::BoneIndices0 && s_VertexAttributes_hp[7].m_eSemantic == ezGALVertexAttributeSemantic::BoneIndices0, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[7].m_uiOffset : s_VertexAttributes_lp[7].m_uiOffset;
}

ezUInt32 ezMeshVertexStreamConfig::GetBoneWeightsDataOffset() const
{
  EZ_ASSERT_DEBUG(s_VertexAttributes_lp[8].m_eSemantic == ezGALVertexAttributeSemantic::BoneWeights0 && s_VertexAttributes_hp[8].m_eSemantic == ezGALVertexAttributeSemantic::BoneWeights0, "");
  return m_bUseHighPrecision ? s_VertexAttributes_hp[8].m_uiOffset : s_VertexAttributes_lp[8].m_uiOffset;
}

ezUInt32 ezMeshVertexStreamConfig::GetStreamElementSize(ezMeshVertexStreamType::Enum type) const
{
  return m_bUseHighPrecision ? s_StreamSizes_hp[type] : s_StreamSizes_lp[type];
}

ezArrayPtr<ezGALVertexAttribute> ezMeshVertexStreamConfig::GetAllVertexAttributes()
{
  return ezMakeArrayPtr(m_bUseHighPrecision ? s_VertexAttributes_hp : s_VertexAttributes_lp);
}

////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshBufferResource, 1, ezRTTIDefaultAllocator<ezMeshBufferResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezMeshBufferResource);
// clang-format on

ezMeshBufferResourceDescriptor::ezMeshBufferResourceDescriptor() = default;
ezMeshBufferResourceDescriptor::~ezMeshBufferResourceDescriptor() = default;

void ezMeshBufferResourceDescriptor::Clear()
{
  m_Topology = ezGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
  m_VertexStreamConfig = ezMeshVertexStreamConfig();
  m_VertexStreamsData.Clear();
  m_IndexBufferData.Clear();
}

void ezMeshBufferResourceDescriptor::AddStream(ezMeshVertexStreamType::Enum type, bool bUseHighPrecision /*= false*/)
{
  EZ_ASSERT_DEV(m_VertexStreamsData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  m_VertexStreamConfig.AddStream(type);
  m_VertexStreamConfig.m_bUseHighPrecision |= bUseHighPrecision;
}

void ezMeshBufferResourceDescriptor::AddCommonStreams(bool bUseHighPrecision /*= false*/)
{
  AddStream(ezMeshVertexStreamType::Position, bUseHighPrecision);
  AddStream(ezMeshVertexStreamType::NormalTangentAndTexCoord0, bUseHighPrecision);
}

void ezMeshBufferResourceDescriptor::AllocateStreams(ezUInt32 uiNumVertices, ezGALPrimitiveTopology::Enum topology, ezUInt32 uiNumPrimitives, bool bZeroFill /*= false*/)
{
  EZ_ASSERT_DEV(m_VertexStreamConfig.m_uiTypesMask != 0, "You have to add streams via 'AddStream' before calling this function");

  m_Topology = topology;
  m_uiVertexCount = uiNumVertices;
  m_uiVertexSize = 0;

  const ezUInt32 uiHighestStreamIndex = m_VertexStreamConfig.GetHighestStreamIndex();
  m_VertexStreamsData.SetCount(uiHighestStreamIndex + 1);

  for (ezUInt32 uiIndex = ezMeshVertexStreamType::Position; uiIndex < ezMeshVertexStreamType::Count; ++uiIndex)
  {
    auto type = static_cast<ezMeshVertexStreamType::Enum>(uiIndex);
    if (!m_VertexStreamConfig.HasStream(type))
      continue;

    const ezUInt32 uiStreamElementSize = m_VertexStreamConfig.GetStreamElementSize(type);
    if (bZeroFill)
    {
      m_VertexStreamsData[uiIndex].SetCount(m_uiVertexCount * uiStreamElementSize);
    }
    else
    {
      m_VertexStreamsData[uiIndex].SetCountUninitialized(m_uiVertexCount * uiStreamElementSize);
    }

    m_uiVertexSize += uiStreamElementSize;
  }

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    ezUInt32 uiIndexBufferSize = ezGALPrimitiveTopology::GetIndexCount(topology, uiNumPrimitives);

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(ezUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(ezUInt16);
    }

    m_IndexBufferData.SetCountUninitialized(uiIndexBufferSize);
  }
}

void ezMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const ezGeometry& geom, ezGALPrimitiveTopology::Enum topology)
{
  ezLogBlock _("Allocate Streams From Geometry");

  // Index Buffer Generation
  ezDynamicArray<ezUInt32> Indices;

  if (topology == ezGALPrimitiveTopology::Points)
  {
    // Leaving indices empty disables indexed rendering.
  }
  else if (topology == ezGALPrimitiveTopology::Lines)
  {
    Indices.Reserve(geom.GetLines().GetCount() * 2);

    for (ezUInt32 p = 0; p < geom.GetLines().GetCount(); ++p)
    {
      Indices.PushBack(geom.GetLines()[p].m_uiStartVertex);
      Indices.PushBack(geom.GetLines()[p].m_uiEndVertex);
    }
  }
  else if (topology == ezGALPrimitiveTopology::Triangles)
  {
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    for (ezUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (ezUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
      }
    }
  }
  AllocateStreams(geom.GetVertices().GetCount(), topology, Indices.GetCount() / (topology + 1), true);

  // Fill vertex buffer.
  {
    if (m_VertexStreamConfig.HasPosition())
    {
      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetPosition(v, geom.GetVertices()[v].m_vPosition);
      }
    }

    if (m_VertexStreamConfig.HasNormalTangentAndTexCoord0())
    {
      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        auto& vert = geom.GetVertices()[v];

        SetNormal(v, vert.m_vNormal);
        SetTangent(v, vert.m_vTangent.GetAsVec4(vert.m_fBiTangentSign));
        SetTexCoord0(v, vert.m_vTexCoord);
      }
    }

    if (m_VertexStreamConfig.HasTexCoord1())
    {
      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetTexCoord1(v, geom.GetVertices()[v].m_vTexCoord);
      }
    }

    if (m_VertexStreamConfig.HasColor0())
    {
      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetColor0(v, geom.GetVertices()[v].m_Color);
      }
    }

    if (m_VertexStreamConfig.HasColor1())
    {
      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        SetColor0(v, geom.GetVertices()[v].m_Color);
      }
    }

    if (m_VertexStreamConfig.HasSkinningData())
    {
      for (ezUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
      {
        auto& vert = geom.GetVertices()[v];

        SetBoneIndices(v, vert.m_BoneIndices);
        SetBoneWeights(v, ezColor(vert.m_BoneWeights).GetAsVec4());
      }
    }
  }

  // Fill index buffer.
  {
    if (topology == ezGALPrimitiveTopology::Points)
    {
      for (ezUInt32 t = 0; t < Indices.GetCount(); t += 1)
      {
        SetPointIndices(t, Indices[t]);
      }
    }
    else if (topology == ezGALPrimitiveTopology::Triangles)
    {
      for (ezUInt32 t = 0; t < Indices.GetCount(); t += 3)
      {
        SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
      }
    }
    else if (topology == ezGALPrimitiveTopology::Lines)
    {
      for (ezUInt32 t = 0; t < Indices.GetCount(); t += 2)
      {
        SetLineIndices(t / 2, Indices[t], Indices[t + 1]);
      }
    }
  }
}

ezUInt32 ezMeshBufferResourceDescriptor::GetNumVertexBuffers() const
{
  return m_VertexStreamsData.GetCount();
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetVertexBufferData(ezMeshVertexStreamType::Enum type) const
{
  return m_VertexStreamsData[type].GetArrayPtr();
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData.GetArrayPtr();
}

ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>& ezMeshBufferResourceDescriptor::GetVertexBufferData(ezMeshVertexStreamType::Enum type)
{
  return m_VertexStreamsData[type];
}

ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper>& ezMeshBufferResourceDescriptor::GetIndexBufferData()
{
  return m_IndexBufferData;
}

ezArrayPtr<const ezVec3> ezMeshBufferResourceDescriptor::GetPositionData() const
{
  auto data = m_VertexStreamsData[ezMeshVertexStreamType::Position].GetArrayPtr();
  return ezMakeArrayPtr(reinterpret_cast<const ezVec3*>(data.GetPtr()), data.GetCount() / sizeof(ezVec3));
}

ezArrayPtr<ezVec3> ezMeshBufferResourceDescriptor::GetPositionData()
{
  auto data = m_VertexStreamsData[ezMeshVertexStreamType::Position].GetArrayPtr();
  return ezMakeArrayPtr(reinterpret_cast<ezVec3*>(data.GetPtr()), data.GetCount() / sizeof(ezVec3));
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetNormalData(ezUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetNormalDataOffset());
}

ezArrayPtr<ezUInt8> ezMeshBufferResourceDescriptor::GetNormalData(ezUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetNormalDataOffset());
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetTangentData(ezUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTangentDataOffset());
}

ezArrayPtr<ezUInt8> ezMeshBufferResourceDescriptor::GetTangentData(ezUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTangentDataOffset());
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetTexCoord0Data(ezUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTexCoord0DataOffset());
}

ezArrayPtr<ezUInt8> ezMeshBufferResourceDescriptor::GetTexCoord0Data(ezUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::NormalTangentAndTexCoord0].GetArrayPtr();
  return data.GetSubArray(m_VertexStreamConfig.GetTexCoord0DataOffset());
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetTexCoord1Data(ezUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetTexCoord1ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::TexCoord1].GetArrayPtr();
  return data;
}

ezArrayPtr<ezUInt8> ezMeshBufferResourceDescriptor::GetTexCoord1Data(ezUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetTexCoord1ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::TexCoord1].GetArrayPtr();
  return data;
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetColor0Data(ezUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::Color0].GetArrayPtr();
  return data;
}

ezArrayPtr<ezUInt8> ezMeshBufferResourceDescriptor::GetColor0Data(ezUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor0ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::Color0].GetArrayPtr();
  return data;
}

ezArrayPtr<const ezUInt8> ezMeshBufferResourceDescriptor::GetColor1Data(ezUInt32* out_pStride /*= nullptr*/) const
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor1ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::Color1].GetArrayPtr();
  return data;
}

ezArrayPtr<ezUInt8> ezMeshBufferResourceDescriptor::GetColor1Data(ezUInt32* out_pStride /*= nullptr*/)
{
  if (out_pStride != nullptr)
    *out_pStride = m_VertexStreamConfig.GetColor1ElementSize();

  auto data = m_VertexStreamsData[ezMeshVertexStreamType::Color1].GetArrayPtr();
  return data;
}

const ezVec3& ezMeshBufferResourceDescriptor::GetPosition(ezUInt32 uiVertexIndex) const
{
  return reinterpret_cast<const ezVec3&>(m_VertexStreamsData[ezMeshVertexStreamType::Position][uiVertexIndex * sizeof(ezVec3)]);
}

void ezMeshBufferResourceDescriptor::SetPosition(ezUInt32 uiVertexIndex, const ezVec3& vPos)
{
  *reinterpret_cast<ezVec3*>(&m_VertexStreamsData[ezMeshVertexStreamType::Position][uiVertexIndex * sizeof(ezVec3)]) = vPos;
}

ezVec3 ezMeshBufferResourceDescriptor::GetNormal(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetNormalDataOffset());

  ezVec3 res;
  ezMeshBufferUtils::DecodeNormal(data, m_VertexStreamConfig.GetNormalFormat(), res).AssertSuccess();

  return res;
}

void ezMeshBufferResourceDescriptor::SetNormal(ezUInt32 uiVertexIndex, const ezVec3& vNormal)
{
  auto data = GetVertexData(ezMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetNormalDataOffset());

  ezMeshBufferUtils::EncodeNormal(vNormal, data, m_VertexStreamConfig.GetNormalFormat()).AssertSuccess();
}

ezVec4 ezMeshBufferResourceDescriptor::GetTangent(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTangentDataOffset());

  ezVec3 vTangent = ezVec3::MakeZero();
  float fBiTangentSign = 0.0f;
  ezMeshBufferUtils::DecodeTangent(data, m_VertexStreamConfig.GetTangentFormat(), vTangent, fBiTangentSign).AssertSuccess();

  return vTangent.GetAsVec4(fBiTangentSign);
}

void ezMeshBufferResourceDescriptor::SetTangent(ezUInt32 uiVertexIndex, const ezVec4& vTangent)
{
  auto data = GetVertexData(ezMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTangentDataOffset());

  ezMeshBufferUtils::EncodeTangent(vTangent.GetAsVec3(), vTangent.w, data, m_VertexStreamConfig.GetTangentFormat()).AssertSuccess();
}

ezVec2 ezMeshBufferResourceDescriptor::GetTexCoord0(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTexCoord0DataOffset());

  ezVec2 res;
  ezMeshBufferUtils::DecodeTexCoord(data, m_VertexStreamConfig.GetTexCoordFormat(), res).AssertSuccess();

  return res;
}

void ezMeshBufferResourceDescriptor::SetTexCoord0(ezUInt32 uiVertexIndex, const ezVec2& vTexCoord)
{
  auto data = GetVertexData(ezMeshVertexStreamType::NormalTangentAndTexCoord0, uiVertexIndex, m_VertexStreamConfig.GetNormalTangentAndTexCoord0ElementSize(), m_VertexStreamConfig.GetTexCoord0DataOffset());

  ezMeshBufferUtils::EncodeTexCoord(vTexCoord, data, m_VertexStreamConfig.GetTexCoordFormat()).AssertSuccess();
}

ezVec2 ezMeshBufferResourceDescriptor::GetTexCoord1(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::TexCoord1, uiVertexIndex, m_VertexStreamConfig.GetTexCoord1ElementSize());

  ezVec2 res;
  ezMeshBufferUtils::DecodeTexCoord(data, m_VertexStreamConfig.GetTexCoordFormat(), res).AssertSuccess();

  return res;
}

void ezMeshBufferResourceDescriptor::SetTexCoord1(ezUInt32 uiVertexIndex, const ezVec2& vTexCoord)
{
  auto data = GetVertexData(ezMeshVertexStreamType::TexCoord1, uiVertexIndex, m_VertexStreamConfig.GetTexCoord1ElementSize());

  ezMeshBufferUtils::EncodeTexCoord(vTexCoord, data, m_VertexStreamConfig.GetTexCoordFormat()).AssertSuccess();
}

ezColor ezMeshBufferResourceDescriptor::GetColor0(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::Color0, uiVertexIndex, m_VertexStreamConfig.GetColor0ElementSize());

  ezColor res;
  ezMeshBufferUtils::DecodeColor(data, m_VertexStreamConfig.GetColorFormat(), res).AssertSuccess();

  return res;
}

void ezMeshBufferResourceDescriptor::SetColor0(ezUInt32 uiVertexIndex, const ezColorLinearUB& color)
{
  auto data = GetVertexData(ezMeshVertexStreamType::Color0, uiVertexIndex, m_VertexStreamConfig.GetColor0ElementSize());

  if (m_VertexStreamConfig.m_bUseHighPrecision)
  {
    ezMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), ezMeshVertexColorConversion::None).AssertSuccess();
  }
  else
  {
    *reinterpret_cast<ezColorLinearUB*>(data.GetPtr()) = color;
  }
}

void ezMeshBufferResourceDescriptor::SetColor0(ezUInt32 uiVertexIndex, const ezColor& color, ezMeshVertexColorConversion::Enum conversion /*= ezMeshVertexColorConversion::Default*/)
{
  auto data = GetVertexData(ezMeshVertexStreamType::Color0, uiVertexIndex, m_VertexStreamConfig.GetColor0ElementSize());

  ezMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), conversion).AssertSuccess();
}

ezColor ezMeshBufferResourceDescriptor::GetColor1(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::Color1, uiVertexIndex, m_VertexStreamConfig.GetColor1ElementSize());

  ezColor res;
  ezMeshBufferUtils::DecodeColor(data, m_VertexStreamConfig.GetColorFormat(), res).AssertSuccess();

  return res;
}

void ezMeshBufferResourceDescriptor::SetColor1(ezUInt32 uiVertexIndex, const ezColorLinearUB& color)
{
  auto data = GetVertexData(ezMeshVertexStreamType::Color1, uiVertexIndex, m_VertexStreamConfig.GetColor1ElementSize());

  if (m_VertexStreamConfig.m_bUseHighPrecision)
  {
    ezMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), ezMeshVertexColorConversion::None).AssertSuccess();
  }
  else
  {
    *reinterpret_cast<ezColorLinearUB*>(data.GetPtr()) = color;
  }
}

void ezMeshBufferResourceDescriptor::SetColor1(ezUInt32 uiVertexIndex, const ezColor& color, ezMeshVertexColorConversion::Enum conversion /*= ezMeshVertexColorConversion::Default*/)
{
  auto data = GetVertexData(ezMeshVertexStreamType::Color1, uiVertexIndex, m_VertexStreamConfig.GetColor1ElementSize());

  ezMeshBufferUtils::EncodeColor(color, data, m_VertexStreamConfig.GetColorFormat(), conversion).AssertSuccess();
}

const ezVec4U16& ezMeshBufferResourceDescriptor::GetBoneIndices(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneIndicesDataOffset());

  return *reinterpret_cast<const ezVec4U16*>(data.GetPtr());
}

void ezMeshBufferResourceDescriptor::SetBoneIndices(ezUInt32 uiVertexIndex, const ezVec4U16& vIndices)
{
  auto data = GetVertexData(ezMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneIndicesDataOffset());

  *reinterpret_cast<ezVec4U16*>(data.GetPtr()) = vIndices;
}

ezVec4 ezMeshBufferResourceDescriptor::GetBoneWeights(ezUInt32 uiVertexIndex) const
{
  auto data = GetVertexData(ezMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneWeightsDataOffset());

  ezVec4 res;
  ezMeshBufferUtils::DecodeBoneWeights(data, m_VertexStreamConfig.GetBoneWeightsFormat(), res).AssertSuccess();

  return res;
}

void ezMeshBufferResourceDescriptor::SetBoneWeights(ezUInt32 uiVertexIndex, const ezVec4& vWeights)
{
  auto data = GetVertexData(ezMeshVertexStreamType::SkinningData, uiVertexIndex, m_VertexStreamConfig.GetSkinningDataElementSize(), m_VertexStreamConfig.GetBoneWeightsDataOffset());

  ezMeshBufferUtils::EncodeBoneWeights(vWeights, data, m_VertexStreamConfig.GetBoneWeightsFormat()).AssertSuccess();
}



void ezMeshBufferResourceDescriptor::SetPointIndices(ezUInt32 uiPoint, ezUInt32 uiVertex0)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Points, "Wrong topology");

  if (Uses32BitIndices())
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiPoint * sizeof(ezUInt32) * 1]);
    pIndices[0] = uiVertex0;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiPoint * sizeof(ezUInt16) * 1]);
    pIndices[0] = static_cast<ezUInt16>(uiVertex0);
  }
}

void ezMeshBufferResourceDescriptor::SetLineIndices(ezUInt32 uiLine, ezUInt32 uiVertex0, ezUInt32 uiVertex1)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Lines, "Wrong topology");

  if (Uses32BitIndices())
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiLine * sizeof(ezUInt32) * 2]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiLine * sizeof(ezUInt16) * 2]);
    pIndices[0] = static_cast<ezUInt16>(uiVertex0);
    pIndices[1] = static_cast<ezUInt16>(uiVertex1);
  }
}

void ezMeshBufferResourceDescriptor::SetTriangleIndices(ezUInt32 uiTriangle, ezUInt32 uiVertex0, ezUInt32 uiVertex1, ezUInt32 uiVertex2)
{
  EZ_ASSERT_DEBUG(m_Topology == ezGALPrimitiveTopology::Triangles, "Wrong topology");
  EZ_ASSERT_DEBUG(uiVertex0 < m_uiVertexCount && uiVertex1 < m_uiVertexCount && uiVertex2 < m_uiVertexCount, "Vertex indices out of range.");

  if (Uses32BitIndices())
  {
    ezUInt32* pIndices = reinterpret_cast<ezUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(ezUInt32) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
  else
  {
    ezUInt16* pIndices = reinterpret_cast<ezUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(ezUInt16) * 3]);
    pIndices[0] = static_cast<ezUInt16>(uiVertex0);
    pIndices[1] = static_cast<ezUInt16>(uiVertex1);
    pIndices[2] = static_cast<ezUInt16>(uiVertex2);
  }
}

ezUInt32 ezMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const ezUInt32 divider = m_Topology + 1;

  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(ezUInt32)) / divider;
    else
      return (m_IndexBufferData.GetCount() / sizeof(ezUInt16)) / divider;
  }
  else
  {
    return m_uiVertexCount / divider;
  }
}

ezBoundingBoxSphere ezMeshBufferResourceDescriptor::ComputeBounds() const
{
  ezBoundingBoxSphere bounds = ezBoundingBoxSphere::MakeInvalid();

  if (m_VertexStreamConfig.HasPosition() && !m_VertexStreamsData.IsEmpty() && m_uiVertexCount > 0)
  {
    const ezVec3* pPositions = GetPositionData().GetPtr();
    bounds = ezBoundingBoxSphere::MakeFromPoints(pPositions, m_uiVertexCount);
  }

  if (!bounds.IsValid())
  {
    bounds = ezBoundingBoxSphere::MakeFromCenterExtents(ezVec3::MakeZero(), ezVec3(0.1f), 0.1f);
  }

  return bounds;
}

ezResult ezMeshBufferResourceDescriptor::RecomputeNormals()
{
  if (m_Topology != ezGALPrimitiveTopology::Triangles)
    return EZ_FAILURE; // normals not needed

  if (!m_VertexStreamConfig.HasPosition() || !m_VertexStreamConfig.HasNormal())
    return EZ_FAILURE; // there are no normals that could be recomputed

  const ezUInt32 uiVertexSize = m_uiVertexSize;
  const ezVec3* pPositions = GetPositionData().GetPtr();

  ezDynamicArray<ezVec3> newNormals;
  newNormals.SetCountUninitialized(m_uiVertexCount);

  for (auto& n : newNormals)
  {
    n.SetZero();
  }

  ezResult res = EZ_SUCCESS;

  const ezUInt16* pIndices16 = reinterpret_cast<const ezUInt16*>(m_IndexBufferData.GetData());
  const ezUInt32* pIndices32 = reinterpret_cast<const ezUInt32*>(m_IndexBufferData.GetData());
  const bool bUseIndices32 = Uses32BitIndices();

  // Compute unnormalized triangle normals and add them to all vertices.
  // This way large triangles have an higher influence on the vertex normal.
  for (ezUInt32 triIdx = 0; triIdx < GetPrimitiveCount(); ++triIdx)
  {
    const ezUInt32 v0 = bUseIndices32 ? pIndices32[triIdx * 3 + 0] : pIndices16[triIdx * 3 + 0];
    const ezUInt32 v1 = bUseIndices32 ? pIndices32[triIdx * 3 + 1] : pIndices16[triIdx * 3 + 1];
    const ezUInt32 v2 = bUseIndices32 ? pIndices32[triIdx * 3 + 2] : pIndices16[triIdx * 3 + 2];

    const ezVec3 p0 = pPositions[v0];
    const ezVec3 p1 = pPositions[v1];
    const ezVec3 p2 = pPositions[v2];

    const ezVec3 d01 = p1 - p0;
    const ezVec3 d02 = p2 - p0;

    const ezVec3 triNormal = d01.CrossRH(d02);

    if (triNormal.IsValid())
    {
      newNormals[v0] += triNormal;
      newNormals[v1] += triNormal;
      newNormals[v2] += triNormal;
    }
  }

  for (ezUInt32 i = 0; i < newNormals.GetCount(); ++i)
  {
    // normalize the new normal
    if (newNormals[i].NormalizeIfNotZero(ezVec3::MakeAxisX()).Failed())
      res = EZ_FAILURE;

    SetNormal(i, newNormals[i]);
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezMeshBufferResource::ezMeshBufferResource()
  : ezResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

ezMeshBufferResource::~ezMeshBufferResource()
{
  for (auto hVertexBuffer : m_hVertexBuffers)
  {
    EZ_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

ezResourceLoadDesc ezMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    if (!hVertexBuffer.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyBuffer(hVertexBuffer);
      hVertexBuffer.Invalidate();
    }
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

ezResourceLoadDesc ezMeshBufferResource::UpdateContent(ezStreamReader* Stream)
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

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezMeshBufferResource, ezMeshBufferResourceDescriptor)
{
  for (auto hVertexBuffer : m_hVertexBuffers)
  {
    EZ_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_VertexStreamConfig = descriptor.GetVertexStreamConfig();
  m_VertexStreamConfig.FillVertexAttributes(m_VertexAttributes);
  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();
  m_Topology = descriptor.GetTopology();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezStringBuilder sName;

  for (ezUInt32 uiIndex : ezIterateBitIndices(m_VertexStreamConfig.m_uiTypesMask))
  {
    auto type = static_cast<ezMeshVertexStreamType::Enum>(uiIndex);
    const ezUInt32 uiElementSize = m_VertexStreamConfig.GetStreamElementSize(type);

    m_hVertexBuffers[uiIndex] = pDevice->CreateVertexBuffer(uiElementSize, descriptor.GetVertexCount(), descriptor.GetVertexBufferData(type));

    sName.SetFormat("{0} Vertex Buffer {1}", GetResourceIdOrDescription(), ezMeshVertexStreamType::GetName(type));
    pDevice->GetBuffer(m_hVertexBuffers[uiIndex])->SetDebugName(sName);
  }

  ezUInt32 uiIndexBufferSize = 0;
  if (descriptor.HasIndexBuffer())
  {
    const ezUInt32 uiIndexCount = ezGALPrimitiveTopology::GetIndexCount(m_Topology, m_uiPrimitiveCount);
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.Uses32BitIndices() ? ezGALIndexType::UInt : ezGALIndexType::UShort, uiIndexCount, descriptor.GetIndexBufferData());

    sName.SetFormat("{0} Index Buffer", GetResourceIdOrDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);

    uiIndexBufferSize = descriptor.GetIndexBufferData().GetCount();
  }

  {
    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = (descriptor.GetVertexDataSize() * descriptor.GetVertexCount()) + uiIndexBufferSize;
  }

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  m_Bounds = descriptor.ComputeBounds();

  return res;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferResource);
