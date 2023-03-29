#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Sequence : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Sequence, ezVisualScriptNode);

public:
  ezVisualScriptNode_Sequence();
  ~ezVisualScriptNode_Sequence();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Delay : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Delay, ezVisualScriptNode);

public:
  ezVisualScriptNode_Delay();
  ~ezVisualScriptNode_Delay();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;
  virtual bool IsManuallyStepped() const override { return true; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

private:
  ezTime m_Delay;
  ezHashedString m_sMessage;
  bool m_bMessageReceived = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Log : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Log, ezVisualScriptNode);

public:
  ezVisualScriptNode_Log();
  ~ezVisualScriptNode_Log();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sLog;
  ezVariant m_Value0 = 0;
  ezVariant m_Value1 = 0;
  ezVariant m_Value2 = 0;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_MessageSender : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_MessageSender, ezVisualScriptNode);

public:
  ezVisualScriptNode_MessageSender();
  ~ezVisualScriptNode_MessageSender();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;
  virtual bool IsManuallyStepped() const override { return true; }

  void SetMessageToSend(ezUniquePtr<ezMessage>&& pMsg);

  ezTime m_Delay;
  bool m_bRecursive = false;

private:
  ezGameObjectHandle m_hObject;
  ezComponentHandle m_hComponent;
  ezUniquePtr<ezMessage> m_pMessageToSend;

  ezSmallArray<ezUInt16, 8> m_PropertyIndexToMemoryOffset;
  ezSmallArray<ezEnum<ezVisualScriptDataPinType>, 8> m_PropertyIndexToDataPinType;
  ezBlob m_ScratchMemory;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_MessageHandler : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_MessageHandler, ezVisualScriptNode);

public:
  ezVisualScriptNode_MessageHandler();
  ~ezVisualScriptNode_MessageHandler();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  virtual bool IsManuallyStepped() const override { return true; }
  virtual ezInt32 HandlesMessagesWithID() const override;
  virtual void HandleMessage(ezMessage* pMsg) override;

  const ezRTTI* m_pMessageTypeToHandle = nullptr;
  ezUniquePtr<ezMessage> m_pMsgCopy;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_FunctionCall : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_FunctionCall, ezVisualScriptNode);

public:
  ezVisualScriptNode_FunctionCall();
  ~ezVisualScriptNode_FunctionCall();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;
  virtual bool IsManuallyStepped() const override { return true; }

  static ezResult ConvertArgumentToRequiredType(ezVariant& ref_var, ezVariantType::Enum type);

  const ezRTTI* m_pExpectedType = nullptr;
  const ezAbstractFunctionProperty* m_pFunctionToCall = nullptr;
  ezGameObjectHandle m_hObject;
  ezComponentHandle m_hComponent;
  ezVariant m_ReturnValue;
  ezHybridArray<ezVariant, 4> m_Arguments;
  ezUInt16 m_ArgumentIsOutParamMask = 0; // the n-th EZ_BIT is set if m_Arguments[n] represents an out or inout parameter
};
