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
  virtual void SetInputPinValue(ezUInt8 uiPin, const ezVariant& value) = 0;

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

enum class ezVisualScriptPinCategory
{
  Execution,
  Number,
  Vector,
  Boolean,
  //ObjectReference,
  //ComponentReference,
  //ResourceReference,
};

class EZ_GAMEENGINE_DLL ezVisualScriptInputPinAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptInputPinAttribute, ezPropertyAttribute);

public:
  ezVisualScriptInputPinAttribute() {}
  ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory category)
  {
    m_Category = category;
  }

  ezVisualScriptPinCategory GetCategory() const { return m_Category; }

private:
  ezVisualScriptPinCategory m_Category;
};

class EZ_GAMEENGINE_DLL ezVisualScriptOutputPinAttribute : public ezPropertyAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptOutputPinAttribute, ezPropertyAttribute);

public:
  ezVisualScriptOutputPinAttribute() {}
  ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory category)
  {
    m_Category = category;
  }

  ezVisualScriptPinCategory GetCategory() const { return m_Category; }

private:
  ezVisualScriptPinCategory m_Category;
};





//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Counter : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Counter, ezVisualScriptNode);
public:
  ezVisualScriptNode_Counter();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void SetInputPinValue(ezUInt8 uiPin, const ezVariant& value) override {};

  ezInt32 m_iCounter = 0;
};


class EZ_GAMEENGINE_DLL ezVisualScriptNode_Printer : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Printer, ezVisualScriptNode);
public:
  ezVisualScriptNode_Printer();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void SetInputPinValue(ezUInt8 uiPin, const ezVariant& value) override;

  ezString m_sPrint;
  ezInt32 m_iValue = 0;
};

class EZ_GAMEENGINE_DLL ezVisualScriptNode_If : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_If, ezVisualScriptNode);
public:
  ezVisualScriptNode_If();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  virtual void SetInputPinValue(ezUInt8 uiPin, const ezVariant& value) override;

  double m_Value1;
  double m_Value2;
};

class EZ_GAMEENGINE_DLL ezVisualScriptNode_Input : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_Input, ezVisualScriptNode);
public:
  ezVisualScriptNode_Input();

  virtual void Execute(ezVisualScriptInstance* pInstance) override;
  void TriggerMessageHandler(ezTriggerMessage& msg);

  ezUInt32 m_UsageStringHash = 0;

  virtual void SetInputPinValue(ezUInt8 uiPin, const ezVariant& value) override {}

};



