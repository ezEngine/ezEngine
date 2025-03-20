#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/Utils.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  template <typename T>
  EZ_ALWAYS_INLINE ezProcessingStream MakeStream(ezArrayPtr<T> data, ezUInt32 uiOffset, const ezHashedString& sName, ezProcessingStream::DataType dataType = ezProcessingStream::DataType::Float)
  {
    return ezProcessingStream(sName, data.ToByteArray().GetSubArray(uiOffset), dataType, sizeof(T));
  }

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
  m_VM.RegisterFunction(ezProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(ezProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

VertexColorTask::~VertexColorTask() = default;

void VertexColorTask::Prepare(const ezWorld& world, const ezMeshBufferResourceDescriptor& desc, const ezTransform& transform, const ezBoundingBox& bbox, ezArrayPtr<ezSharedPtr<const VertexColorOutput>> outputs, ezArrayPtr<ezProcVertexColorMapping> outputMappings, ezArrayPtr<ezColorLinearUB> outputVertexColors)
{
  EZ_PROFILE_SCOPE("VertexColorPrepare");

  m_InputVertices.Clear();
  m_InputVertices.Reserve(desc.GetVertexCount());

  const ezVec3* pPositions = desc.GetPositionData().GetPtr();

  ezUInt32 uiNormalDataStride = 0;
  const ezUInt8* pNormals = desc.GetNormalData(&uiNormalDataStride).GetPtr();
  const ezGALResourceFormat::Enum normalFormat = desc.GetVertexStreamConfig().GetNormalFormat();

  ezUInt32 uiColorDataStride = 0;
  const ezUInt8* pColors = nullptr;
  const ezGALResourceFormat::Enum colorFormat = desc.GetVertexStreamConfig().GetColorFormat();
  if (desc.GetVertexStreamConfig().HasColor0())
  {
    pColors = desc.GetColor0Data(&uiColorDataStride).GetPtr();
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
  normalTransform.Invert(0.0f).IgnoreResult();
  normalTransform.Transpose();

  // write out all vertices
  for (ezUInt32 i = 0; i < desc.GetVertexCount(); ++i)
  {
    ezMeshBufferUtils::DecodeNormal(ezMakeArrayPtr(pNormals, sizeof(ezVec3)), normalFormat, vNormal).IgnoreResult();

    auto& vert = m_InputVertices.ExpandAndGetRef();
    vert.m_vPosition = transform.TransformPosition(*pPositions);
    vert.m_vNormal = normalTransform.TransformDirection(vNormal).GetNormalized();
    vert.m_uiIndex = i;

    ++pPositions;
    pNormals = ezMemoryUtils::AddByteOffset(pNormals, uiNormalDataStride);

    if (pColors != nullptr)
    {
      ezVec4 c;
      ezMeshBufferUtils::DecodeToVec4(ezMakeArrayPtr(pColors, sizeof(ezColor)), colorFormat, c).IgnoreResult();

      vert.m_Color = ezColor(c.x, c.y, c.z, c.w);

      pColors = ezMemoryUtils::AddByteOffset(pColors, uiColorDataStride);
    }
    else
    {
      vert.m_Color = ezColor::MakeZero();
    }
  }

  m_Outputs = outputs;
  m_OutputMappings = outputMappings;
  m_OutputVertexColors = outputVertexColors;

  //////////////////////////////////////////////////////////////////////////

  ezBoundingBox box = bbox;
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

  const ezUInt32 uiTransformHash = ezHashingUtils::xxHash32(&transform, sizeof(ezTransform));
  ezProcGenInternal::SetInstanceSeed(uiTransformHash, m_GlobalData);
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

    ezHybridArray<ezProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.r), ExpressionInputs::s_sColorR));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.g), ExpressionInputs::s_sColorG));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.b), ExpressionInputs::s_sColorB));
      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_Color.a), ExpressionInputs::s_sColorA));

      inputs.PushBack(MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_uiIndex), ExpressionInputs::s_sPointIndex, ezProcessingStream::DataType::Int));
    }

    ezHybridArray<ezProcessingStream, 8> outputs;
    {
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, r), ExpressionOutputs::s_sOutColorR));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, g), ExpressionOutputs::s_sOutColorG));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, b), ExpressionOutputs::s_sOutColorB));
      outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, a), ExpressionOutputs::s_sOutColorA));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumVertices, m_GlobalData, ezExpressionVM::Flags::BestPerformance).Failed())
    {
      continue;
    }

    auto& outputMapping = m_OutputMappings[uiOutputIndex];
    for (ezUInt32 i = 0; i < uiNumVertices; ++i)
    {
      ezColor srcColor = m_TempData[i];
      ezColor remappedColor;
      remappedColor.r = Remap(outputMapping.m_R, srcColor);
      remappedColor.g = Remap(outputMapping.m_G, srcColor);
      remappedColor.b = Remap(outputMapping.m_B, srcColor);
      remappedColor.a = Remap(outputMapping.m_A, srcColor);

      // Store output vertex colors interleaved
      m_OutputVertexColors[i * uiNumOutputs + uiOutputIndex] = remappedColor;
    }
  }
}
