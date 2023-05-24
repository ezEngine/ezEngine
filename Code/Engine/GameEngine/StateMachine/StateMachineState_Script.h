#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/StateMachine/StateMachine.h>

/// \brief A state machine state implementation that can be scripted using e.g. visual scripting.
class EZ_GAMEENGINE_DLL ezStateMachineState_Script : public ezStateMachineState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineState_Script, ezStateMachineState);

public:
  ezStateMachineState_Script(ezStringView sName = ezStringView());
  ~ezStateMachineState_Script();

  virtual void OnEnter(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pFromState) const override;
  virtual void OnExit(ezStateMachineInstance& ref_instance, void* pInstanceData, const ezStateMachineState* pToState) const override;
  virtual void Update(ezStateMachineInstance& ref_instance, void* pInstanceData, ezTime deltaTime) const override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc) override;

  void SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;      // [ property ]

  // Exposed Parameters
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  ezArrayMap<ezHashedString, ezVariant> m_Parameters;

  ezString m_sScriptClassFile;
};
