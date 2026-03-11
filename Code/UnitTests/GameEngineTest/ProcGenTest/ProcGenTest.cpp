#include <GameEngineTest/GameEngineTestPCH.h>

#include "ProcGenTest.h"

#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/Tasks/Utils.h>

static ezGameEngineTestProcGen s_GameEngineTestProcGen;

const char* ezGameEngineTestProcGen::GetTestName() const
{
  return "ProcGen Tests";
}

ezGameEngineTestApplication* ezGameEngineTestProcGen::CreateApplication()
{
  m_pOwnApplication = EZ_DEFAULT_NEW(ezGameEngineTestApplication, "ProcGen");
  return m_pOwnApplication;
}

void ezGameEngineTestProcGen::SetupSubTests()
{
  AddSubTest("VertexColors", SubTests::VertexColors);
  AddSubTest("CurveNode", SubTests::CurveNode);
}

ezResult ezGameEngineTestProcGen::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(SUPER::InitializeSubTest(iIdentifier));

  m_iFrame = -1;
  m_uiImgCompIdx = 0;
  m_ImgCompFrames.Clear();

  if (iIdentifier == SubTests::VertexColors)
  {
    m_ImgCompFrames.PushBack(1);

    EZ_SUCCEED_OR_RETURN(m_pOwnApplication->LoadScene("ProcGen/AssetCache/Common/Scenes/VertexColors.ezBinScene"));
    return EZ_SUCCESS;
  }
  else if (iIdentifier == SubTests::CurveNode)
  {
    InputVertex inputVertices[] = {
      {ezVec3(0.0f), ezVec3(0, 0, 1), ezColor::White, 0},
      {ezVec3(0.25f), ezVec3(0, 0, 1), ezColor::White, 1},
      {ezVec3(0.5f), ezVec3(0, 0, 1), ezColor::White, 2},
      {ezVec3(1.0f), ezVec3(0, 0, 1), ezColor::White, 3},
      {ezVec3(2.0f), ezVec3(0, 0, 1), ezColor::White, 4},
    };

    ezVec4 expectedOutputs[] = {
      ezVec4(1.0f, 0, 0, 1),
      ezVec4(0.375f, 1, 0.02f, 1),
      ezVec4(0.0f, 0, 0.274f, 1),
      ezVec4(1.0f, 0, 2, 1),
      ezVec4(1.0f, 0, 2, 0),
    };

    EZ_SUCCEED_OR_RETURN(TestOutput(ezMakeHashedString("CurveNodeTest"), ezMakeArrayPtr(inputVertices), ezMakeArrayPtr(expectedOutputs)));
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezTestAppRun ezGameEngineTestProcGen::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  const bool bVulkan = ezGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    return ezTestAppRun::Quit;
  }

  if (m_ImgCompFrames.IsEmpty())
  {
    return ezTestAppRun::Quit;
  }

  if (m_ImgCompFrames[m_uiImgCompIdx] == m_iFrame)
  {
    EZ_TEST_IMAGE(m_uiImgCompIdx, bVulkan ? 300 : 250);
    ++m_uiImgCompIdx;

    if (m_uiImgCompIdx >= m_ImgCompFrames.GetCount())
    {
      return ezTestAppRun::Quit;
    }
  }

  return ezTestAppRun::Continue;
}

ezResult ezGameEngineTestProcGen::DeInitializeTest()
{
  m_pVM.Clear();

  return SUPER::DeInitializeTest();
}

//////////////////////////////////////////////////////////////////////////////

template <typename T>
EZ_ALWAYS_INLINE ezProcessingStream MakeStream(ezArrayPtr<T> data, ezUInt32 uiOffset, const ezHashedString& sName, ezProcessingStream::DataType dataType = ezProcessingStream::DataType::Float)
{
  return ezProcessingStream(sName, data.ToByteArray().GetSubArray(uiOffset), dataType, sizeof(T));
}

ezResult ezGameEngineTestProcGen::TestOutput(const ezHashedString& sOutputName, ezArrayPtr<InputVertex> inputVertices, ezArrayPtr<const ezVec4> expectedOutputs)
{
  EZ_ASSERT_DEV(inputVertices.GetCount() == expectedOutputs.GetCount(), "Input and expected output count must match");

  // Data/ProcGenGraph.ezProcGenGraphAsset
  ezProcGenGraphResourceHandle hResource = ezResourceManager::LoadResource<ezProcGenGraphResource>("{ 11fe0278-f21e-4e05-9262-c836adeeef10 }");

  ezResourceLock<ezProcGenGraphResource> pResource(hResource, ezResourceAcquireMode::BlockTillLoaded);
  if (pResource.GetAcquireResult() != ezResourceAcquireResult::Final)
  {
    ezLog::Error("Failed to load ProcGenGraphResource for testing");
    return EZ_FAILURE;
  }

  ezSharedPtr<const ezProcGenInternal::VertexColorOutput> pOutput;
  {
    auto& vcOutputs = pResource->GetVertexColorOutputs();
    for (auto& pVcOutput : vcOutputs)
    {
      if (pVcOutput->m_sName == sOutputName)
      {
        pOutput = pVcOutput;
        break;
      }
    }
    if (!pOutput)
    {
      ezLog::Error("Failed to find VertexColorOutput '{0}' in ProcGenGraphResource", sOutputName);
      return EZ_FAILURE;
    }
  }

  if (m_pVM == nullptr)
  {
    m_pVM = EZ_DEFAULT_NEW(ezExpressionVM);
    m_pVM->RegisterFunction(ezProcGenExpressionFunctions::s_SampleCurveFunc);
  }

  ezTempHybridArray<ezProcessingStream, 8> inputs;
  {
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vPosition.x), ezProcGenInternal::ExpressionInputs::s_sPositionX));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vPosition.y), ezProcGenInternal::ExpressionInputs::s_sPositionY));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vPosition.z), ezProcGenInternal::ExpressionInputs::s_sPositionZ));

    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vNormal.x), ezProcGenInternal::ExpressionInputs::s_sNormalX));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vNormal.y), ezProcGenInternal::ExpressionInputs::s_sNormalY));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_vNormal.z), ezProcGenInternal::ExpressionInputs::s_sNormalZ));

    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.r), ezProcGenInternal::ExpressionInputs::s_sColorR));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.g), ezProcGenInternal::ExpressionInputs::s_sColorG));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.b), ezProcGenInternal::ExpressionInputs::s_sColorB));
    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_Color.a), ezProcGenInternal::ExpressionInputs::s_sColorA));

    inputs.PushBack(MakeStream(inputVertices, offsetof(InputVertex, m_uiIndex), ezProcGenInternal::ExpressionInputs::s_sPointIndex, ezProcessingStream::DataType::Int));
  }

  ezTempHybridArray<ezVec4, 16> m_TempData;
  m_TempData.SetCountUninitialized(inputVertices.GetCount());

  ezTempHybridArray<ezProcessingStream, 8> outputs;
  {
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezVec4, x), ezProcGenInternal::ExpressionOutputs::s_sOutColorR));
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezVec4, y), ezProcGenInternal::ExpressionOutputs::s_sOutColorG));
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezVec4, z), ezProcGenInternal::ExpressionOutputs::s_sOutColorB));
    outputs.PushBack(MakeStream(m_TempData.GetArrayPtr(), offsetof(ezVec4, w), ezProcGenInternal::ExpressionOutputs::s_sOutColorA));
  }

  m_GlobalData.Clear();
  ezProcGenGlobalData::SetCurves(*pOutput, m_GlobalData);

  EZ_SUCCEED_OR_RETURN(m_pVM->Execute(*(pOutput->m_pByteCode), inputs, outputs, inputVertices.GetCount(), m_GlobalData, ezExpressionVM::Flags::BestPerformance));

  for (ezUInt32 i = 0; i < m_TempData.GetCount(); ++i)
  {
    const ezVec4& actual = m_TempData[i];
    const ezVec4& expected = expectedOutputs[i];
    if (!actual.IsEqual(expected, 0.001f))
    {
      ezLog::Error("Output value mismatch at index {}: Expected {}, but got {}", i, expected, actual);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}
