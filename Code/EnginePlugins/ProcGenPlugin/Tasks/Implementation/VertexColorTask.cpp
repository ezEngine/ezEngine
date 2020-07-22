#include <ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  EZ_ALWAYS_INLINE float Remap(ezEnum<ezProcVertexColorChannelMapping> channelMapping, const ezColor& srcColor)
  {
    if (channelMapping >= ezProcVertexColorChannelMapping::R && channelMapping <= ezProcVertexColorChannelMapping::A)
    {
      return (&srcColor.r)[channelMapping];
    }
    else
    {
      return channelMapping == ezProcVertexColorChannelMapping::White ? 1.0f : 0.0f;
    }
  }
} // namespace

using namespace ezProcGenInternal;

VertexColorTask::VertexColorTask()
{
  m_VM.RegisterDefaultFunctions();
  m_VM.RegisterFunction("ApplyVolumes", &ezProcGenExpressionFunctions::ApplyVolumes, &ezProcGenExpressionFunctions::ApplyVolumesValidate);
}

VertexColorTask::~VertexColorTask() = default;

void VertexColorTask::Prepare(const ezWorld& world, const ezMeshBufferResourceDescriptor& mbDesc, const ezTransform& transform,
  ezArrayPtr<ezSharedPtr<const VertexColorOutput>> outputs, ezArrayPtr<ezProcVertexColorMapping> outputMappings,
  ezArrayPtr<ezUInt32> outputVertexColors)
{
  EZ_PROFILE_SCOPE("VertexColorPrepare");

  m_InputVertices.Clear();
  m_InputVertices.Reserve(mbDesc.GetVertexCount());

  const ezVertexDeclarationInfo& vdi = mbDesc.GetVertexDeclaration();
  const ezUInt8* pRawVertexData = mbDesc.GetVertexBufferData().GetData();

  const float* pPositions = nullptr;
  const ezUInt8* pNormals = nullptr;
  ezGALResourceFormat::Enum normalFormat = ezGALResourceFormat::Invalid;
  const ezColorLinearUB* pColors = nullptr;

  for (ezUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
      {
        ezLog::Error("Unsupported CPU mesh vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const float*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Normal)
    {
      pNormals = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Color0)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBAUByteNormalized)
      {
        ezLog::Error("Unsupported CPU mesh vertex color format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other color formats are not supported
      }

      pColors = reinterpret_cast<const ezColorLinearUB*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    ezLog::Error("No position and normal stream found in CPU mesh");
    return;
  }

  ezUInt8 dummySource[16] = {};
  ezVec3 vNormal;
  if (ezMeshBufferUtils::DecodeNormal(ezMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    ezLog::Error("Unsupported CPU mesh vertex normal format {0}", normalFormat);
    return;
  }

  ezMat3 normalTransform = transform.GetAsMat4().GetRotationalPart();
  normalTransform.Invert(0.0f);
  normalTransform.Transpose();

  const ezUInt32 uiElementStride = mbDesc.GetVertexDataSize();

  // write out all vertices
  for (ezUInt32 i = 0; i < mbDesc.GetVertexCount(); ++i)
  {
    ezMeshBufferUtils::DecodeNormal(ezMakeArrayPtr(pNormals, sizeof(ezVec3)), normalFormat, vNormal);

    auto& vert = m_InputVertices.ExpandAndGetRef();
    vert.m_vPosition = transform.TransformPosition(ezVec3(pPositions[0], pPositions[1], pPositions[2]));
    vert.m_vNormal = normalTransform.TransformDirection(vNormal).GetNormalized();
    vert.m_Color = pColors != nullptr ? ezColor(*pColors) : ezColor::ZeroColor();
    vert.m_fIndex = i;

    pPositions = ezMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    pNormals = ezMemoryUtils::AddByteOffset(pNormals, uiElementStride);
    pColors = pColors != nullptr ? ezMemoryUtils::AddByteOffset(pColors, uiElementStride) : nullptr;
  }

  m_Outputs = outputs;
  m_OutputMappings = outputMappings;
  m_OutputVertexColors = outputVertexColors;

  //////////////////////////////////////////////////////////////////////////

  // TODO:
  // ezBoundingBox box = mbDesc.GetBounds();
  ezBoundingBox box = ezBoundingBox(ezVec3(-1000), ezVec3(1000));
  box.TransformFromOrigin(transform.GetAsMat4());

  m_VolumeCollections.Clear();
  m_GlobalData.Clear();

  for (auto& pOutput : outputs)
  {
    if (pOutput != nullptr)
    {
      ezProcGenInternal::ExtractVolumeCollections(world, box, *pOutput, m_VolumeCollections, m_GlobalData);
    }
  }
}

void VertexColorTask::Execute()
{
  if (m_InputVertices.IsEmpty())
    return;

  const ezUInt32 uiNumOutputs = m_Outputs.GetCount();
  for (ezUInt32 uiOutputIndex = 0; uiOutputIndex < uiNumOutputs; ++uiOutputIndex)
  {
    auto& pOutput = m_Outputs[uiOutputIndex];
    if (pOutput == nullptr || pOutput->m_pByteCode == nullptr)
      continue;

    EZ_PROFILE_SCOPE("ExecuteVM");

    ezUInt32 uiNumVertices = m_InputVertices.GetCount();
    m_TempData.SetCountUninitialized(uiNumVertices);

    ezHybridArray<ezExpression::Stream, 8> inputs;
    {
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.r), ExpressionInputs::s_sColorR));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.g), ExpressionInputs::s_sColorG));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.b), ExpressionInputs::s_sColorB));
      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.a), ExpressionInputs::s_sColorA));

      inputs.PushBack(ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_fIndex), ExpressionInputs::s_sPointIndex));
    }

    ezHybridArray<ezExpression::Stream, 8> outputs;
    {
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, r), ExpressionOutputs::s_sR));
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, g), ExpressionOutputs::s_sG));
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, b), ExpressionOutputs::s_sB));
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, a), ExpressionOutputs::s_sA));
    }

    // Execute expression bytecode
    m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumVertices, m_GlobalData);

    auto& outputMapping = m_OutputMappings[uiOutputIndex];
    for (ezUInt32 i = 0; i < uiNumVertices; ++i)
    {
      ezColor srcColor = m_TempData[i];
      ezColor remappedColor;
      remappedColor.r = Remap(outputMapping.m_R, srcColor);
      remappedColor.g = Remap(outputMapping.m_G, srcColor);
      remappedColor.b = Remap(outputMapping.m_B, srcColor);
      remappedColor.a = Remap(outputMapping.m_A, srcColor);

      ezColorLinearUB vertexColor = remappedColor;

      // Store output vertex colors interleaved
      m_OutputVertexColors[i * uiNumOutputs + uiOutputIndex] = *reinterpret_cast<ezUInt32*>(&vertexColor.r);
    }
  }
}
