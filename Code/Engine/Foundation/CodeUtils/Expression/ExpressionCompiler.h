#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/Delegate.h>

class ezExpressionByteCode;

class EZ_FOUNDATION_DLL ezExpressionCompiler
{
public:
  ezExpressionCompiler();
  ~ezExpressionCompiler();

  ezResult Compile(ezExpressionAST& ref_ast, ezExpressionByteCode& out_byteCode, ezStringView sDebugAstOutputPath = ezStringView());

private:
  ezResult TransformAndOptimizeAST(ezExpressionAST& ast, ezStringView sDebugAstOutputPath);
  ezResult BuildNodeInstructions(const ezExpressionAST& ast);
  ezResult UpdateRegisterLifetime();
  ezResult AssignRegisters();
  ezResult GenerateByteCode(const ezExpressionAST& ast, ezExpressionByteCode& out_byteCode);
  ezResult GenerateConstantByteCode(const ezExpressionAST::Constant* pConstant);

  using TransformFunc = ezDelegate<ezExpressionAST::Node*(ezExpressionAST::Node*)>;
  ezResult TransformASTPreOrder(ezExpressionAST& ast, TransformFunc func);
  ezResult TransformASTPostOrder(ezExpressionAST& ast, TransformFunc func);
  ezResult TransformNode(ezExpressionAST::Node*& pNode, TransformFunc& func);
  ezResult TransformOutputNode(ezExpressionAST::Output*& pOutputNode, TransformFunc& func);

  void DumpAST(const ezExpressionAST& ast, ezStringView sOutputPath, ezStringView sSuffix);

  ezHybridArray<ezExpressionAST::Node*, 64> m_NodeStack;
  ezHybridArray<ezExpressionAST::Node*, 64> m_NodeInstructions;
  ezHashTable<const ezExpressionAST::Node*, ezUInt32> m_NodeToRegisterIndex;
  ezHashTable<ezExpressionAST::Node*, ezExpressionAST::Node*> m_TransformCache;

  ezHashTable<ezHashedString, ezUInt32> m_InputToIndex;
  ezHashTable<ezHashedString, ezUInt32> m_OutputToIndex;
  ezHashTable<ezHashedString, ezUInt32> m_FunctionToIndex;

  ezDynamicArray<ezUInt32> m_ByteCode;

  struct LiveInterval
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiStart;
    ezUInt32 m_uiEnd;
    const ezExpressionAST::Node* m_pNode;
  };

  ezDynamicArray<LiveInterval> m_LiveIntervals;
};
