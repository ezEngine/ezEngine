#pragma once

#include <ProceduralPlacementPlugin/VM/ExpressionAST.h>

class ezExpressionByteCode;

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezExpressionCompiler
{
public:
  ezExpressionCompiler();
  ~ezExpressionCompiler();

  ezResult Compile(ezExpressionAST& ast, ezExpressionByteCode& out_byteCode);

private:
  ezResult BuildNodeInstructions(const ezExpressionAST& ast);
  ezResult UpdateRegisterLifetime(const ezExpressionAST& ast);
  ezResult AssignRegisters();
  ezResult GenerateByteCode(const ezExpressionAST& ast, ezExpressionByteCode& out_byteCode);

  ezHybridArray<const ezExpressionAST::Node*, 64> m_NodeStack;
  ezHybridArray<const ezExpressionAST::Node*, 64> m_NodeInstructions;
  ezHashTable<const ezExpressionAST::Node*, ezUInt32> m_NodeToRegisterIndex;

  struct LiveInterval
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiStart;
    ezUInt32 m_uiEnd;
    const ezExpressionAST::Node* m_pNode;
  };

  ezDynamicArray<LiveInterval> m_LiveIntervals;
};

