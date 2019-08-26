#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Core/World/Declarations.h>

class ezVisualScriptInstance;
struct ezMsgCollision;

class EZ_GAMEENGINE_DLL ezVisualScriptNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode, ezReflectedClass);

public:
  ezVisualScriptNode();
  ~ezVisualScriptNode();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) = 0;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) = 0;

  /// \brief Should return the message ID of the message type that this node wants to handle
  /// 
  /// ie. ezMessage::GetTypeMsgId() of the desired message type
  virtual ezInt32 HandlesMessagesWithID() const;

  /// \brief If HandlesMessagesWithID() returns a valid message ID, messages of that type may be delivered through this function
  virtual void HandleMessage(ezMessage* pMsg);

  /// \brief Whether the node has an execution pin (input or output) and thus must be stepped manually. Otherwise it will be implicitly executed on demand.
  ///
  /// By default this is determined by checking the properties of the ezVisualScriptNode for attributes of type ezVisScriptExecPinOutAttribute
  /// and ezVisScriptExecPinInAttribute. If those exist, it is a manually stepped node. However, derived types can override this to use other criteria.
  virtual bool IsManuallyStepped() const;

protected:

  /// When this is set to true (e.g. in a message handler, the node will be stepped during the next script update)
  bool m_bStepNode = false;
  /// Set to true whenever the input values have been modified before 'Execute' is called. Automatically set to false afterwards.
  bool m_bInputValuesChanged = true;

private:
  friend class ezVisualScriptInstance;

  ezUInt16 m_uiNodeID;
};


#define EZ_INPUT_EXECUTION_PIN(name, slot) EZ_CONSTANT_PROPERTY(name, 0)->AddAttributes(new ezVisScriptExecPinInAttribute(slot))
#define EZ_OUTPUT_EXECUTION_PIN(name, slot) EZ_CONSTANT_PROPERTY(name, 0)->AddAttributes(new ezVisScriptExecPinOutAttribute(slot))
#define EZ_INPUT_DATA_PIN(name, slot, type) EZ_CONSTANT_PROPERTY(name, 0)->AddAttributes(new ezVisScriptDataPinInAttribute(slot, type))
#define EZ_OUTPUT_DATA_PIN(name, slot, type) EZ_CONSTANT_PROPERTY(name, 0)->AddAttributes(new ezVisScriptDataPinOutAttribute(slot, type))
#define EZ_INPUT_DATA_PIN_AND_PROPERTY(name, slot, type, member) EZ_MEMBER_PROPERTY(name, member)->AddAttributes(new ezVisScriptDataPinInAttribute(slot, type))

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

  ezGameObjectHandle m_hObject;
  ezComponentHandle m_hComponent;
  ezTime m_Delay;
  bool m_bRecursive;
  ezMessage* m_pMessageToSend = nullptr;
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

  static void ConvertArgumentToRequiredType(ezVariant& var, ezVariantType::Enum type);

  /// \brief Enforces m_ReturnValue and m_Arguments to be supported types, ie. mostly doubles for number types
  static void EnforceVariantTypeForInputPins(ezVariant& var);

  const ezRTTI* m_pExpectedType = nullptr;
  const ezAbstractFunctionProperty* m_pFunctionToCall = nullptr;
  ezComponentHandle m_hComponent;
  ezVariant m_ReturnValue;
  ezHybridArray<ezVariant, 4> m_Arguments;
};


//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisScriptExecPinOutAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisScriptExecPinOutAttribute, ezPropertyAttribute);

public:
  explicit ezVisScriptExecPinOutAttribute(ezUInt8 uiSlot = 0xFF) { m_uiPinSlot = uiSlot; }

  ezUInt8 m_uiPinSlot;
};

class EZ_GAMEENGINE_DLL ezVisScriptExecPinInAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisScriptExecPinInAttribute, ezPropertyAttribute);

public:
  explicit ezVisScriptExecPinInAttribute(ezUInt8 uiSlot = 0) { m_uiPinSlot = uiSlot; }

  ezUInt8 m_uiPinSlot;
};

struct EZ_GAMEENGINE_DLL ezVisualScriptDataPinType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    Number, ///< Numbers are represented as doubles
    Boolean,
    Vec3,
    GameObjectHandle, ///< ezGameObjectHandle
    ComponentHandle, ///< ezComponentHandle
    //ResourceHandle, ///< ezTypelessResourceHandle ?
    Default = None,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezVisualScriptDataPinType);

class EZ_GAMEENGINE_DLL ezVisScriptDataPinInAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisScriptDataPinInAttribute, ezPropertyAttribute);

public:
  ezVisScriptDataPinInAttribute() { m_uiPinSlot = 0xff; m_DataType = ezVisualScriptDataPinType::None; }
  ezVisScriptDataPinInAttribute(ezUInt8 uiSlot, ezVisualScriptDataPinType::Enum dataType) { m_uiPinSlot = uiSlot; m_DataType = dataType; }

  ezUInt8 m_uiPinSlot;
  ezEnum<ezVisualScriptDataPinType> m_DataType;
};

class EZ_GAMEENGINE_DLL ezVisScriptDataPinOutAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisScriptDataPinOutAttribute, ezPropertyAttribute);

public:
  ezVisScriptDataPinOutAttribute() { m_uiPinSlot = 0xff; m_DataType = ezVisualScriptDataPinType::None; }
  ezVisScriptDataPinOutAttribute(ezUInt8 uiSlot, ezVisualScriptDataPinType::Enum dataType) { m_uiPinSlot = uiSlot; m_DataType = dataType; }

  ezUInt8 m_uiPinSlot;
  ezEnum<ezVisualScriptDataPinType> m_DataType;
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
  double m_Value1 = 0;
  double m_Value2 = 0;
};

