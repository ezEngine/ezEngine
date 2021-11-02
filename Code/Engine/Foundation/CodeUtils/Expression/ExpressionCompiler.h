#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>

class ezExpressionByteCode;

class EZ_FOUNDATION_DLL ezExpressionCompiler
{
public:
  ezExpressionCompiler();
  ~ezExpressionCompiler();

  ezResult Compile(ezExpressionAST& ast, ezExpressionByteCode& out_byteCode);

private:
  ezResult TransformAndOptimizeAST(ezExpressionAST& ast);
  ezResult BuildNodeInstructions(const ezExpressionAST& ast);
  ezResult UpdateRegisterLifetime(const ezExpressionAST& ast);
  ezResult AssignRegisters();
  ezResult GenerateByteCode(const ezExpressionAST& ast, ezExpressionByteCode& out_byteCode);

  using TransformFunc = ezDelegate<ezExpressionAST::Node*(ezExpressionAST::Node*)>;
  ezResult TransformASTPreOrder(ezExpressionAST& ast, TransformFunc func);
  ezResult TransformASTPostOrder(ezExpressionAST& ast, TransformFunc func);

  ezHybridArray<ezExpressionAST::Node*, 64> m_NodeStack;
  ezHybridArray<ezExpressionAST::Node*, 64> m_NodeInstructions;
  ezHashTable<const ezExpressionAST::Node*, ezUInt32> m_NodeToRegisterIndex;
  ezHashTable<ezExpressionAST::Node*, ezExpressionAST::Node*> m_TransformCache;

  ezHashTable<ezHashedString, ezUInt32> m_InputToIndex;
  ezHashTable<ezHashedString, ezUInt32> m_OutputToIndex;
  ezHashTable<ezHashedString, ezUInt32> m_FunctionToIndex;

  struct LiveInterval
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiStart;
    ezUInt32 m_uiEnd;
    const ezExpressionAST::Node* m_pNode;
  };

  ezDynamicArray<LiveInterval> m_LiveIntervals;
};
