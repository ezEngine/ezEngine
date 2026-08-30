#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/Passes/SwitchPass.h>
#include <RendererCore/Pipeline/SubGraphNode.h>
#include <RendererCore/RendererCoreDLL.h>

/// The static pass graph of an ezRenderPipeline.
///
/// Owns the passes and extractors of a pipeline and stores their connectivity in a flat, index-based form. The graph itself never changes after construction, only which parts of it are considered alive, which depends on the current switch values. Culling and sorting have to run again whenever a switch value changed.
class EZ_RENDERERCORE_DLL ezRenderPipelinePassGraph
{
public:
  struct SwitchInfo
  {
    ezSwitchBasePass* m_pSwitch = nullptr;
    ezHashedString m_sBlackboardProperty;
  };

  /// Resolves the connections against the pins of the given passes. Connections that reference an unknown pin are dropped with a warning.
  ezRenderPipelinePassGraph(ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>&& passes, ezDynamicArray<ezUniquePtr<ezExtractor>>&& extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections);

  /// Determines which passes and connections contribute to the pipeline's output, starting from the passes that have no outputs.
  ///
  /// A switch only keeps the branch that feeds its selected input alive.
  ezResult CullDeadPasses();

  /// Brings the alive passes into an execution order. Requires CullDeadPasses to have run.
  ///
  /// Fails if the alive part of the graph contains a cycle. Two pass-through consumers on the same output are also a cycle, because each of them has to run after the other.
  ezResult SortPasses();

  ezArrayPtr<ezUniquePtr<ezRenderPipelinePass>> GetPasses() { return m_Passes; }
  ezArrayPtr<const ezUniquePtr<ezRenderPipelinePass>> GetPasses() const { return m_Passes; }
  ezArrayPtr<ezUniquePtr<ezExtractor>> GetExtractors() { return m_Extractors; }
  ezArrayPtr<const ezUniquePtr<ezExtractor>> GetExtractors() const { return m_Extractors; }
  ezRenderPipelinePass* GetPassByName(ezStringView sName) const;
  ezExtractor* GetExtractorByName(ezStringView sName) const;
  ezArrayPtr<const SwitchInfo> GetSwitches() const { return m_Switches; }

  /// \return Whether the selection changed, in which case CullDeadPasses and SortPasses have to run again.
  bool SetSwitchValue(ezUInt32 uiSwitchIndex, ezInt32 iValue);

  /// Selects a switch's first value. Used when no blackboard provides a value for it.
  ///
  /// \return Whether the selection changed, in which case CullDeadPasses and SortPasses have to run again.
  bool SetSwitchToDefault(ezUInt32 uiSwitchIndex);

  /// Runs all alive passes in the sorted order and lets them add their work to the render graph.
  ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph);

  /// Lets passes with a texture provider pin replace the imported texture of their connection with an externally owned one.
  ezStatus UpdateTextureProviders(ezRenderGraph& ref_graph);

private:
  void SortExtractors();

  static constexpr ezUInt16 s_uiInvalidIndex = ezMath::MaxValue<ezUInt16>();

  struct PinInfo
  {
    ezUInt16 m_uiPassIndex = 0;
    ezUInt8 m_uiInputPinIndex = 0;  ///< 0xFF if this is not an input pin.
    ezUInt8 m_uiOutputPinIndex = 0; ///< 0xFF if this is not an output pin.
    ezBitflags<ezRenderPipelineNodePin::Type> m_Flags;
  };

  /// One output pin and every input pin that it feeds.
  struct ConnectionInfo
  {
    ezUInt16 m_uiOutputPin = 0;         ///< Pin index.
    ezArrayPtr<ezUInt16> m_uiInputPins; ///< Pin indices, points into m_InputPinsStorage.
  };

  struct PassInfo
  {
    ezArrayPtr<ezUInt16> m_uiInputConnections;  ///< Connection index per input pin, points into m_InputConnectionsStorage. s_uiInvalidIndex if the pin is unconnected.
    ezArrayPtr<ezUInt16> m_uiOutputConnections; ///< Connection index per output pin, points into m_OutputConnectionsStorage. s_uiInvalidIndex if the pin is unconnected.
  };

  /// \name Compact immutable graph state: written only once in ctor
  ///@{

  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> m_Passes;
  ezDynamicArray<ezUniquePtr<ezExtractor>> m_Extractors;

  // The array pointers in PassInfo and ConnectionInfo reference these.
  ezDynamicArray<ezUInt16> m_InputConnectionsStorage;
  ezDynamicArray<ezUInt16> m_OutputConnectionsStorage;
  ezDynamicArray<ezUInt16> m_InputPinsStorage;

  ezDynamicArray<PassInfo> m_PassInfos;           // pass idx
  ezDynamicArray<PinInfo> m_Pins;                 // pin idx
  ezDynamicArray<ConnectionInfo> m_Connections;   // connection idx
  ezDynamicArray<ezUInt16> m_TextureProviderPins; // pin idx
  ezDynamicArray<SwitchInfo> m_Switches;

  ///@}
  /// \name CullDeadPasses / SortPasses result
  ///@{

  ezDynamicBitfield m_AlivePasses;         // pass idx
  ezDynamicBitfield m_AliveConnections;    // connection idx
  ezDynamicArray<ezUInt16> m_SortedPasses; // pass indices, in execution order

  ///@}

  // Temp array that hold the connection's ezRenderGraphTextureHandle / ezRenderGraphBufferHandle during AddRenderPasses
  ezDynamicArray<ezRenderPipelinePinConnection> m_ConnectionsRenderGraph; // connection idx
};
