#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  void DumpAST(const ezExpressionAST& ast, ezStringView sOutputName)
  {
    ezDGMLGraph dgmlGraph;
    ast.PrintGraph(dgmlGraph);

    ezStringBuilder sFileName;
    sFileName.Format(":output/Expression/{}_AST.dgml", sOutputName);

    ezDGMLGraphWriter dgmlGraphWriter;
    EZ_IGNORE_UNUSED(dgmlGraphWriter);
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      ezLog::Info("AST was dumped to: {}", sFileName);
    }
    else
    {
      ezLog::Error("Failed to dump AST to: {}", sFileName);
    }
  }
} // namespace

EZ_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  ezStringBuilder outputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

  ezExpressionParser::Stream inputs[] = {
    ezExpressionParser::Stream("a", ezProcessingStream::DataType::Float),
    ezExpressionParser::Stream("b", ezProcessingStream::DataType::Float),
    ezExpressionParser::Stream("c", ezProcessingStream::DataType::Float),
    ezExpressionParser::Stream("d", ezProcessingStream::DataType::Float),
  };

  ezExpressionParser::Stream outputs[] = {
    ezExpressionParser::Stream("output", ezProcessingStream::DataType::Float),
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parser")
  {
    ezExpressionParser parser;
    ezExpressionCompiler compiler;

    ezStringView code = "output = -a + b * 2";

    ezExpressionAST ast;
    EZ_TEST_BOOL(parser.Parse(code, inputs, outputs, {}, ast).Succeeded());

    DumpAST(ast, "ParserTest");

    ezExpressionByteCode byteCode;
    EZ_TEST_BOOL(compiler.Compile(ast, byteCode).Succeeded());

    DumpAST(ast, "ParserTest_Opt");
  }
}
