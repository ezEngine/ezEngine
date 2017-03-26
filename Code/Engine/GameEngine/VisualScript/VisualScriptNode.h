#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Reflection/Reflection.h>

class ezVisualScriptInstance;
struct ezTriggerMessage;
struct ezCollisionMessage;

class EZ_GAMEENGINE_DLL ezVisualScriptNode : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode, ezReflectedClass);

public:
  ezVisualScriptNode();

  virtual void Execute(ezVisualScriptInstance* pInstance) = 0;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) = 0;

  /// \brief Whether the node has an execution pin (input or output) and thus must be stepped manually. Otherwise it will be implicitly executed on demand.
  bool IsManuallyStepped() const;

protected:

  /// When this is set to true (e.g. in a message handler, the node will be stepped during the next script update)
  bool m_bStepNode = false;

private:
  friend class ezVisualScriptInstance;

  ezUInt16 m_uiNodeID;
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

enum class ezVisualScriptDataPinType
{
  None,
  Number, ///< Numbers are represented as doubles
  Boolean,
  Vec3,
  GameObjectHandle, ///< ezGameObjectHandle
  ComponentHandle, ///< ezComponentHandle
  //ResourceHandle, ///< ezTypelessResourceHandle ?
};

class EZ_GAMEENGINE_DLL ezVisScriptDataPinInAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisScriptDataPinInAttribute, ezPropertyAttribute);

public:
  ezVisScriptDataPinInAttribute() { m_uiPinSlot = 0xff; m_DataType = ezVisualScriptDataPinType::None; }
  ezVisScriptDataPinInAttribute(ezUInt8 uiSlot, ezVisualScriptDataPinType dataType) { m_uiPinSlot = uiSlot; m_DataType = dataType; }

  ezUInt8 m_uiPinSlot;
  ezVisualScriptDataPinType m_DataType;
};

class EZ_GAMEENGINE_DLL ezVisScriptDataPinOutAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisScriptDataPinOutAttribute, ezPropertyAttribute);

public:
  ezVisScriptDataPinOutAttribute() { m_uiPinSlot = 0xff; m_DataType = ezVisualScriptDataPinType::None; }
  ezVisScriptDataPinOutAttribute(ezUInt8 uiSlot, ezVisualScriptDataPinType dataType) { m_uiPinSlot = uiSlot; m_DataType = dataType; }

  ezUInt8 m_uiPinSlot;
  ezVisualScriptDataPinType m_DataType;
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Counter : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Counter, ezVisualScriptNode);
public:
  ezVisualScriptNode_Counter();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }

  double m_Counter = 0;
};


class EZ_GAMEENGINE_DLL ezVisualScriptNode_Printer : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Printer, ezVisualScriptNode);
public:
  ezVisualScriptNode_Printer();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  ezString m_sPrint;
  double m_Value = 0;
};

class EZ_GAMEENGINE_DLL ezVisualScriptNode_If : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_If, ezVisualScriptNode);
public:
  ezVisualScriptNode_If();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  double m_Value1;
  double m_Value2;
};

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Input : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Input, ezVisualScriptNode);
public:
  ezVisualScriptNode_Input();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override { return nullptr; }
  void TriggerMessageHandler(ezTriggerMessage& msg);

  ezUInt32 m_UsageStringHash = 0;
};



