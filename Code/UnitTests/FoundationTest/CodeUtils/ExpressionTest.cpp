#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
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
    if (dgmlGraphWriter.WriteGraphToFile(sFileName, dgmlGraph).Succeeded())
    {
      ezLog::Info("AST was dumped to: {}", sFileName);
    }
    else
    {
      ezLog::Error("Failed to dump AST to: {}", sFileName);
    }
  }

  void DumpDisassembly(const ezExpressionByteCode& byteCode, ezStringView sOutputName, ezUInt32 uiCounter)
  {
    ezStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    ezStringBuilder sFileName;
    sFileName.Format(":output/Expression/{}_{}_ByteCode.txt", ezArgU(uiCounter, 2, true), sOutputName);

    ezFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      ezLog::Error("Disassembly was dumped to: {}", sFileName);
    }
    else
    {
      ezLog::Error("Failed to dump Disassembly to: {}", sFileName);
    }
  }

  static ezUInt32 s_uiNumByteCodeComparisons = 0;

  bool CompareByteCode(const ezExpressionByteCode& testCode, const ezExpressionByteCode& referenceCode)
  {
    ezUInt32 uiCounter = s_uiNumByteCodeComparisons;
    ++s_uiNumByteCodeComparisons;

    if (testCode != referenceCode)
    {
      DumpDisassembly(referenceCode, "Reference", uiCounter);
      DumpDisassembly(testCode, "Test", uiCounter);
      return false;
    }

    return true;
  }

  static ezHashedString s_sA = ezMakeHashedString("a");
  static ezHashedString s_sB = ezMakeHashedString("b");
  static ezHashedString s_sC = ezMakeHashedString("c");
  static ezHashedString s_sD = ezMakeHashedString("d");
  static ezHashedString s_sOutput = ezMakeHashedString("output");
} // namespace

EZ_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  s_uiNumByteCodeComparisons = 0;

  ezStringBuilder outputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

  ezExpressionParser parser;
  ezExpressionCompiler compiler;
  ezExpressionVM vm;

  auto Compile = [&](ezStringView code, ezExpressionByteCode& out_ByteCode, bool dumpASTs = false)
  {
    ezExpressionParser::Stream inputs[] = {
      ezExpressionParser::Stream(s_sA, ezProcessingStream::DataType::Float),
      ezExpressionParser::Stream(s_sB, ezProcessingStream::DataType::Float),
      ezExpressionParser::Stream(s_sC, ezProcessingStream::DataType::Float),
      ezExpressionParser::Stream(s_sD, ezProcessingStream::DataType::Float),
    };

    ezExpressionParser::Stream outputs[] = {
      ezExpressionParser::Stream(s_sOutput, ezProcessingStream::DataType::Float),
    };

    ezExpressionAST ast;
    EZ_TEST_BOOL(parser.Parse(code, inputs, outputs, {}, ast).Succeeded());

    if (dumpASTs)
    {
      DumpAST(ast, "ParserTest");
    }

    EZ_TEST_BOOL(compiler.Compile(ast, out_ByteCode).Succeeded());

    if (dumpASTs)
    {
      DumpAST(ast, "ParserTest_Opt");
    }
  };

  auto Execute = [&](const ezExpressionByteCode& byteCode, float a = 0.0f, float b = 0.0f, float c = 0.0f, float d = 0.0f)
  {
    ezProcessingStream inputs[] = {
      ezProcessingStream(s_sA, ezMakeArrayPtr(&a, 1).ToByteArray(), ezProcessingStream::DataType::Float),
      ezProcessingStream(s_sB, ezMakeArrayPtr(&b, 1).ToByteArray(), ezProcessingStream::DataType::Float),
      ezProcessingStream(s_sC, ezMakeArrayPtr(&c, 1).ToByteArray(), ezProcessingStream::DataType::Float),
      ezProcessingStream(s_sD, ezMakeArrayPtr(&d, 1).ToByteArray(), ezProcessingStream::DataType::Float),
    };

    float fOutput = ezMath::NaN<float>();
    ezProcessingStream outputs[] = {
      ezProcessingStream(s_sOutput, ezMakeArrayPtr(&fOutput, 1).ToByteArray(), ezProcessingStream::DataType::Float),
    };

    EZ_TEST_BOOL(vm.Execute(byteCode, inputs, outputs, 1).Succeeded());

    return fOutput;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Local variables")
  {
    ezExpressionByteCode referenceByteCode;
    {
      ezStringView code = "output = (a + b) * 2";
      Compile(code, referenceByteCode);
    }

    ezExpressionByteCode testByteCode;

    ezStringView code = "var e = a + b; output = e * 2";
    Compile(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e = e * 2; output = e";
    Compile(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e *= 2; output = e";
    Compile(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; var f = e; e = 2; output = f * e";
    Compile(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    const float a = 2;
    const float b = 3;
    EZ_TEST_FLOAT(Execute(testByteCode, a, b), 10.0f, ezMath::DefaultEpsilon<float>());
  }
}
