#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/VisualScript/VisualScriptNode.h>

/// \brief Computes math expression given by string.
///
/// Expression is evaluated lazily on first execution.
class EZ_GAMEENGINE_DLL ezVisualScriptNode_MathExpression : public ezVisualScriptNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptNode_MathExpression, ezVisualScriptNode);

public:
  ezVisualScriptNode_MathExpression();
  ~ezVisualScriptNode_MathExpression();

  virtual void Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin) override;
  virtual void* GetInputPinDataPointer(ezUInt8 uiPin) override;

  const char* GetExpression() const;
  void SetExpression(const char* e);

  double m_ValueA = 0;
  double m_ValueB = 1;
  double m_ValueC = 2;
  double m_ValueD = 3;

private:
  ezMathExpression m_mMathExpression;
};
