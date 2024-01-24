#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

class EZ_FOUNDATION_DLL ezExpressionParser
{
public:
  ezExpressionParser();
  ~ezExpressionParser();

  static const ezHashTable<ezHashedString, ezEnum<ezExpressionAST::DataType>>& GetKnownTypes();
  static const ezHashTable<ezHashedString, ezEnum<ezExpressionAST::NodeType>>& GetBuiltinFunctions();

  void RegisterFunction(const ezExpression::FunctionDesc& funcDesc);
  void UnregisterFunction(const ezExpression::FunctionDesc& funcDesc);

  struct Options
  {
    bool m_bTreatUnknownVariablesAsInputs = false;
  };

  ezResult Parse(ezStringView sCode, ezArrayPtr<ezExpression::StreamDesc> inputs, ezArrayPtr<ezExpression::StreamDesc> outputs, const Options& options, ezExpressionAST& out_ast);

private:
  static constexpr int s_iLowestPrecedence = 20;

  static void RegisterKnownTypes();
  static void RegisterBuiltinFunctions();
  void SetupInAndOutputs(ezArrayPtr<ezExpression::StreamDesc> inputs, ezArrayPtr<ezExpression::StreamDesc> outputs);

  ezResult ParseStatement();
  ezResult ParseType(ezStringView sTypeName, ezEnum<ezExpressionAST::DataType>& out_type);
  ezResult ParseVariableDefinition(ezEnum<ezExpressionAST::DataType> type);
  ezResult ParseAssignment();

  ezExpressionAST::Node* ParseFactor();
  ezExpressionAST::Node* ParseExpression(int iPrecedence = s_iLowestPrecedence);
  ezExpressionAST::Node* ParseUnaryExpression();
  ezExpressionAST::Node* ParseFunctionCall(ezStringView sFunctionName);
  ezExpressionAST::Node* ParseSwizzle(ezExpressionAST::Node* pExpression);

  bool AcceptStatementTerminator();
  bool AcceptOperator(ezStringView sName);
  bool AcceptBinaryOperator(ezExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, ezUInt32& out_uiOperatorLength);
  ezExpressionAST::Node* GetVariable(ezStringView sVarName);
  ezExpressionAST::Node* EnsureExpectedType(ezExpressionAST::Node* pNode, ezExpressionAST::DataType::Enum expectedType);
  ezExpressionAST::Node* Unpack(ezExpressionAST::Node* pNode, bool bUnassignedError = true);

  ezResult Expect(ezStringView sToken, const ezToken** pExpectedToken = nullptr);
  ezResult Expect(ezTokenType::Enum Type, const ezToken** pExpectedToken = nullptr);

  void ReportError(const ezToken* pToken, const ezFormatString& message);

  /// \brief Checks whether all outputs have been written
  ezResult CheckOutputs();

  Options m_Options;

  ezTokenParseUtils::TokenStream m_TokenStream;
  ezUInt32 m_uiCurrentToken = 0;
  ezExpressionAST* m_pAST = nullptr;

  ezHashTable<ezHashedString, ezExpressionAST::Node*> m_KnownVariables;
  ezHashTable<ezHashedString, ezHybridArray<ezExpression::FunctionDesc, 1>> m_FunctionDescs;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionParser_inl.h>
