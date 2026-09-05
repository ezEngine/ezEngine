#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Forwards one of several inputs to its output, depending on a value read from a blackboard.
///
/// The pin names are the string representations of the entries in m_Values, so changing the values invalidates existing connections. Only the branch that feeds the selected input is kept alive, everything that solely feeds the other branches is culled from the pipeline. Changing the selected value therefore requires a rebuild of the pass graph.
class EZ_RENDERERCORE_DLL ezSwitchBasePass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSwitchBasePass, ezRenderPipelinePass);

public:
  /// Maximum number of inputs. Also limits how many entries m_Values may have.
  static constexpr ezUInt32 s_uiMaxInputs = 8;

  /// Selects the input whose entry in m_Values equals iValue. Falls back to the first entry if the value is unknown.
  ///
  /// \return Whether the selection actually changed. If it did, the pass graph has to be rebuilt.
  bool SetSwitchValue(ezInt32 iValue);

  virtual ezStatus AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  ezString m_sBlackboardProperty;     ///< Name of the blackboard entry that selects the active input. If it is empty or missing, the switch uses its first value.
  ezDynamicArray<ezInt32> m_Values;   ///< The value that selects each input pin. Must be unique and must not hold more than s_uiMaxInputs entries.
  ezUInt8 m_uiSelectedValueIndex = 0; ///< Index into m_Values, and therefore also the index of the active input pin.

protected:
  ezSwitchBasePass(const char* szName);

  void AddDynamicInputPins(ezArrayPtr<const ezRenderPipelineNodePin* const> pins, ezHashTable<ezHashedString, const ezRenderPipelineNodePin*>& ref_nameToPin);
};

/// ezSwitchBasePass for texture connections.
class EZ_RENDERERCORE_DLL ezTextureSwitchPass : public ezSwitchBasePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTextureSwitchPass, ezSwitchBasePass);

public:
  ezTextureSwitchPass();
  virtual void AddDynamicPins(ezHashTable<ezHashedString, const ezRenderPipelineNodePin*>& ref_nameToPin) override;

  ezRenderPipelineNodeOutputPin m_Output;
  ezRenderPipelineNodeInputPin m_Input0;
  ezRenderPipelineNodeInputPin m_Input1;
  ezRenderPipelineNodeInputPin m_Input2;
  ezRenderPipelineNodeInputPin m_Input3;
  ezRenderPipelineNodeInputPin m_Input4;
  ezRenderPipelineNodeInputPin m_Input5;
  ezRenderPipelineNodeInputPin m_Input6;
  ezRenderPipelineNodeInputPin m_Input7;
};

/// ezSwitchBasePass for buffer connections.
class EZ_RENDERERCORE_DLL ezBufferSwitchPass : public ezSwitchBasePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBufferSwitchPass, ezSwitchBasePass);

public:
  ezBufferSwitchPass();
  virtual void AddDynamicPins(ezHashTable<ezHashedString, const ezRenderPipelineNodePin*>& ref_nameToPin) override;

  ezRenderPipelineNodeBufferOutputPin m_Output;
  ezRenderPipelineNodeBufferInputPin m_Input0;
  ezRenderPipelineNodeBufferInputPin m_Input1;
  ezRenderPipelineNodeBufferInputPin m_Input2;
  ezRenderPipelineNodeBufferInputPin m_Input3;
  ezRenderPipelineNodeBufferInputPin m_Input4;
  ezRenderPipelineNodeBufferInputPin m_Input5;
  ezRenderPipelineNodeBufferInputPin m_Input6;
  ezRenderPipelineNodeBufferInputPin m_Input7;
};
