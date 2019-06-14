#include <ProcGenPluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

using namespace ezProcGenInternal;

VertexColorTask::VertexColorTask() = default;
VertexColorTask::~VertexColorTask() = default;

void VertexColorTask::Prepare(const ezMeshBufferResourceDescriptor& mbDesc, const ezTransform& transform,
  const ezSharedPtr<const VertexColorOutput>& pOutput, ezArrayPtr<ezUInt32> outputVertexColors)
{
  m_InputVertices.Clear();
  m_InputVertices.Reserve(mbDesc.GetVertexCount());

  const ezVertexDeclarationInfo& vdi = mbDesc.GetVertexDeclaration();
  const ezUInt8* pRawVertexData = mbDesc.GetVertexBufferData().GetData();

  const float* pPositions = nullptr;
  const float* pNormals = nullptr;
  ezUInt32 uiElementStride = 0;

  for (ezUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    uiElementStride += vdi.m_VertexStreams[vs].m_uiElementSize;

    if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
      {
        ezLog::Warning("Unsupported CPU mesh vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other position formats are not supported
      }

      pPositions = (const float*)(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == ezGALVertexAttributeSemantic::Normal)
    {
      if (vdi.m_VertexStreams[vs].m_Format != ezGALResourceFormat::RGBFloat)
      {
        ezLog::Warning("Unsupported CPU mesh vertex normal format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return; // other position formats are not supported
      }

      pNormals = (const float*)(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  EZ_ASSERT_DEV(uiElementStride == mbDesc.GetVertexDataSize(), "");

  if (pPositions == nullptr || pNormals == nullptr)
  {
    ezLog::Warning("No position and normal stream found in CPU mesh");
    return;
  }

  ezMat3 normalTransform = transform.GetAsMat4().GetRotationalPart();
  normalTransform.Invert(0.0f);
  normalTransform.Transpose();

  // write out all vertices
  for (ezUInt32 i = 0; i < mbDesc.GetVertexCount(); ++i)
  {
    auto& vert = m_InputVertices.ExpandAndGetRef();
    vert.m_vPosition = transform.TransformPosition(ezVec3(pPositions[0], pPositions[1], pPositions[2]));
    vert.m_vNormal = normalTransform.TransformDirection(ezVec3(pNormals[0], pNormals[1], pNormals[2]));
    vert.m_fIndex = i;

    pPositions = ezMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    pNormals = ezMemoryUtils::AddByteOffset(pNormals, uiElementStride);
  }

  m_pOutput = pOutput;
  m_OutputVertexColors = outputVertexColors;
}

void VertexColorTask::Execute()
{
  if (m_InputVertices.IsEmpty())
    return;

  if (m_pOutput->m_pByteCode != nullptr)
  {
    EZ_PROFILE_SCOPE("ExecuteVM");

    ezUInt32 uiNumVertices = m_InputVertices.GetCount();
    m_TempData.SetCountUninitialized(uiNumVertices);

    ezHybridArray<ezExpression::Stream, 8> inputs;
    {
      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.x), ExpressionInputs::s_sPositionX));
      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.y), ExpressionInputs::s_sPositionY));
      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vPosition.z), ExpressionInputs::s_sPositionZ));

      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.x), ExpressionInputs::s_sNormalX));
      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.y), ExpressionInputs::s_sNormalY));
      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_vNormal.z), ExpressionInputs::s_sNormalZ));

      inputs.PushBack(
        ezExpression::MakeStream(m_InputVertices.GetArrayPtr(), offsetof(InputVertex, m_fIndex), ExpressionInputs::s_sPointIndex));
    }

    ezHybridArray<ezExpression::Stream, 8> outputs;
    {
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, r), ExpressionOutputs::s_sR));
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, g), ExpressionOutputs::s_sG));
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, b), ExpressionOutputs::s_sB));
      outputs.PushBack(ezExpression::MakeStream(m_TempData.GetArrayPtr(), offsetof(ezColor, a), ExpressionOutputs::s_sA));
    }

    // Execute expression bytecode
    m_VM.Execute(*(m_pOutput->m_pByteCode), inputs, outputs, uiNumVertices);

    for (ezUInt32 i = 0; i < uiNumVertices; ++i)
    {
      ezColorLinearUB vertexColor = m_TempData[i];
      m_OutputVertexColors[i] = *reinterpret_cast<ezUInt32*>(&vertexColor.r);
    }
  }
}
