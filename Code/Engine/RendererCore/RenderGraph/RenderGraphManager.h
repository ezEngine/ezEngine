#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/RenderGraph/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezGALDeviceEvent;
class ezGALResourceStateTracker;
class ezRenderGraph;
class ezRenderGraphPassObserver;
class ezRenderGraphResourcePool;
struct ezTextureValidationError;
struct ezBufferValidationError;

struct ezRenderGraphRenderEvent
{
  enum class Type
  {
    BeginRender,          ///< Fired before rendering begins.
    BeforeGraphExecution, ///< Fired before executing a render graph.
    AfterGraphExecution,  ///< Fired after executing a render graph.
    EndRender,            ///< Fired after rendering is complete.
  };

  Type m_Type;
  ezRenderGraph* m_pGraph = nullptr;
  ezRenderGraphContext* m_pContext = nullptr;
};

/// \brief Manages ezRenderGraph lifetime and execution.
class EZ_RENDERERCORE_DLL ezRenderGraphManager
{
public:
  /// Creates a new render graph. The caller holds a reference via the returned scoped pointer.
  /// When all references are released, the graph is deleted at the end of the frame.
  static ezSharedPtr<ezRenderGraph> CreateRenderGraph(ezStringView sName, ezEnum<ezRenderGraphPhase> phase = ezRenderGraphPhase::Render);

  /// Enqueue a render graph for execution in the current frame. Must be called before ExecuteRenderGraphs. Usually in ezRenderWorldRenderEvent::Type::BeginRender or ezGALDeviceEvent::AfterBeginFrame.
  static void EnqueueRenderGraph(const ezSharedPtr<ezRenderGraph>& pRenderGraph);

  /// Must be called after ezGALDeviceEvent::BeforeBeginFrame and before ezGALDeviceEvent::AfterEndFrame.
  static void ExecuteRenderGraphs(ezGALDevice* pDevice);

  /// Access the shared resource pool. Only valid after device init.
  static ezRenderGraphResourcePool* GetResourcePool();

  /// \name Pass Observers
  ///@{

  /// Fills a summary with the render graphs and available swapchains.
  /// Only safe to call from an ezRenderGraphRenderEvent::Type::AfterGraphExecution callback registered through s_RenderEvent.
  static void GetExecutionSummary(ezRenderGraphInspectionSummary& out_summary);

  /// Fills an inspection info of a specific render graph.
  /// Only safe to call from an ezRenderGraphRenderEvent::Type::AfterGraphExecution callback registered through s_RenderEvent.
  static ezResult GetRenderGraphInspectionInfo(ezUInt64 uiRenderGraphId, ezRenderGraphInspectionInfo& out_inspectionInfo);

  static ezSharedPtr<ezRenderGraphPassObserver> CreateObserver();

  ///@}

  static ezEvent<const ezRenderGraphRenderEvent&, ezMutex> s_RenderEvent;

  /// Prints all operations and barriers affecting a texture resource up to the current execution point.
  /// Only operations overlapping the given range are printed.
  static void PrintTextureResourceHistory(const ezTextureValidationError& error);

  /// Prints all operations and barriers affecting a buffer resource up to the current execution point.
  /// Call from validation failure handlers to diagnose barrier issues.
  static void PrintBufferResourceHistory(const ezBufferValidationError& error);

private:
  friend class ezRenderGraph;
  friend class ezRenderGraphPassObserver;

  static void OnEngineStartup();
  static void OnEngineShutdown();
  static void GALDeviceEventHandler(const ezGALDeviceEvent& e);

  static void InitPool(ezGALDevice* pDevice);
  static void DeinitPool(ezGALDevice* pDevice);
  static void BeginFrame();
  static void OnGraphDestroyed(ezRenderGraph* pGraph);
  static ezUInt64 GetRenderGraphId(ezRenderGraph* pGraph);
  static ezRenderGraph* GetRenderGraphById(ezUInt64 uiRenderGraphId);
  static ezUInt32 GetSwapChainId(ezGALSwapChainHandle hSwapChain);
  static ezGALSwapChainHandle GetSwapChainById(ezUInt32 uiSwapChainId);

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderGraphManager);

  static ezMutex s_Mutex;
  static ezDynamicArray<ezSharedPtr<ezRenderGraph>> s_EnqueuedRenderGraphs[3];
  static ezDynamicArray<ezRenderGraph*> s_AllRenderGraphs[3];
  static ezUniquePtr<ezRenderGraphResourcePool> s_pPool;
  static ezUniquePtr<ezGALResourceStateTracker> s_pStateTracker;

  // Pass observers
  static ezDynamicArray<ezSharedPtr<ezRenderGraphPassObserver>> s_Observers;
  static ezSharedPtr<ezRenderGraph> s_pObserverGraph;

  // Execution tracking for diagnostics
  static ezDynamicArray<ezRenderGraph*> s_ExecutingGraphs; ///< Executed this frame, kept alive via s_EnqueuedRenderGraphs
  static ezDynamicArray<ezRenderGraphPassObserver*> s_ExecutingObservers;
  static ezUInt32 s_uiCurrentGraphIndex;
  static ezUInt32 s_uiCurrentPassIndex;
};
