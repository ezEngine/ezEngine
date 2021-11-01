#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class EZ_FOUNDATION_DLL ezExpressionParser
{
public:
  ezExpressionParser();
  ~ezExpressionParser();

  struct Stream
  {
    Stream(const char* szName, ezProcessingStream::DataType dataType)
      : m_DataType(dataType)
    {
      m_sName.Assign(szName);
    }

    ezHashedString m_sName;
    ezProcessingStream::DataType m_DataType;
  };

  ezResult Parse(ezStringView code, ezArrayPtr<Stream> inputs, ezArrayPtr<Stream> outputs, ezExpressionAST& out_ast);

private:
  void SetupInAndOutputs(ezArrayPtr<Stream> inputs, ezArrayPtr<Stream> outputs);
  ezResult ParseAssignment();
  
  ezExpressionAST::Node* ParseFactor();
  ezExpressionAST::Node* ParseExpression(int iPrecedence = 0);
  ezExpressionAST::Node* ParseUnaryExpression();

  bool AcceptBinaryOperator(ezExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence);
  ezExpressionAST::Node* GetVariable(ezStringView sVarName);

  ezResult Expect(const char* szToken, const ezToken** pExpectedToken = nullptr);
  ezResult Expect(ezTokenType::Enum Type, const ezToken** pExpectedToken = nullptr);

  void ReportError(const ezToken* pToken, const ezFormatString& message);

  /// \brief Checks whether all outputs have been written
  ezResult CheckOutputs();

  ezTokenParseUtils::TokenStream m_TokenStream;
  ezUInt32 m_uiCurrentToken = 0;
  ezExpressionAST* m_pAST = nullptr;

  ezHashTable<ezHashedString, ezExpressionAST::Node*> m_KnownVariables;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionParser_inl.h>
