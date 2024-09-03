#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/MathExpression.h>

static ezHashedString s_sOutput = ezMakeHashedString("output");

ezMathExpression::ezMathExpression() = default;

ezMathExpression::ezMathExpression(ezStringView sExpressionString)
{
  Reset(sExpressionString);
}

void ezMathExpression::Reset(ezStringView sExpressionString)
{
  m_sOriginalExpression.Assign(sExpressionString);
  m_ByteCode.Clear();
  m_bIsValid = false;

  if (sExpressionString.IsEmpty())
    return;

  ezStringBuilder tmp = s_sOutput.GetView();
  tmp.Append(" = ", sExpressionString);

  ezExpression::StreamDesc outputs[] = {
    {s_sOutput, ezProcessingStream::DataType::Float},
  };

  ezExpressionParser parser;
  ezExpressionParser::Options parserOptions;
  parserOptions.m_bTreatUnknownVariablesAsInputs = true;

  ezExpressionAST ast;
  if (parser.Parse(tmp, ezArrayPtr<ezExpression::StreamDesc>(), outputs, parserOptions, ast).Failed())
    return;

  ezExpressionCompiler compiler;
  if (compiler.Compile(ast, m_ByteCode).Failed())
    return;

  m_bIsValid = true;
}

float ezMathExpression::Evaluate(ezArrayPtr<Input> inputs)
{
  float fOutput = ezMath::NaN<float>();

  if (!IsValid() || m_ByteCode.IsEmpty())
  {
    ezLog::Error("Can't evaluate invalid math expression '{0}'", m_sOriginalExpression);
    return fOutput;
  }

  ezHybridArray<ezProcessingStream, 8> inputStreams;
  for (auto& input : inputs)
  {
    if (input.m_sName.IsEmpty())
      continue;

    inputStreams.PushBack(ezProcessingStream(input.m_sName, ezMakeArrayPtr(&input.m_fValue, 1).ToByteArray(), ezProcessingStream::DataType::Float));
  }

  ezProcessingStream outputStream(s_sOutput, ezMakeArrayPtr(&fOutput, 1).ToByteArray(), ezProcessingStream::DataType::Float);
  ezArrayPtr<ezProcessingStream> outputStreams = ezMakeArrayPtr(&outputStream, 1);

  if (m_VM.Execute(m_ByteCode, inputStreams, outputStreams, 1).Failed())
  {
    ezLog::Error("Failed to execute expression VM");
  }

  return fOutput;
}
