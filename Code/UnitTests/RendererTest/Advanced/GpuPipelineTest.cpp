#include <RendererTest/RendererTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Passes/SwitchPass.h>
#include <RendererCore/Pipeline/RenderPipelinePassGraph.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererTest/Advanced/GpuPipelineTest.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  using Connectivity = ezRenderPipelinePinConnection::Connectivity;

  struct RecordedPin
  {
    Connectivity m_Connectivity = Connectivity::None;
    ezUInt32 m_uiHandleId = 0;
  };

  struct RecordedPass
  {
    ezString m_sName;
    ezHybridArray<RecordedPin, 4> m_Inputs;
    ezHybridArray<RecordedPin, 4> m_Outputs;
  };

  ezDynamicArray<RecordedPass>* s_pExecutionOrder = nullptr;

  RecordedPin MakeRecordedPin(const ezRenderPipelinePinConnection& connection)
  {
    RecordedPin pin;
    pin.m_Connectivity = connection.m_Connectivity;
    if (connection.m_Connectivity == Connectivity::Texture)
      pin.m_uiHandleId = connection.m_TextureHandle.GetInternalID().m_Data;
    else if (connection.m_Connectivity == Connectivity::Buffer)
      pin.m_uiHandleId = connection.m_BufferHandle.GetInternalID().m_Data;

    return pin;
  }

  void RecordPass(ezStringView sName, ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<const ezRenderPipelinePinConnection> outputs)
  {
    RecordedPass& recorded = s_pExecutionOrder->ExpandAndGetRef();
    recorded.m_sName = sName;

    for (const ezRenderPipelinePinConnection& connection : inputs)
    {
      recorded.m_Inputs.PushBack(MakeRecordedPin(connection));
    }
    for (const ezRenderPipelinePinConnection& connection : outputs)
    {
      recorded.m_Outputs.PushBack(MakeRecordedPin(connection));
    }
  }

  class ezGpuPipelineTestSourcePass : public ezRenderPipelinePass
  {
    EZ_ADD_DYNAMIC_REFLECTION(ezGpuPipelineTestSourcePass, ezRenderPipelinePass);

  public:
    ezGpuPipelineTestSourcePass()
      : ezRenderPipelinePass("Source", true)
    {
    }

    // Creates a real resource per output so that pass-through forwarding can be verified by handle identity.
    virtual ezStatus AddRenderPasses(const ezViewData&, const ezCamera&, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override
    {
      for (ezRenderPipelinePinConnection& output : outputs)
      {
        if (output.m_Connectivity == Connectivity::Texture)
        {
          ezGALTextureCreationDescription desc;
          desc.SetAsRenderTarget(4, 4, ezGALResourceFormat::RGBAUByteNormalized);
          output = ezRenderPipelinePinConnection(Connectivity::Texture, ref_graph.CreateTexture(desc));
        }
        else if (output.m_Connectivity == Connectivity::Buffer)
        {
          ezGALBufferCreationDescription desc;
          desc.m_uiTotalSize = 256;
          output = ezRenderPipelinePinConnection(Connectivity::Buffer, ref_graph.CreateBuffer(desc));
        }
      }

      RecordPass(GetName(), inputs, outputs);
      return EZ_SUCCESS;
    }

    ezRenderPipelineNodeOutputPin m_Output;
    ezRenderPipelineNodeBufferOutputPin m_BufferOutput;
  };

  class ezGpuPipelineTestPass : public ezRenderPipelinePass
  {
    EZ_ADD_DYNAMIC_REFLECTION(ezGpuPipelineTestPass, ezRenderPipelinePass);

  public:
    ezGpuPipelineTestPass()
      : ezRenderPipelinePass("Pass", true)
    {
    }

    virtual ezStatus AddRenderPasses(const ezViewData&, const ezCamera&, ezRenderGraph&, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override
    {
      RecordPass(GetName(), inputs, outputs);
      return EZ_SUCCESS;
    }

    ezRenderPipelineNodeInputPin m_Input;
    ezRenderPipelineNodeOutputPin m_Output;
    ezRenderPipelineNodeBufferInputPin m_BufferInput;
    ezRenderPipelineNodeBufferOutputPin m_BufferOutput;
  };

  class ezGpuPipelineTestSinkPass : public ezRenderPipelinePass
  {
    EZ_ADD_DYNAMIC_REFLECTION(ezGpuPipelineTestSinkPass, ezRenderPipelinePass);

  public:
    ezGpuPipelineTestSinkPass()
      : ezRenderPipelinePass("Sink", true)
    {
    }

    virtual ezStatus AddRenderPasses(const ezViewData&, const ezCamera&, ezRenderGraph&, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override
    {
      RecordPass(GetName(), inputs, outputs);
      return EZ_SUCCESS;
    }

    ezRenderPipelineNodeInputPin m_InputA;
    ezRenderPipelineNodeInputPin m_InputB;
    ezRenderPipelineNodeBufferInputPin m_BufferInputA;
    ezRenderPipelineNodeBufferInputPin m_BufferInputB;
    ezInt32 m_iValue = 0;
  };

  class ezGpuPipelineTestPassThroughPass : public ezRenderPipelinePass
  {
    EZ_ADD_DYNAMIC_REFLECTION(ezGpuPipelineTestPassThroughPass, ezRenderPipelinePass);

  public:
    ezGpuPipelineTestPassThroughPass()
      : ezRenderPipelinePass("PassThrough", true)
    {
    }

    virtual ezStatus AddRenderPasses(const ezViewData&, const ezCamera&, ezRenderGraph&, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override
    {
      RecordPass(GetName(), inputs, outputs);
      return EZ_SUCCESS;
    }

    ezRenderPipelineNodePassThroughPin m_Pin;
    ezRenderPipelineNodeBufferPassThroughPin m_BufferPin;
  };

  // clang-format off
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGpuPipelineTestSourcePass, 1, ezRTTIDefaultAllocator<ezGpuPipelineTestSourcePass>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Output", m_Output),
      EZ_MEMBER_PROPERTY("BufferOutput", m_BufferOutput),
    }
    EZ_END_PROPERTIES;
  }
  EZ_END_DYNAMIC_REFLECTED_TYPE;

  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGpuPipelineTestPass, 1, ezRTTIDefaultAllocator<ezGpuPipelineTestPass>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Input", m_Input),
      EZ_MEMBER_PROPERTY("Output", m_Output),
      EZ_MEMBER_PROPERTY("BufferInput", m_BufferInput),
      EZ_MEMBER_PROPERTY("BufferOutput", m_BufferOutput),
    }
    EZ_END_PROPERTIES;
  }
  EZ_END_DYNAMIC_REFLECTED_TYPE;

  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGpuPipelineTestSinkPass, 1, ezRTTIDefaultAllocator<ezGpuPipelineTestSinkPass>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("InputA", m_InputA),
      EZ_MEMBER_PROPERTY("InputB", m_InputB),
      EZ_MEMBER_PROPERTY("BufferInputA", m_BufferInputA),
      EZ_MEMBER_PROPERTY("BufferInputB", m_BufferInputB),
      EZ_MEMBER_PROPERTY("Value", m_iValue),
    }
    EZ_END_PROPERTIES;
  }
  EZ_END_DYNAMIC_REFLECTED_TYPE;

  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGpuPipelineTestPassThroughPass, 1, ezRTTIDefaultAllocator<ezGpuPipelineTestPassThroughPass>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Pin", m_Pin),
      EZ_MEMBER_PROPERTY("BufferPin", m_BufferPin),
    }
    EZ_END_PROPERTIES;
  }
  EZ_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  template <typename PassType>
  ezUInt32 AddPass(ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& ref_passes, const char* szName)
  {
    ezUniquePtr<PassType> pPass = EZ_DEFAULT_NEW(PassType);
    pPass->SetName(szName);
    const ezUInt32 uiIndex = ref_passes.GetCount();
    ref_passes.PushBack(std::move(pPass));
    return uiIndex;
  }

  void Connect(ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& ref_connections, ezUInt32 uiSource, const char* szSourcePin, ezUInt32 uiTarget, const char* szTargetPin)
  {
    auto& connection = ref_connections.ExpandAndGetRef();
    connection.m_uiSource = uiSource;
    connection.m_uiTarget = uiTarget;
    connection.m_sSourcePin = szSourcePin;
    connection.m_sTargetPin = szTargetPin;
  }

  ezUniquePtr<ezRenderPipelinePassGraph> CreatePipeline(ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>&& passes, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& ref_connections)
  {
    ezDynamicArray<ezUniquePtr<ezExtractor>> extractors;
    return EZ_DEFAULT_NEW(ezRenderPipelinePassGraph, std::move(passes), std::move(extractors), ref_connections);
  }

  void CompileAndExecute(ezRenderPipelinePassGraph& ref_pipeline, ezRenderGraph& ref_graph, ezDynamicArray<RecordedPass>& ref_executionOrder)
  {
    EZ_TEST_RESULT(ref_pipeline.CullDeadPasses());
    EZ_TEST_RESULT(ref_pipeline.SortPasses());

    ref_executionOrder.Clear();
    s_pExecutionOrder = &ref_executionOrder;
    ezViewData viewData;
    ezCamera camera;
    EZ_TEST_BOOL(ref_pipeline.AddRenderPasses(viewData, camera, ref_graph).Succeeded());
    s_pExecutionOrder = nullptr;
  }

  void TestExecutionOrder(const ezDynamicArray<RecordedPass>& executionOrder, ezArrayPtr<const char* const> expectedOrder)
  {
    EZ_TEST_INT(executionOrder.GetCount(), expectedOrder.GetCount());
    if (executionOrder.GetCount() != expectedOrder.GetCount())
      return;

    for (ezUInt32 i = 0; i < expectedOrder.GetCount(); ++i)
    {
      EZ_TEST_STRING(executionOrder[i].m_sName, expectedOrder[i]);
    }
  }

  ezUInt32 GetPassIndex(const ezDynamicArray<RecordedPass>& executionOrder, const char* szName)
  {
    for (ezUInt32 i = 0; i < executionOrder.GetCount(); ++i)
    {
      if (executionOrder[i].m_sName == szName)
        return i;
    }

    EZ_TEST_FAILURE("Pass not found", "No pass named '{}' was executed.", szName);
    return ezInvalidIndex;
  }

  void TestExecutedBefore(const ezDynamicArray<RecordedPass>& executionOrder, const char* szFirst, const char* szSecond)
  {
    EZ_TEST_BOOL(GetPassIndex(executionOrder, szFirst) < GetPassIndex(executionOrder, szSecond));
  }

  RecordedPin GetInput(const ezDynamicArray<RecordedPass>& executionOrder, const char* szName, ezUInt32 uiPin)
  {
    const ezUInt32 uiPass = GetPassIndex(executionOrder, szName);
    if (uiPass == ezInvalidIndex || !EZ_TEST_BOOL(uiPin < executionOrder[uiPass].m_Inputs.GetCount()))
      return {};

    return executionOrder[uiPass].m_Inputs[uiPin];
  }

  RecordedPin GetOutput(const ezDynamicArray<RecordedPass>& executionOrder, const char* szName, ezUInt32 uiPin)
  {
    const ezUInt32 uiPass = GetPassIndex(executionOrder, szName);
    if (uiPass == ezInvalidIndex || !EZ_TEST_BOOL(uiPin < executionOrder[uiPass].m_Outputs.GetCount()))
      return {};

    return executionOrder[uiPass].m_Outputs[uiPin];
  }

  void TestPinsEqual(const RecordedPin& lhs, const RecordedPin& rhs, Connectivity expectedConnectivity)
  {
    EZ_TEST_BOOL(lhs.m_Connectivity == expectedConnectivity);
    EZ_TEST_BOOL(rhs.m_Connectivity == expectedConnectivity);
    EZ_TEST_INT(lhs.m_uiHandleId, rhs.m_uiHandleId);
  }

  struct InlineTestData
  {
    enum class Mode
    {
      Basic,
      Forward,
      Mixed,
      BufferForward,
      MixedBoundaries,
      Extractors,
      InvalidConnection,
      Failure,
    };

    Mode m_Mode = Mode::Basic;

    ezStatus Import(ezStringView, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& out_passes, ezDynamicArray<ezUniquePtr<ezExtractor>>& out_extractors, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& out_connections)
    {
      EZ_IGNORE_UNUSED(out_extractors);

      if (m_Mode == Mode::Failure)
        return ezStatus("Test failure");

      if (m_Mode == Mode::Extractors)
      {
        out_extractors.PushBack(EZ_DEFAULT_NEW(ezVisibleObjectsExtractor));
        out_extractors.PushBack(EZ_DEFAULT_NEW(ezSelectedObjectsExtractor));
        return EZ_SUCCESS;
      }

      auto AddImportedPass = [&out_passes](ezRenderPipelinePass* pPass, const char* szName)
      {
        pPass->SetName(szName);
        out_passes.PushBack(ezUniquePtr<ezRenderPipelinePass>(pPass, ezFoundation::GetDefaultAllocator()));
      };

      if (m_Mode == Mode::Forward)
      {
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureInputNode), "Input");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureOutputNode), "Output");
        Connect(out_connections, 0, "Value", 1, "Value");
        return EZ_SUCCESS;
      }

      if (m_Mode == Mode::BufferForward)
      {
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphBufferInputNode), "Input");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphBufferOutputNode), "Output");
        Connect(out_connections, 0, "Value", 1, "Value");
        return EZ_SUCCESS;
      }

      if (m_Mode == Mode::MixedBoundaries)
      {
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureInputNode), "TextureInput");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphBufferInputNode), "BufferInput");
        AddImportedPass(EZ_DEFAULT_NEW(ezGpuPipelineTestPass), "Internal");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureOutputNode), "TextureOutput");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphBufferOutputNode), "BufferOutput");
        Connect(out_connections, 0, "Value", 2, "Input");
        Connect(out_connections, 1, "Value", 2, "BufferInput");
        Connect(out_connections, 2, "Output", 3, "Value");
        Connect(out_connections, 2, "BufferOutput", 4, "Value");
        return EZ_SUCCESS;
      }

      if (m_Mode == Mode::Mixed)
      {
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureInputNode), "Input");
        AddImportedPass(EZ_DEFAULT_NEW(ezGpuPipelineTestPass), "Internal");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureOutputNode), "Forwarded");
        AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureOutputNode), "Processed");
        Connect(out_connections, 0, "Value", 1, "Input");
        Connect(out_connections, 0, "Value", 2, "Value");
        Connect(out_connections, 1, "Output", 3, "Value");
        return EZ_SUCCESS;
      }

      AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureInputNode), "Input");
      AddImportedPass(EZ_DEFAULT_NEW(ezGpuPipelineTestPass), "First");
      AddImportedPass(EZ_DEFAULT_NEW(ezGpuPipelineTestPass), "Second");
      AddImportedPass(EZ_DEFAULT_NEW(ezSubGraphTextureOutputNode), "Output");

      if (m_Mode == Mode::InvalidConnection)
      {
        Connect(out_connections, 0, "Value", 10, "Input");
        return EZ_SUCCESS;
      }

      Connect(out_connections, 0, "Value", 1, "Input");
      Connect(out_connections, 0, "Value", 2, "Input");
      Connect(out_connections, 1, "Output", 3, "Value");
      return EZ_SUCCESS;
    }
  };

  ezUniquePtr<ezSubGraphNode> CreateSubGraph(const char* szPipeline)
  {
    ezUniquePtr<ezSubGraphNode> pSubGraph = EZ_DEFAULT_NEW(ezSubGraphNode);
    pSubGraph->m_sPipeline = szPipeline;
    return pSubGraph;
  }
} // namespace

static ezGpuPipelineTest s_GpuPipelineTest;

void ezGpuPipelineTest::SetupSubTests()
{
  AddSubTest("DeadPassCulling", SubTests::ST_DeadPassCulling);
  AddSubTest("DependencySorting", SubTests::ST_DependencySorting);
  AddSubTest("PassThroughSorting", SubTests::ST_PassThroughSorting);
  AddSubTest("CycleDetection", SubTests::ST_CycleDetection);
  AddSubTest("TextureSwitch", SubTests::ST_TextureSwitch);
  AddSubTest("SubGraphInlining", SubTests::ST_SubGraphInlining);
  AddSubTest("SubGraphInliningErrors", SubTests::ST_SubGraphInliningErrors);
  AddSubTest("ViewBlackboard", SubTests::ST_ViewBlackboard);
  AddSubTest("BufferPassThroughSorting", SubTests::ST_BufferPassThroughSorting);
  AddSubTest("MixedPassThrough", SubTests::ST_MixedPassThrough);
  AddSubTest("MixedPinIndexing", SubTests::ST_MixedPinIndexing);
  AddSubTest("BufferSwitch", SubTests::ST_BufferSwitch);
  AddSubTest("SubGraphBufferInlining", SubTests::ST_SubGraphBufferInlining);
  AddSubTest("IncompatiblePinConnection", SubTests::ST_IncompatiblePinConnection);
  AddSubTest("SharedSourceSwitch", SubTests::ST_SharedSourceSwitch);
}

ezResult ezGpuPipelineTest::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("GpuPipelineTest");
  return EZ_SUCCESS;
}

ezResult ezGpuPipelineTest::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_pRenderGraph = nullptr;
  return ezGraphicsTest::DeInitializeSubTest(iIdentifier);
}

ezTestAppRun ezGpuPipelineTest::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::ST_DeadPassCulling:
      DeadPassCulling();
      break;
    case SubTests::ST_DependencySorting:
      DependencySorting();
      break;
    case SubTests::ST_PassThroughSorting:
      PassThroughSorting();
      break;
    case SubTests::ST_CycleDetection:
      CycleDetection();
      break;
    case SubTests::ST_TextureSwitch:
      TextureSwitch();
      break;
    case SubTests::ST_SubGraphInlining:
      SubGraphInlining();
      break;
    case SubTests::ST_SubGraphInliningErrors:
      SubGraphInliningErrors();
      break;
    case SubTests::ST_ViewBlackboard:
      ViewBlackboard();
      break;
    case SubTests::ST_BufferPassThroughSorting:
      BufferPassThroughSorting();
      break;
    case SubTests::ST_MixedPassThrough:
      MixedPassThrough();
      break;
    case SubTests::ST_MixedPinIndexing:
      MixedPinIndexing();
      break;
    case SubTests::ST_BufferSwitch:
      BufferSwitch();
      break;
    case SubTests::ST_SubGraphBufferInlining:
      SubGraphBufferInlining();
      break;
    case SubTests::ST_IncompatiblePinConnection:
      IncompatiblePinConnection();
      break;
    case SubTests::ST_SharedSourceSwitch:
      SharedSourceSwitch();
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return ezTestAppRun::Quit;
}

void ezGpuPipelineTest::DeadPassCulling()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiDeadSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "DeadSource");
  const ezUInt32 uiDeadPass = AddPass<ezGpuPipelineTestPass>(passes, "DeadPass");
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");
  const ezUInt32 uiLivePass = AddPass<ezGpuPipelineTestPass>(passes, "LivePass");
  const ezUInt32 uiLiveSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "LiveSource");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiDeadSource, "Output", uiDeadPass, "Input");
  Connect(connections, uiLiveSource, "Output", uiLivePass, "Input");
  Connect(connections, uiLivePass, "Output", uiSink, "InputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"LiveSource", "LivePass", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void ezGpuPipelineTest::DependencySorting()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");
  const ezUInt32 uiBranchB = AddPass<ezGpuPipelineTestPass>(passes, "BranchB");
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");
  const ezUInt32 uiBranchA = AddPass<ezGpuPipelineTestPass>(passes, "BranchA");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiBranchA, "Input");
  Connect(connections, uiSource, "Output", uiBranchB, "Input");
  Connect(connections, uiBranchA, "Output", uiSink, "InputA");
  Connect(connections, uiBranchB, "Output", uiSink, "InputB");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "BranchA", "BranchB", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void ezGpuPipelineTest::PassThroughSorting()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");
  const ezUInt32 uiPassThrough = AddPass<ezGpuPipelineTestPassThroughPass>(passes, "PassThrough");
  const ezUInt32 uiReader = AddPass<ezGpuPipelineTestSinkPass>(passes, "Reader");
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiPassThrough, "Pin");
  Connect(connections, uiSource, "Output", uiReader, "InputA");
  Connect(connections, uiPassThrough, "Pin", uiSink, "InputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Reader", "PassThrough", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void ezGpuPipelineTest::CycleDetection()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiPassA = AddPass<ezGpuPipelineTestPass>(passes, "PassA");
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");
  const ezUInt32 uiPassB = AddPass<ezGpuPipelineTestPass>(passes, "PassB");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiPassA, "Output", uiPassB, "Input");
  Connect(connections, uiPassB, "Output", uiPassA, "Input");
  Connect(connections, uiPassB, "Output", uiSink, "InputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  EZ_TEST_RESULT(pPipeline->CullDeadPasses());

  ezTestLogInterface log;
  ezTestLogSystemScope logSystemScope(&log, true);
  log.ExpectMessage("GPU pipeline contains a cycle", ezLogMsgType::ErrorMsg);
  log.ExpectMessage("Failed to sort pass", ezLogMsgType::ErrorMsg, 3);
  EZ_TEST_BOOL(pPipeline->SortPasses().Failed());
}

void ezGpuPipelineTest::TextureSwitch()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSourceA = AddPass<ezGpuPipelineTestSourcePass>(passes, "SourceA");
  const ezUInt32 uiSourceB = AddPass<ezGpuPipelineTestSourcePass>(passes, "SourceB");

  ezUniquePtr<ezTextureSwitchPass> pSwitch = EZ_DEFAULT_NEW(ezTextureSwitchPass);
  pSwitch->SetName("Switch");
  pSwitch->m_sBlackboardProperty = "Quality";
  pSwitch->m_Values.PushBack(10);
  pSwitch->m_Values.PushBack(20);
  const ezUInt32 uiSwitch = passes.GetCount();
  passes.PushBack(std::move(pSwitch));

  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSourceA, "Output", uiSwitch, "10");
  Connect(connections, uiSourceB, "Output", uiSwitch, "20");
  Connect(connections, uiSwitch, "Output", uiSink, "InputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  EZ_TEST_INT(pPipeline->GetSwitches().GetCount(), 1);
  EZ_TEST_STRING(pPipeline->GetSwitches()[0].m_sBlackboardProperty.GetString(), "Quality");

  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedDefaultOrder[] = {"SourceA", "Sink"};
  TestExecutionOrder(executionOrder, expectedDefaultOrder);

  EZ_TEST_BOOL(pPipeline->SetSwitchValue(0, 20));
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedSwitchedOrder[] = {"SourceB", "Sink"};
  TestExecutionOrder(executionOrder, expectedSwitchedOrder);
  EZ_TEST_BOOL(!pPipeline->SetSwitchValue(0, 20));

  EZ_TEST_BOOL(pPipeline->SetSwitchValue(0, 42));
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  TestExecutionOrder(executionOrder, expectedDefaultOrder);
  EZ_TEST_BOOL(!pPipeline->SetSwitchToDefault(0));
}

void ezGpuPipelineTest::SharedSourceSwitch()
{
  // One source feeding two switches. This graph has no cycle, so sorting must succeed.
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");

  ezUniquePtr<ezTextureSwitchPass> pSwitchX = EZ_DEFAULT_NEW(ezTextureSwitchPass);
  pSwitchX->SetName("SwitchX");
  pSwitchX->m_sBlackboardProperty = "QualityX";
  pSwitchX->m_Values.PushBack(10);
  const ezUInt32 uiSwitchX = passes.GetCount();
  passes.PushBack(std::move(pSwitchX));

  ezUniquePtr<ezTextureSwitchPass> pSwitchY = EZ_DEFAULT_NEW(ezTextureSwitchPass);
  pSwitchY->SetName("SwitchY");
  pSwitchY->m_sBlackboardProperty = "QualityY";
  pSwitchY->m_Values.PushBack(10);
  const ezUInt32 uiSwitchY = passes.GetCount();
  passes.PushBack(std::move(pSwitchY));

  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiSwitchX, "10");
  Connect(connections, uiSource, "Output", uiSwitchY, "10");
  Connect(connections, uiSwitchX, "Output", uiSink, "InputA");
  Connect(connections, uiSwitchY, "Output", uiSink, "InputB");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);

  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedOrder[] = {"Source", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);
}

void ezGpuPipelineTest::SubGraphInlining()
{
  {
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> rootPasses;
    const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(rootPasses, "Source");
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("Basic");
    const ezUInt32 uiSubGraph = 1;
    const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(rootPasses, "Sink");

    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(rootPasses[uiSource].Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(rootPasses[uiSink].Borrow());

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, uiSource, "Output", uiSubGraph, "Input");
    Connect(connections, uiSubGraph, "Output", 2, "InputA");

    InlineTestData data;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 4);
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[0])->GetName(), "Source");
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[1])->GetName(), "Sink");
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[2])->GetName(), "First");
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[3])->GetName(), "Second");
    EZ_TEST_INT(ownedPasses.GetCount(), 4);
    EZ_TEST_INT(connections.GetCount(), 3);
    EZ_TEST_INT(connections[0].m_uiSource, 0);
    EZ_TEST_INT(connections[0].m_uiTarget, 2);
    EZ_TEST_STRING(connections[0].m_sTargetPin, "Input");
    EZ_TEST_INT(connections[1].m_uiSource, 2);
    EZ_TEST_INT(connections[1].m_uiTarget, 1);
    EZ_TEST_STRING(connections[1].m_sSourcePin, "Output");
  }

  {
    ezUniquePtr<ezGpuPipelineTestSourcePass> pSource = EZ_DEFAULT_NEW(ezGpuPipelineTestSourcePass);
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("Forward");
    ezUniquePtr<ezGpuPipelineTestSinkPass> pSink = EZ_DEFAULT_NEW(ezGpuPipelineTestSinkPass);
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "Input");
    Connect(connections, 1, "Output", 2, "InputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::Forward;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 2);
    EZ_TEST_INT(connections.GetCount(), 1);
    EZ_TEST_INT(connections[0].m_uiSource, 0);
    EZ_TEST_INT(connections[0].m_uiTarget, 1);
    EZ_TEST_STRING(connections[0].m_sSourcePin, "Output");
    EZ_TEST_STRING(connections[0].m_sTargetPin, "InputA");
  }

  {
    ezUniquePtr<ezGpuPipelineTestSourcePass> pSource = EZ_DEFAULT_NEW(ezGpuPipelineTestSourcePass);
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("Mixed");
    ezUniquePtr<ezGpuPipelineTestSinkPass> pForwardSink = EZ_DEFAULT_NEW(ezGpuPipelineTestSinkPass);
    ezUniquePtr<ezGpuPipelineTestSinkPass> pProcessedSink = EZ_DEFAULT_NEW(ezGpuPipelineTestSinkPass);
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pForwardSink.Borrow());
    nodes.PushBack(pProcessedSink.Borrow());

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "Input");
    Connect(connections, 1, "Forwarded", 2, "InputA");
    Connect(connections, 1, "Processed", 3, "InputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::Mixed;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 4);
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[3])->GetName(), "Internal");
    EZ_TEST_INT(connections.GetCount(), 3);
    EZ_TEST_INT(connections[0].m_uiSource, 0);
    EZ_TEST_INT(connections[0].m_uiTarget, 3);
    EZ_TEST_INT(connections[1].m_uiSource, 0);
    EZ_TEST_INT(connections[1].m_uiTarget, 1);
    EZ_TEST_INT(connections[2].m_uiSource, 3);
    EZ_TEST_INT(connections[2].m_uiTarget, 2);
  }

  {
    ezUniquePtr<ezGpuPipelineTestSourcePass> pSource = EZ_DEFAULT_NEW(ezGpuPipelineTestSourcePass);
    ezUniquePtr<ezSubGraphNode> pFirstSubGraph = CreateSubGraph("First");
    ezUniquePtr<ezSubGraphNode> pSecondSubGraph = CreateSubGraph("Second");
    ezUniquePtr<ezGpuPipelineTestSinkPass> pSink = EZ_DEFAULT_NEW(ezGpuPipelineTestSinkPass);
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pFirstSubGraph.Borrow());
    nodes.PushBack(pSecondSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "Input");
    Connect(connections, 1, "Output", 2, "Input");
    Connect(connections, 2, "Output", 3, "InputA");

    InlineTestData data;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 6);
    EZ_TEST_INT(ownedPasses.GetCount(), 8);
    EZ_TEST_INT(connections.GetCount(), 5);
    for (ezRenderPipelineNode* pNode : nodes)
    {
      EZ_TEST_BOOL(ezDynamicCast<ezSubGraphNode*>(pNode) == nullptr);
    }
  }

  {
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("Extractors");
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSubGraph.Borrow());

    ezUniquePtr<ezVisibleObjectsExtractor> pRootExtractor = EZ_DEFAULT_NEW(ezVisibleObjectsExtractor);
    ezDynamicArray<ezExtractor*> extractors;
    extractors.PushBack(pRootExtractor.Borrow());

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::Extractors;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 0);
    EZ_TEST_INT(extractors.GetCount(), 2);
    EZ_TEST_BOOL(extractors[0] == pRootExtractor.Borrow());
    EZ_TEST_BOOL(extractors[1]->GetDynamicRTTI() == ezGetStaticRTTI<ezSelectedObjectsExtractor>());
    EZ_TEST_INT(ownedExtractors.GetCount(), 1);
    EZ_TEST_BOOL(ownedExtractors[0].Borrow() == extractors[1]);
  }
}

void ezGpuPipelineTest::SubGraphInliningErrors()
{
  auto Run = [](InlineTestData::Mode mode, const char* szInputPin)
  {
    ezUniquePtr<ezGpuPipelineTestSourcePass> pSource = EZ_DEFAULT_NEW(ezGpuPipelineTestSourcePass);
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("Error");
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, szInputPin);

    InlineTestData data;
    data.m_Mode = mode;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    return ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data));
  };

  EZ_TEST_BOOL(Run(InlineTestData::Mode::Failure, "Input").Failed());
  EZ_TEST_BOOL(Run(InlineTestData::Mode::InvalidConnection, "Input").Failed());
  EZ_TEST_BOOL(Run(InlineTestData::Mode::Basic, "Missing").Failed());
}

void ezGpuPipelineTest::ViewBlackboard()
{
  // SourceA -> Switch("10") , SourceB -> Switch("20") , Switch -> Sink.InputA
  // "Sink.Value" drives a regular property mapping, "Quality" drives the switch mapping.
  ezRenderPipelineResourceDescriptor desc;
  {
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
    AddPass<ezGpuPipelineTestSourcePass>(passes, "SourceA");
    AddPass<ezGpuPipelineTestSourcePass>(passes, "SourceB");

    {
      ezUniquePtr<ezTextureSwitchPass> pSwitch = EZ_DEFAULT_NEW(ezTextureSwitchPass);
      pSwitch->SetName("Switch");
      pSwitch->m_sBlackboardProperty = "Quality";
      pSwitch->m_Values.PushBack(10);
      pSwitch->m_Values.PushBack(20);
      passes.PushBack(std::move(pSwitch));
    }

    AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");

    ezDynamicArray<const ezRenderPipelinePass*> passPointers;
    for (const ezUniquePtr<ezRenderPipelinePass>& pPass : passes)
    {
      passPointers.PushBack(pPass.Borrow());
    }

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 2, "10");
    Connect(connections, 1, "Output", 2, "20");
    Connect(connections, 2, "Output", 3, "InputA");

    ezMemoryStreamContainerWrapperStorage<ezDynamicArray<ezUInt8>> storage(&desc.m_SerializedPipeline);
    ezMemoryStreamWriter writer(&storage);
    EZ_TEST_RESULT(ezRenderPipelineResourceLoader::ExportPipeline(passPointers, {}, connections, writer));
  }

  ezRenderPipelineResourceHandle hPipeline = ezResourceManager::CreateResource<ezRenderPipelineResource>("ViewBlackboardTestPipeline", std::move(desc), "ViewBlackboardTestPipeline");

  ezWorldDesc worldDesc("ViewBlackboardTestWorld");
  ezWorld world(worldDesc);

  ezView* pView = nullptr;
  const ezViewHandle hView = ezRenderWorld::CreateView("ViewBlackboardTest", pView);
  EZ_TEST_BOOL(pView != nullptr);
  if (pView == nullptr)
    return;

  EZ_SCOPE_EXIT(ezRenderWorld::DeleteView(hView));

  const ezSharedPtr<ezBlackboard>& pWorldBlackboard = world.GetBlackboard();
  ezSharedPtr<ezBlackboard> pViewBlackboard = ezBlackboard::Create("ViewBlackboardTestView");

  pWorldBlackboard->SetEntryValue("Sink.Value", 1);
  pWorldBlackboard->SetEntryValue("Quality", 20);

  pView->SetWorld(&world);
  pView->SetBlackboard(pViewBlackboard);
  pView->SetRenderPipelineResource(hPipeline);

  if (!EZ_TEST_BOOL(pView->m_pRenderPipeline != nullptr))
    return;

  auto GetPropertyValue = [&]() -> ezInt32
  {
    auto* pSink = static_cast<ezGpuPipelineTestSinkPass*>(pView->m_pRenderPipeline->GetPassByName("Sink"));
    return pSink != nullptr ? pSink->m_iValue : -1;
  };

  auto GetSwitchIndex = [&]() -> ezInt32
  {
    auto* pSwitch = static_cast<ezSwitchBasePass*>(pView->m_pRenderPipeline->GetPassByName("Switch"));
    return pSwitch != nullptr ? static_cast<ezInt32>(pSwitch->m_uiSelectedValueIndex) : -1;
  };

  auto RemoveEntry = [](const ezSharedPtr<ezBlackboard>& pBlackboard, const char* szName)
  {
    ezHashedString sName;
    sName.Assign(szName);
    pBlackboard->RemoveEntry(sName);
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "World blackboard only")
  {
    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 1);
    EZ_TEST_INT(GetSwitchIndex(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "View blackboard overrides world blackboard")
  {
    pViewBlackboard->SetEntryValue("Sink.Value", 2);
    pViewBlackboard->SetEntryValue("Quality", 10);

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 2);
    EZ_TEST_INT(GetSwitchIndex(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "View blackboard value change")
  {
    pViewBlackboard->SetEntryValue("Sink.Value", 3);
    pViewBlackboard->SetEntryValue("Quality", 20);

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 3);
    EZ_TEST_INT(GetSwitchIndex(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "World blackboard does not override view blackboard")
  {
    pWorldBlackboard->SetEntryValue("Sink.Value", 100);
    pWorldBlackboard->SetEntryValue("Quality", 10);

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 3);
    EZ_TEST_INT(GetSwitchIndex(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Removing view entries falls back to world blackboard")
  {
    RemoveEntry(pViewBlackboard, "Sink.Value");
    RemoveEntry(pViewBlackboard, "Quality");

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 100);
    EZ_TEST_INT(GetSwitchIndex(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Detaching the view blackboard falls back to world blackboard")
  {
    pViewBlackboard->SetEntryValue("Sink.Value", 7);
    pViewBlackboard->SetEntryValue("Quality", 20);

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 7);
    EZ_TEST_INT(GetSwitchIndex(), 1);

    pView->SetBlackboard(ezSharedPtr<ezBlackboard>());

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 100);
    EZ_TEST_INT(GetSwitchIndex(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Removing world entries restores the default value")
  {
    RemoveEntry(pWorldBlackboard, "Sink.Value");
    RemoveEntry(pWorldBlackboard, "Quality");

    pView->EnsureUpToDate();
    EZ_TEST_INT(GetPropertyValue(), 0);
    EZ_TEST_INT(GetSwitchIndex(), 0);
  }
}

void ezGpuPipelineTest::BufferPassThroughSorting()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");
  const ezUInt32 uiPassThrough = AddPass<ezGpuPipelineTestPassThroughPass>(passes, "PassThrough");
  const ezUInt32 uiReader = AddPass<ezGpuPipelineTestSinkPass>(passes, "Reader");
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "BufferOutput", uiPassThrough, "BufferPin");
  Connect(connections, uiSource, "BufferOutput", uiReader, "BufferInputA");
  Connect(connections, uiPassThrough, "BufferPin", uiSink, "BufferInputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Reader", "PassThrough", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);

  const RecordedPin sourceBuffer = GetOutput(executionOrder, "Source", 1);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "Reader", 2), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetOutput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);
}

void ezGpuPipelineTest::MixedPassThrough()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");
  const ezUInt32 uiPassThrough = AddPass<ezGpuPipelineTestPassThroughPass>(passes, "PassThrough");
  const ezUInt32 uiTextureReader = AddPass<ezGpuPipelineTestSinkPass>(passes, "TextureReader");
  const ezUInt32 uiBufferReader = AddPass<ezGpuPipelineTestSinkPass>(passes, "BufferReader");
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiPassThrough, "Pin");
  Connect(connections, uiSource, "Output", uiTextureReader, "InputA");
  Connect(connections, uiSource, "BufferOutput", uiPassThrough, "BufferPin");
  Connect(connections, uiSource, "BufferOutput", uiBufferReader, "BufferInputA");
  Connect(connections, uiPassThrough, "Pin", uiSink, "InputA");
  Connect(connections, uiPassThrough, "BufferPin", uiSink, "BufferInputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  EZ_TEST_INT(executionOrder.GetCount(), 5);

  // Both resources may be modified in place, so both readers have to run before the pass-through.
  TestExecutedBefore(executionOrder, "Source", "TextureReader");
  TestExecutedBefore(executionOrder, "Source", "BufferReader");
  TestExecutedBefore(executionOrder, "TextureReader", "PassThrough");
  TestExecutedBefore(executionOrder, "BufferReader", "PassThrough");
  TestExecutedBefore(executionOrder, "PassThrough", "Sink");

  const RecordedPin sourceTexture = GetOutput(executionOrder, "Source", 0);
  TestPinsEqual(sourceTexture, GetInput(executionOrder, "PassThrough", 0), Connectivity::Texture);
  TestPinsEqual(sourceTexture, GetOutput(executionOrder, "PassThrough", 0), Connectivity::Texture);
  TestPinsEqual(sourceTexture, GetInput(executionOrder, "Sink", 0), Connectivity::Texture);

  const RecordedPin sourceBuffer = GetOutput(executionOrder, "Source", 1);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetOutput(executionOrder, "PassThrough", 1), Connectivity::Buffer);
  TestPinsEqual(sourceBuffer, GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);
}

void ezGpuPipelineTest::MixedPinIndexing()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");
  const ezUInt32 uiPass = AddPass<ezGpuPipelineTestPass>(passes, "Pass");
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiPass, "Input");
  Connect(connections, uiSource, "BufferOutput", uiPass, "BufferInput");
  Connect(connections, uiPass, "Output", uiSink, "InputA");
  Connect(connections, uiPass, "BufferOutput", uiSink, "BufferInputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Pass", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);

  // Texture and buffer pins are indexed independently, in declaration order.
  TestPinsEqual(GetOutput(executionOrder, "Source", 0), GetInput(executionOrder, "Pass", 0), Connectivity::Texture);
  TestPinsEqual(GetOutput(executionOrder, "Source", 1), GetInput(executionOrder, "Pass", 1), Connectivity::Buffer);
  EZ_TEST_BOOL(GetOutput(executionOrder, "Pass", 0).m_Connectivity == Connectivity::Texture);
  EZ_TEST_BOOL(GetOutput(executionOrder, "Pass", 1).m_Connectivity == Connectivity::Buffer);
  EZ_TEST_BOOL(GetInput(executionOrder, "Sink", 0).m_Connectivity == Connectivity::Texture);
  EZ_TEST_BOOL(GetInput(executionOrder, "Sink", 1).m_Connectivity == Connectivity::None);
  EZ_TEST_BOOL(GetInput(executionOrder, "Sink", 2).m_Connectivity == Connectivity::Buffer);
  EZ_TEST_BOOL(GetInput(executionOrder, "Sink", 3).m_Connectivity == Connectivity::None);
}

void ezGpuPipelineTest::BufferSwitch()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSourceA = AddPass<ezGpuPipelineTestSourcePass>(passes, "SourceA");
  const ezUInt32 uiSourceB = AddPass<ezGpuPipelineTestSourcePass>(passes, "SourceB");

  ezUniquePtr<ezBufferSwitchPass> pSwitch = EZ_DEFAULT_NEW(ezBufferSwitchPass);
  pSwitch->SetName("Switch");
  pSwitch->m_sBlackboardProperty = "Quality";
  pSwitch->m_Values.PushBack(10);
  pSwitch->m_Values.PushBack(20);
  const ezUInt32 uiSwitch = passes.GetCount();
  passes.PushBack(std::move(pSwitch));

  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSourceA, "BufferOutput", uiSwitch, "10");
  Connect(connections, uiSourceB, "BufferOutput", uiSwitch, "20");
  Connect(connections, uiSwitch, "Output", uiSink, "BufferInputA");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline = CreatePipeline(std::move(passes), connections);
  EZ_TEST_INT(pPipeline->GetSwitches().GetCount(), 1);

  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedDefaultOrder[] = {"SourceA", "Sink"};
  TestExecutionOrder(executionOrder, expectedDefaultOrder);
  TestPinsEqual(GetOutput(executionOrder, "SourceA", 1), GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);

  EZ_TEST_BOOL(pPipeline->SetSwitchValue(0, 20));
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);
  const char* expectedSwitchedOrder[] = {"SourceB", "Sink"};
  TestExecutionOrder(executionOrder, expectedSwitchedOrder);
  TestPinsEqual(GetOutput(executionOrder, "SourceB", 1), GetInput(executionOrder, "Sink", 2), Connectivity::Buffer);
}

void ezGpuPipelineTest::SubGraphBufferInlining()
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Buffer boundary forwarded directly")
  {
    ezUniquePtr<ezGpuPipelineTestSourcePass> pSource = EZ_DEFAULT_NEW(ezGpuPipelineTestSourcePass);
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("BufferForward");
    ezUniquePtr<ezGpuPipelineTestSinkPass> pSink = EZ_DEFAULT_NEW(ezGpuPipelineTestSinkPass);
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "BufferOutput", 1, "Input");
    Connect(connections, 1, "Output", 2, "BufferInputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::BufferForward;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 2);
    EZ_TEST_INT(connections.GetCount(), 1);
    EZ_TEST_INT(connections[0].m_uiSource, 0);
    EZ_TEST_INT(connections[0].m_uiTarget, 1);
    EZ_TEST_STRING(connections[0].m_sSourcePin, "BufferOutput");
    EZ_TEST_STRING(connections[0].m_sTargetPin, "BufferInputA");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Texture and buffer boundaries in one sub-graph")
  {
    ezUniquePtr<ezGpuPipelineTestSourcePass> pSource = EZ_DEFAULT_NEW(ezGpuPipelineTestSourcePass);
    ezUniquePtr<ezSubGraphNode> pSubGraph = CreateSubGraph("MixedBoundaries");
    ezUniquePtr<ezGpuPipelineTestSinkPass> pSink = EZ_DEFAULT_NEW(ezGpuPipelineTestSinkPass);
    ezDynamicArray<ezRenderPipelineNode*> nodes;
    nodes.PushBack(pSource.Borrow());
    nodes.PushBack(pSubGraph.Borrow());
    nodes.PushBack(pSink.Borrow());

    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
    Connect(connections, 0, "Output", 1, "TextureInput");
    Connect(connections, 0, "BufferOutput", 1, "BufferInput");
    Connect(connections, 1, "TextureOutput", 2, "InputA");
    Connect(connections, 1, "BufferOutput", 2, "BufferInputA");

    InlineTestData data;
    data.m_Mode = InlineTestData::Mode::MixedBoundaries;
    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
    ezDynamicArray<ezExtractor*> extractors;
    ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
    EZ_TEST_BOOL(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ezMakeDelegate(&InlineTestData::Import, &data)).Succeeded());

    EZ_TEST_INT(nodes.GetCount(), 3);
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[1])->GetName(), "Sink");
    EZ_TEST_STRING(ezDynamicCast<ezRenderPipelinePass*>(nodes[2])->GetName(), "Internal");
    EZ_TEST_INT(connections.GetCount(), 4);

    auto HasConnection = [&connections](ezUInt32 uiSource, const char* szSourcePin, ezUInt32 uiTarget, const char* szTargetPin) -> bool
    {
      for (const ezRenderPipelineResourceLoaderConnection& connection : connections)
      {
        if (connection.m_uiSource == uiSource && connection.m_sSourcePin == szSourcePin && connection.m_uiTarget == uiTarget && connection.m_sTargetPin == szTargetPin)
          return true;
      }
      return false;
    };

    EZ_TEST_BOOL(HasConnection(0, "Output", 2, "Input"));
    EZ_TEST_BOOL(HasConnection(0, "BufferOutput", 2, "BufferInput"));
    EZ_TEST_BOOL(HasConnection(2, "Output", 1, "InputA"));
    EZ_TEST_BOOL(HasConnection(2, "BufferOutput", 1, "BufferInputA"));
  }
}

void ezGpuPipelineTest::IncompatiblePinConnection()
{
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  const ezUInt32 uiSource = AddPass<ezGpuPipelineTestSourcePass>(passes, "Source");
  const ezUInt32 uiSink = AddPass<ezGpuPipelineTestSinkPass>(passes, "Sink");

  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  Connect(connections, uiSource, "Output", uiSink, "BufferInputA");
  Connect(connections, uiSource, "BufferOutput", uiSink, "InputA");
  Connect(connections, uiSource, "Output", uiSink, "InputB");

  ezUniquePtr<ezRenderPipelinePassGraph> pPipeline;
  {
    ezTestLogInterface log;
    ezTestLogSystemScope logSystemScope(&log, true);
    log.ExpectMessage("texture pins can't be connected to buffer pins", ezLogMsgType::ErrorMsg, 2);

    pPipeline = CreatePipeline(std::move(passes), connections);
  }

  // The mismatched connections are dropped, the compatible one still works.
  ezDynamicArray<RecordedPass> executionOrder;
  CompileAndExecute(*pPipeline, *m_pRenderGraph, executionOrder);

  const char* expectedOrder[] = {"Source", "Sink"};
  TestExecutionOrder(executionOrder, expectedOrder);

  EZ_TEST_BOOL(GetInput(executionOrder, "Sink", 0).m_Connectivity == Connectivity::None);
  EZ_TEST_BOOL(GetInput(executionOrder, "Sink", 2).m_Connectivity == Connectivity::None);
  TestPinsEqual(GetOutput(executionOrder, "Source", 0), GetInput(executionOrder, "Sink", 1), Connectivity::Texture);
}