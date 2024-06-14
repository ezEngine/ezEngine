#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  static ezUInt32 s_uiNumASTDumps = 0;

  void MakeASTOutputPath(ezStringView sOutputName, ezStringBuilder& out_sOutputPath)
  {
    ezUInt32 uiCounter = s_uiNumASTDumps;
    ++s_uiNumASTDumps;

    out_sOutputPath.SetFormat(":output/Expression/{}_{}_AST.dgml", ezArgU(uiCounter, 2, true), sOutputName);
  }

  void DumpDisassembly(const ezExpressionByteCode& byteCode, ezStringView sOutputName, ezUInt32 uiCounter)
  {
    ezStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    ezStringBuilder sFileName;
    sFileName.SetFormat(":output/Expression/{}_{}_ByteCode.txt", ezArgU(uiCounter, 2, true), sOutputName);

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

  static ezUniquePtr<ezExpressionParser> s_pParser;
  static ezUniquePtr<ezExpressionCompiler> s_pCompiler;
  static ezUniquePtr<ezExpressionVM> s_pVM;

  template <typename T>
  struct StreamDataTypeDeduction
  {
  };

  template <>
  struct StreamDataTypeDeduction<ezFloat16>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Half;
    static ezFloat16 Default() { return ezMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<float>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Float;
    static float Default() { return ezMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<ezInt8>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Byte;
    static ezInt8 Default() { return ezMath::MinValue<ezInt8>(); }
  };

  template <>
  struct StreamDataTypeDeduction<ezInt16>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Short;
    static ezInt16 Default() { return ezMath::MinValue<ezInt16>(); }
  };

  template <>
  struct StreamDataTypeDeduction<int>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Int;
    static int Default() { return ezMath::MinValue<int>(); }
  };

  template <>
  struct StreamDataTypeDeduction<ezVec3>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Float3;
    static ezVec3 Default() { return ezVec3(ezMath::MinValue<float>()); }
  };

  template <>
  struct StreamDataTypeDeduction<ezVec3I32>
  {
    static constexpr ezProcessingStream::DataType Type = ezProcessingStream::DataType::Int3;
    static ezVec3I32 Default() { return ezVec3I32(ezMath::MinValue<int>()); }
  };

  template <typename T>
  void Compile(ezStringView sCode, ezExpressionByteCode& out_byteCode, ezStringView sDumpAstOutputName = ezStringView())
  {
    ezExpression::StreamDesc inputs[] = {
      {s_sA, StreamDataTypeDeduction<T>::Type},
      {s_sB, StreamDataTypeDeduction<T>::Type},
      {s_sC, StreamDataTypeDeduction<T>::Type},
      {s_sD, StreamDataTypeDeduction<T>::Type},
    };

    ezExpression::StreamDesc outputs[] = {
      {s_sOutput, StreamDataTypeDeduction<T>::Type},
    };

    ezExpressionAST ast;
    EZ_TEST_BOOL(s_pParser->Parse(sCode, inputs, outputs, {}, ast).Succeeded());

    ezStringBuilder sOutputPath;
    if (sDumpAstOutputName.IsEmpty() == false)
    {
      MakeASTOutputPath(sDumpAstOutputName, sOutputPath);
    }
    EZ_TEST_BOOL(s_pCompiler->Compile(ast, out_byteCode, sOutputPath).Succeeded());
  }

  template <typename T>
  T Execute(const ezExpressionByteCode& byteCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0))
  {
    ezProcessingStream inputs[] = {
      ezProcessingStream(s_sA, ezMakeArrayPtr(&a, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      ezProcessingStream(s_sB, ezMakeArrayPtr(&b, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      ezProcessingStream(s_sC, ezMakeArrayPtr(&c, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      ezProcessingStream(s_sD, ezMakeArrayPtr(&d, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    T output = StreamDataTypeDeduction<T>::Default();
    ezProcessingStream outputs[] = {
      ezProcessingStream(s_sOutput, ezMakeArrayPtr(&output, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    EZ_TEST_BOOL(s_pVM->Execute(byteCode, inputs, outputs, 1).Succeeded());

    return output;
  };

  template <typename T>
  T TestInstruction(ezStringView sCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0), bool bDumpASTs = false)
  {
    ezExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestInstruction" : "");
    return Execute<T>(byteCode, a, b, c, d);
  }

  template <typename T>
  T TestConstant(ezStringView sCode, bool bDumpASTs = false)
  {
    ezExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestConstant" : "");
    EZ_TEST_INT(byteCode.GetNumInstructions(), 2); // MovX_C, StoreX
    EZ_TEST_INT(byteCode.GetNumTempRegisters(), 1);
    return Execute<T>(byteCode);
  }

  enum TestBinaryFlags
  {
    LeftConstantOptimization = EZ_BIT(0),
    NoInstructionsCountCheck = EZ_BIT(2),
  };

  template <typename R, typename T, ezUInt32 flags>
  void TestBinaryInstruction(ezStringView sOp, T a, T b, T expectedResult, bool bDumpASTs = false)
  {
    constexpr bool boolInputs = std::is_same<T, bool>::value;
    using U = typename std::conditional<boolInputs, int, T>::type;

    U aAsU;
    U bAsU;
    U expectedResultAsU;
    if constexpr (boolInputs)
    {
      aAsU = a ? 1 : 0;
      bAsU = b ? 1 : 0;
      expectedResultAsU = expectedResult ? 1 : 0;
    }
    else
    {
      aAsU = a;
      bAsU = b;
      expectedResultAsU = expectedResult;
    }

    auto TestRes = [](U res, U expectedRes, const char* szCode, const char* szAValue, const char* szBValue)
    {
      if constexpr (std::is_same<T, float>::value)
      {
        EZ_TEST_FLOAT_MSG(res, expectedRes, ezMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, int>::value)
      {
        EZ_TEST_INT_MSG(res, expectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, bool>::value)
      {
        const char* szRes = (res != 0) ? "true" : "false";
        const char* szExpectedRes = (expectedRes != 0) ? "true" : "false";
        EZ_TEST_STRING_MSG(szRes, szExpectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, ezVec3>::value)
      {
        EZ_TEST_VEC3_MSG(res, expectedRes, ezMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, ezVec3I32>::value)
      {
        EZ_TEST_INT_MSG(res.x, expectedRes.x, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        EZ_TEST_INT_MSG(res.y, expectedRes.y, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        EZ_TEST_INT_MSG(res.z, expectedRes.z, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else
      {
        EZ_ASSERT_NOT_IMPLEMENTED;
      }
    };

    const bool functionStyleSyntax = sOp.FindSubString("(");
    const char* formatString = functionStyleSyntax ? "output = {0}{1}, {2})" : "output = {1} {0} {2}";
    const char* aInput = boolInputs ? "(a != 0)" : "a";
    const char* bInput = boolInputs ? "(b != 0)" : "b";

    ezStringBuilder aValue;
    ezStringBuilder bValue;
    if constexpr (std::is_same<T, ezVec3>::value || std::is_same<T, ezVec3I32>::value)
    {
      aValue.SetFormat("vec3({}, {}, {})", a.x, a.y, a.z);
      bValue.SetFormat("vec3({}, {}, {})", b.x, b.y, b.z);
    }
    else
    {
      aValue.SetFormat("{}", a);
      bValue.SetFormat("{}", b);
    }

    int oneConstantInstructions = 3; // LoadX, OpX_RC, StoreX
    int oneConstantRegisters = 1;
    if constexpr (std::is_same<R, bool>::value)
    {
      oneConstantInstructions += 3; // + MovX_C, MovX_C, SelI_RRR
      oneConstantRegisters += 2;    // Two more registers needed for constants above
    }
    if constexpr (boolInputs)
    {
      oneConstantInstructions += 1; // + NotEqI_RC
    }

    int numOutputElements = 1;
    bool hasDifferentOutputElements = false;
    if constexpr (std::is_same<T, ezVec3>::value || std::is_same<T, ezVec3I32>::value)
    {
      numOutputElements = 3;

      for (int i = 1; i < 3; ++i)
      {
        if (expectedResult.GetData()[i] != expectedResult.GetData()[i - 1])
        {
          hasDifferentOutputElements = true;
          break;
        }
      }
    }

    ezStringBuilder code;
    ezExpressionByteCode byteCode;

    code.SetFormat(formatString, sOp, aInput, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryNoConstants" : "");
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aValue, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryLeftConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      int leftConstantInstructions = oneConstantInstructions;
      int leftConstantRegisters = oneConstantRegisters;
      if constexpr ((flags & LeftConstantOptimization) == 0)
      {
        leftConstantInstructions += 1;
        leftConstantRegisters += 1;
      }

      if (byteCode.GetNumInstructions() != leftConstantInstructions || byteCode.GetNumTempRegisters() != leftConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryLeftConstant", 0);
        EZ_TEST_INT(byteCode.GetNumInstructions(), leftConstantInstructions);
        EZ_TEST_INT(byteCode.GetNumTempRegisters(), leftConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aInput, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryRightConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      if (byteCode.GetNumInstructions() != oneConstantInstructions || byteCode.GetNumTempRegisters() != oneConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryRightConstant", 0);
        EZ_TEST_INT(byteCode.GetNumInstructions(), oneConstantInstructions);
        EZ_TEST_INT(byteCode.GetNumTempRegisters(), oneConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aValue, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryConstant" : "");
    if (hasDifferentOutputElements == false)
    {
      int bothConstantsInstructions = 1 + numOutputElements; // MovX_C + StoreX * numOutputElements
      int bothConstantsRegisters = 1;
      if (byteCode.GetNumInstructions() != bothConstantsInstructions || byteCode.GetNumTempRegisters() != bothConstantsRegisters)
      {
        DumpDisassembly(byteCode, "BinaryConstant", 0);
        EZ_TEST_INT(byteCode.GetNumInstructions(), bothConstantsInstructions);
        EZ_TEST_INT(byteCode.GetNumTempRegisters(), bothConstantsRegisters);
      }
    }
    TestRes(Execute<U>(byteCode), expectedResultAsU, code, aValue, bValue);
  }

  template <typename T>
  bool CompareCode(ezStringView sTestCode, ezStringView sReferenceCode, ezExpressionByteCode& out_testByteCode, bool bDumpASTs = false)
  {
    Compile<T>(sTestCode, out_testByteCode, bDumpASTs ? "Test" : "");

    ezExpressionByteCode referenceByteCode;
    Compile<T>(sReferenceCode, referenceByteCode, bDumpASTs ? "Reference" : "");

    return CompareByteCode(out_testByteCode, referenceByteCode);
  }

  template <typename T>
  void TestInputOutput()
  {
    ezStringView testCode = "output = a + b * 2";
    ezExpressionByteCode testByteCode;
    Compile<T>(testCode, testByteCode);

    constexpr ezUInt32 uiCount = 17;
    ezHybridArray<T, uiCount> a;
    ezHybridArray<T, uiCount> b;
    ezHybridArray<T, uiCount> o;
    ezHybridArray<float, uiCount> expectedOutput;
    a.SetCountUninitialized(uiCount);
    b.SetCountUninitialized(uiCount);
    o.SetCount(uiCount);
    expectedOutput.SetCountUninitialized(uiCount);

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      a[i] = static_cast<T>(3.0f * i);
      b[i] = static_cast<T>(1.5f * i);
      expectedOutput[i] = a[i] + b[i] * 2.0f;
    }

    ezProcessingStream inputs[] = {
      ezProcessingStream(s_sA, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      ezProcessingStream(s_sB, b.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      ezProcessingStream(s_sC, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type), // Dummy stream, not actually used
      ezProcessingStream(s_sD, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type), // Dummy stream, not actually used
    };

    ezProcessingStream outputs[] = {
      ezProcessingStream(s_sOutput, o.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
    };

    EZ_TEST_BOOL(s_pVM->Execute(testByteCode, inputs, outputs, uiCount, ezExpression::GlobalData(), ezExpressionVM::Flags::BestPerformance).Succeeded());

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      EZ_TEST_FLOAT(static_cast<float>(o[i]), expectedOutput[i], ezMath::DefaultEpsilon<float>());
    }
  }

  static const ezEnum<ezExpression::RegisterType> s_TestFunc1InputTypes[] = {ezExpression::RegisterType::Float, ezExpression::RegisterType::Int};
  static const ezEnum<ezExpression::RegisterType> s_TestFunc2InputTypes[] = {ezExpression::RegisterType::Float, ezExpression::RegisterType::Float, ezExpression::RegisterType::Int};

  static void TestFunc1(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
  {
    const ezExpression::Register* pX = inputs[0].GetPtr();
    const ezExpression::Register* pY = inputs[1].GetPtr();
    const ezExpression::Register* pXEnd = inputs[0].GetEndPtr();
    ezExpression::Register* pOutput = output.GetPtr();

    while (pX < pXEnd)
    {
      pOutput->f = pX->f.CompMul(pY->i.ToFloat());

      ++pX;
      ++pY;
      ++pOutput;
    }
  }

  static void TestFunc2(ezExpression::Inputs inputs, ezExpression::Output output, const ezExpression::GlobalData& globalData)
  {
    const ezExpression::Register* pX = inputs[0].GetPtr();
    const ezExpression::Register* pY = inputs[1].GetPtr();
    const ezExpression::Register* pXEnd = inputs[0].GetEndPtr();
    ezExpression::Register* pOutput = output.GetPtr();

    if (inputs.GetCount() >= 3)
    {
      const ezExpression::Register* pZ = inputs[2].GetPtr();

      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f + pZ->i.ToFloat();

        ++pX;
        ++pY;
        ++pZ;
        ++pOutput;
      }
    }
    else
    {
      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f;

        ++pX;
        ++pY;
        ++pOutput;
      }
    }
  }

  ezExpressionFunction s_TestFunc1 = {
    {ezMakeHashedString("TestFunc"), ezExpression::FunctionDesc::TypeList(s_TestFunc1InputTypes), 2, ezExpression::RegisterType::Float},
    &TestFunc1,
  };

  ezExpressionFunction s_TestFunc2 = {
    {ezMakeHashedString("TestFunc"), ezExpression::FunctionDesc::TypeList(s_TestFunc2InputTypes), 3, ezExpression::RegisterType::Float},
    &TestFunc2,
  };

} // namespace

EZ_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  s_uiNumByteCodeComparisons = 0;

  ezStringBuilder outputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", ezDataDirUsage::AllowWrites) == EZ_SUCCESS);

  s_pParser = EZ_DEFAULT_NEW(ezExpressionParser);
  s_pCompiler = EZ_DEFAULT_NEW(ezExpressionCompiler);
  s_pVM = EZ_DEFAULT_NEW(ezExpressionVM);
  EZ_SCOPE_EXIT(s_pParser = nullptr; s_pCompiler = nullptr; s_pVM = nullptr;);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unary instructions")
  {
    // Negate
    EZ_TEST_INT(TestInstruction("output = -a", 2), -2);
    EZ_TEST_FLOAT(TestInstruction("output = -a", 2.5f), -2.5f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_INT(TestConstant<int>("output = -2"), -2);
    EZ_TEST_FLOAT(TestConstant<float>("output = -2.5"), -2.5f, ezMath::DefaultEpsilon<float>());

    // Absolute
    EZ_TEST_INT(TestInstruction("output = abs(a)", -2), 2);
    EZ_TEST_FLOAT(TestInstruction("output = abs(a)", -2.5f), 2.5f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_INT(TestConstant<int>("output = abs(-2)"), 2);
    EZ_TEST_FLOAT(TestConstant<float>("output = abs(-2.5)"), 2.5f, ezMath::DefaultEpsilon<float>());

    // Saturate
    EZ_TEST_INT(TestInstruction("output = saturate(a)", -1), 0);
    EZ_TEST_INT(TestInstruction("output = saturate(a)", 2), 1);
    EZ_TEST_FLOAT(TestInstruction("output = saturate(a)", -1.5f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = saturate(a)", 2.5f), 1.0f, ezMath::DefaultEpsilon<float>());

    EZ_TEST_INT(TestConstant<int>("output = saturate(-1)"), 0);
    EZ_TEST_INT(TestConstant<int>("output = saturate(2)"), 1);
    EZ_TEST_FLOAT(TestConstant<float>("output = saturate(-1.5)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = saturate(2.5)"), 1.0f, ezMath::DefaultEpsilon<float>());

    // Sqrt
    EZ_TEST_FLOAT(TestInstruction("output = sqrt(a)", 25.0f), 5.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = sqrt(a)", 2.0f), ezMath::Sqrt(2.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = sqrt(25)"), 5.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = sqrt(2)"), ezMath::Sqrt(2.0f), ezMath::DefaultEpsilon<float>());

    // Exp
    EZ_TEST_FLOAT(TestInstruction("output = exp(a)", 0.0f), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = exp(a)", 2.0f), ezMath::Exp(2.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = exp(0.0)"), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = exp(2.0)"), ezMath::Exp(2.0f), ezMath::DefaultEpsilon<float>());

    // Ln
    EZ_TEST_FLOAT(TestInstruction("output = ln(a)", 1.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = ln(a)", 2.0f), ezMath::Ln(2.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = ln(1.0)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = ln(2.0)"), ezMath::Ln(2.0f), ezMath::DefaultEpsilon<float>());

    // Log2
    EZ_TEST_INT(TestInstruction("output = log2(a)", 1), 0);
    EZ_TEST_INT(TestInstruction("output = log2(a)", 8), 3);
    EZ_TEST_FLOAT(TestInstruction("output = log2(a)", 1.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = log2(a)", 4.0f), 2.0f, ezMath::DefaultEpsilon<float>());

    EZ_TEST_INT(TestConstant<int>("output = log2(1)"), 0);
    EZ_TEST_INT(TestConstant<int>("output = log2(16)"), 4);
    EZ_TEST_FLOAT(TestConstant<float>("output = log2(1.0)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = log2(32.0)"), 5.0f, ezMath::DefaultEpsilon<float>());

    // Log10
    EZ_TEST_FLOAT(TestInstruction("output = log10(a)", 10.0f), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = log10(a)", 1000.0f), 3.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = log10(10.0)"), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = log10(100.0)"), 2.0f, ezMath::DefaultEpsilon<float>());

    // Pow2
    EZ_TEST_INT(TestInstruction("output = pow2(a)", 0), 1);
    EZ_TEST_INT(TestInstruction("output = pow2(a)", 3), 8);
    EZ_TEST_FLOAT(TestInstruction("output = pow2(a)", 4.0f), 16.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = pow2(a)", 6.0f), 64.0f, ezMath::DefaultEpsilon<float>());

    EZ_TEST_INT(TestConstant<int>("output = pow2(0)"), 1);
    EZ_TEST_INT(TestConstant<int>("output = pow2(3)"), 8);
    EZ_TEST_FLOAT(TestConstant<float>("output = pow2(3.0)"), 8.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = pow2(5.0)"), 32.0f, ezMath::DefaultEpsilon<float>());

    // Sin
    EZ_TEST_FLOAT(TestInstruction("output = sin(a)", ezAngle::MakeFromDegree(90.0f).GetRadian()), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = sin(a)", ezAngle::MakeFromDegree(45.0f).GetRadian()), ezMath::Sin(ezAngle::MakeFromDegree(45.0f)), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = sin(PI / 2)"), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = sin(PI / 4)"), ezMath::Sin(ezAngle::MakeFromDegree(45.0f)), ezMath::DefaultEpsilon<float>());

    // Cos
    EZ_TEST_FLOAT(TestInstruction("output = cos(a)", 0.0f), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = cos(a)", ezAngle::MakeFromDegree(45.0f).GetRadian()), ezMath::Cos(ezAngle::MakeFromDegree(45.0f)), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = cos(0)"), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = cos(PI / 4)"), ezMath::Cos(ezAngle::MakeFromDegree(45.0f)), ezMath::DefaultEpsilon<float>());

    // Tan
    EZ_TEST_FLOAT(TestInstruction("output = tan(a)", 0.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = tan(a)", ezAngle::MakeFromDegree(45.0f).GetRadian()), 1.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = tan(0)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = tan(PI / 4)"), 1.0f, ezMath::DefaultEpsilon<float>());

    // ASin
    EZ_TEST_FLOAT(TestInstruction("output = asin(a)", 1.0f), ezAngle::MakeFromDegree(90.0f).GetRadian(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = asin(a)", ezMath::Sin(ezAngle::MakeFromDegree(45.0f))), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::LargeEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = asin(1)"), ezAngle::MakeFromDegree(90.0f).GetRadian(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = asin(sin(PI / 4))"), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::LargeEpsilon<float>());

    // ACos
    EZ_TEST_FLOAT(TestInstruction("output = acos(a)", 1.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = acos(a)", ezMath::Cos(ezAngle::MakeFromDegree(45.0f))), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::LargeEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = acos(1)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = acos(cos(PI / 4))"), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::LargeEpsilon<float>());

    // ATan
    EZ_TEST_FLOAT(TestInstruction("output = atan(a)", 0.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = atan(a)", 1.0f), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = atan(0)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = atan(1)"), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::DefaultEpsilon<float>());

    // RadToDeg
    EZ_TEST_FLOAT(TestInstruction("output = radToDeg(a)", ezAngle::MakeFromDegree(135.0f).GetRadian()), 135.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = rad_to_deg(a)", ezAngle::MakeFromDegree(180.0f).GetRadian()), 180.0f, ezMath::LargeEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = radToDeg(PI / 2)"), 90.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = rad_to_deg(PI/4)"), 45.0f, ezMath::DefaultEpsilon<float>());

    // DegToRad
    EZ_TEST_FLOAT(TestInstruction("output = degToRad(a)", 135.0f), ezAngle::MakeFromDegree(135.0f).GetRadian(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = deg_to_rad(a)", 180.0f), ezAngle::MakeFromDegree(180.0f).GetRadian(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = degToRad(90.0)"), ezAngle::MakeFromDegree(90.0f).GetRadian(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = deg_to_rad(45)"), ezAngle::MakeFromDegree(45.0f).GetRadian(), ezMath::DefaultEpsilon<float>());

    // Round
    EZ_TEST_FLOAT(TestInstruction("output = round(a)", 12.34f), 12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = round(a)", -12.34f), -12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = round(a)", 12.54f), 13, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = round(a)", -12.54f), -13, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = round(4.3)"), 4, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = round(4.51)"), 5, ezMath::DefaultEpsilon<float>());

    // Floor
    EZ_TEST_FLOAT(TestInstruction("output = floor(a)", 12.34f), 12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = floor(a)", -12.34f), -13, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = floor(a)", 12.54f), 12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = floor(a)", -12.54f), -13, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = floor(4.3)"), 4, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = floor(4.51)"), 4, ezMath::DefaultEpsilon<float>());

    // Ceil
    EZ_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.34f), 13, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.34f), -12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.54f), 13, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.54f), -12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = ceil(4.3)"), 5, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = ceil(4.51)"), 5, ezMath::DefaultEpsilon<float>());

    // Trunc
    EZ_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.34f), 12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.34f), -12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.54f), 12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.54f), -12, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = trunc(4.3)"), 4, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = trunc(4.51)"), 4, ezMath::DefaultEpsilon<float>());

    // Frac
    EZ_TEST_FLOAT(TestInstruction("output = frac(a)", 12.34f), 0.34f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = frac(a)", -12.34f), -0.34f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = frac(a)", 12.54f), 0.54f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = frac(a)", -12.54f), -0.54f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = frac(4.3)"), 0.3f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = frac(4.51)"), 0.51f, ezMath::DefaultEpsilon<float>());

    // Length
    EZ_TEST_VEC3(TestInstruction<ezVec3>("output = length(a)", ezVec3(0, 4, 3)), ezVec3(5), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(TestInstruction<ezVec3>("output = length(a)", ezVec3(-3, 4, 0)), ezVec3(5), ezMath::DefaultEpsilon<float>());

    // Normalize
    EZ_TEST_VEC3(TestInstruction<ezVec3>("output = normalize(a)", ezVec3(1, 4, 3)), ezVec3(1, 4, 3).GetNormalized(), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(TestInstruction<ezVec3>("output = normalize(a)", ezVec3(-3, 7, 22)), ezVec3(-3, 7, 22).GetNormalized(), ezMath::DefaultEpsilon<float>());

    // Length and normalize optimization
    {
      ezStringView testCode = "var x = length(a); var na = normalize(a); output = b * x + na";
      ezStringView referenceCode = "var x = length(a); var na = a / x; output = b * x + na";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<ezVec3>(testCode, referenceCode, testByteCode));

      ezVec3 a = ezVec3(0, 4, 3);
      ezVec3 b = ezVec3(1, 0, 0);
      ezVec3 res = b * a.GetLength() + a.GetNormalized();
      EZ_TEST_VEC3(Execute(testByteCode, a, b), res, ezMath::DefaultEpsilon<float>());
    }

    // BitwiseNot
    EZ_TEST_INT(TestInstruction("output = ~a", 1), ~1);
    EZ_TEST_INT(TestInstruction("output = ~a", 8), ~8);
    EZ_TEST_INT(TestConstant<int>("output = ~1"), ~1);
    EZ_TEST_INT(TestConstant<int>("output = ~17"), ~17);

    // LogicalNot
    EZ_TEST_INT(TestInstruction("output = !(a == 1)", 1), 0);
    EZ_TEST_INT(TestInstruction("output = !(a == 1)", 8), 1);
    EZ_TEST_INT(TestConstant<int>("output = !(1 == 1)"), 0);
    EZ_TEST_INT(TestConstant<int>("output = !(8 == 1)"), 1);

    // All
    EZ_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", ezVec3(1, 2, 3), ezVec3(1, 2, 3)), ezVec3(1), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", ezVec3(1, 2, 3), ezVec3(1, 2, 4)), ezVec3(0), ezMath::DefaultEpsilon<float>());

    // Any
    EZ_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", ezVec3(1, 2, 3), ezVec3(4, 5, 3)), ezVec3(1), ezMath::DefaultEpsilon<float>());
    EZ_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", ezVec3(1, 2, 3), ezVec3(4, 5, 6)), ezVec3(0), ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Binary instructions")
  {
    // Add
    TestBinaryInstruction<int, int, LeftConstantOptimization>("+", 3, 5, 8);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("+", 3.5f, 5.3f, 8.8f);

    // Subtract
    TestBinaryInstruction<int, int, 0>("-", 9, 5, 4);
    TestBinaryInstruction<float, float, 0>("-", 9.5f, 5.3f, 4.2f);

    // Multiply
    TestBinaryInstruction<int, int, LeftConstantOptimization>("*", 3, 5, 15);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("*", 3.5f, 5.3f, 18.55f);

    // Divide
    TestBinaryInstruction<int, int, 0>("/", 11, 5, 2);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("/", -11, 4, -2); // divide by power of 2 optimization
    TestBinaryInstruction<int, int, 0>("/", 11, -4, -2);                        // divide by power of 2 optimization only works for positive divisors
    TestBinaryInstruction<float, float, 0>("/", 12.6f, 3.0f, 4.2f);

    // Modulo
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", 13, 5, 3);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", -13, 5, -3);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", 13, 4, 1);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", -13, 4, -1);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("%", 13.5, 5.0, 3.5);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("mod(", -13.5, 5.0, -3.5);

    // Log
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 2.0f, 1024.0f, 10.0f);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 7.1f, 81.62f, ezMath::Log(7.1f, 81.62f));

    // Pow
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("pow(", 2, 5, 32);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("pow(", 3, 3, 27);

    // Pow is replaced by multiplication for constant exponents up until 16.
    // Test all of them to ensure the multiplication tables are correct.
    for (int i = 0; i <= 16; ++i)
    {
      ezStringBuilder testCode;
      testCode.SetFormat("output = pow(a, {})", i);

      ezExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      EZ_TEST_INT(Execute(testByteCode, 3), ezMath::Pow(3, i));
    }

    {
      ezStringView testCode = "output = pow(a, 7)";
      ezStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; output = a6 * a";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      EZ_TEST_INT(Execute(testByteCode, 3), 2187);
    }

    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 2.0, 5.0, 32.0);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 3.0f, 7.9f, ezMath::Pow(3.0f, 7.9f));

    {
      ezStringView testCode = "output = pow(a, 15.0)";
      ezStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; var a12 = a6 * a6; output = a12 * a3";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      EZ_TEST_FLOAT(Execute(testByteCode, 2.1f), ezMath::Pow(2.1f, 15.0f), ezMath::DefaultEpsilon<float>());
    }

    // Min
    TestBinaryInstruction<int, int, LeftConstantOptimization>("min(", 11, 5, 5);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("min(", 12.6f, 3.0f, 3.0f);

    // Max
    TestBinaryInstruction<int, int, LeftConstantOptimization>("max(", 11, 5, 11);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("max(", 12.6f, 3.0f, 12.6f);

    // Dot
    TestBinaryInstruction<ezVec3, ezVec3, NoInstructionsCountCheck>("dot(", ezVec3(1, -2, 3), ezVec3(-5, -6, 7), ezVec3(28));
    TestBinaryInstruction<ezVec3I32, ezVec3I32, NoInstructionsCountCheck>("dot(", ezVec3I32(1, -2, 3), ezVec3I32(-5, -6, 7), ezVec3I32(28));

    // Cross
    TestBinaryInstruction<ezVec3, ezVec3, NoInstructionsCountCheck>("cross(", ezVec3(1, 0, 0), ezVec3(0, 1, 0), ezVec3(0, 0, 1));
    TestBinaryInstruction<ezVec3, ezVec3, NoInstructionsCountCheck>("cross(", ezVec3(0, 1, 0), ezVec3(0, 0, 1), ezVec3(1, 0, 0));
    TestBinaryInstruction<ezVec3, ezVec3, NoInstructionsCountCheck>("cross(", ezVec3(0, 0, 1), ezVec3(1, 0, 0), ezVec3(0, 1, 0));

    // Reflect
    TestBinaryInstruction<ezVec3, ezVec3, NoInstructionsCountCheck>("reflect(", ezVec3(1, 2, -1), ezVec3(0, 0, 1), ezVec3(1, 2, 1));

    // BitshiftLeft
    TestBinaryInstruction<int, int, 0>("<<", 11, 5, 11 << 5);

    // BitshiftRight
    TestBinaryInstruction<int, int, 0>(">>", 0xABCD, 8, 0xAB);

    // BitwiseAnd
    TestBinaryInstruction<int, int, LeftConstantOptimization>("&", 0xFFCD, 0xABFF, 0xABCD);

    // BitwiseXor
    TestBinaryInstruction<int, int, LeftConstantOptimization>("^", 0xFFCD, 0xABFF, 0xFFCD ^ 0xABFF);

    // BitwiseOr
    TestBinaryInstruction<int, int, LeftConstantOptimization>("|", 0x00CD, 0xAB00, 0xABCD);

    // Equal
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("==", 11, 5, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("==", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("==", true, false, false);

    // NotEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("!=", 11, 5, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("!=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("!=", true, false, true);

    // Less
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<", 11, 5, 0);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 12.6f, 0.0f);

    // LessEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<=", 11, 5, 0);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 12.6f, 1.0f);

    // Greater
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">", 11, 5, 1);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 12.6f, 0.0f);

    // GreaterEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">=", 11, 5, 1);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 12.6f, 1.0f);

    // LogicalAnd
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("&&", true, false, false);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("&&", true, true, true);

    // LogicalOr
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("||", true, false, true);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("||", false, false, false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Ternary instructions")
  {
    // Clamp
    EZ_TEST_INT(TestInstruction("output = clamp(a, b, c)", -1, 0, 10), 0);
    EZ_TEST_INT(TestInstruction("output = clamp(a, b, c)", 2, 0, 10), 2);
    EZ_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", -1.5f, 0.0f, 1.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", 2.5f, 0.0f, 1.0f), 1.0f, ezMath::DefaultEpsilon<float>());

    EZ_TEST_INT(TestConstant<int>("output = clamp(-1, 0, 10)"), 0);
    EZ_TEST_INT(TestConstant<int>("output = clamp(2, 0, 10)"), 2);
    EZ_TEST_FLOAT(TestConstant<float>("output = clamp(-1.5, 0, 2)"), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = clamp(2.5, 0, 2)"), 2.0f, ezMath::DefaultEpsilon<float>());

    // Select
    EZ_TEST_INT(TestInstruction("output = (a == 1) ? b : c", 1, 2, 3), 2);
    EZ_TEST_INT(TestInstruction("output = a != 1 ? b : c", 1, 2, 3), 3);
    EZ_TEST_FLOAT(TestInstruction("output = (a == 1) ? b : c", 1.0f, 2.4f, 3.5f), 2.4f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = a != 1 ? b : c", 1.0f, 2.4f, 3.5f), 3.5f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_INT(TestInstruction("output = (a == 1) ? (b > 2) : (c > 2)", 1, 2, 3), 0);
    EZ_TEST_INT(TestInstruction("output = a != 1 ? b > 2 : c > 2", 1, 2, 3), 1);

    EZ_TEST_INT(TestConstant<int>("output = (1 == 1) ? 2 : 3"), 2);
    EZ_TEST_INT(TestConstant<int>("output = 1 != 1 ? 2 : 3"), 3);
    EZ_TEST_FLOAT(TestConstant<float>("output = (1.0 == 1.0) ? 2.4 : 3.5"), 2.4f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = 1.0 != 1.0 ? 2.4 : 3.5"), 3.5f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_INT(TestConstant<int>("output = (1 == 1) ? false : true"), 0);
    EZ_TEST_INT(TestConstant<int>("output = 1 != 1 ? false : true"), 1);

    // Lerp
    EZ_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", 1.0f, 5.0f, 0.75f), 4.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", -1.0f, -11.0f, 0.1f), -2.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = lerp(1, 5, 0.75)"), 4.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = lerp(-1, -11, 0.1)"), -2.0f, ezMath::DefaultEpsilon<float>());

    // SmoothStep
    EZ_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.0f, 0.0f, 1.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.2f, 0.0f, 1.0f), ezMath::SmoothStep(0.2f, 0.0f, 1.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.5f, 0.0f, 1.0f), 0.5f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.2f, 0.2f, 0.8f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.4f, 0.2f, 0.8f), ezMath::SmoothStep(0.4f, 0.2f, 0.8f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = smoothstep(0.2, 0, 1)"), ezMath::SmoothStep(0.2f, 0.0f, 1.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = smoothstep(0.4, 0.2, 0.8)"), ezMath::SmoothStep(0.4f, 0.2f, 0.8f), ezMath::DefaultEpsilon<float>());

    // SmootherStep
    EZ_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.0f, 0.0f, 1.0f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.2f, 0.0f, 1.0f), ezMath::SmootherStep(0.2f, 0.0f, 1.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.5f, 0.0f, 1.0f), 0.5f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.2f, 0.2f, 0.8f), 0.0f, ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.4f, 0.2f, 0.8f), ezMath::SmootherStep(0.4f, 0.2f, 0.8f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = smootherstep(0.2, 0, 1)"), ezMath::SmootherStep(0.2f, 0.0f, 1.0f), ezMath::DefaultEpsilon<float>());
    EZ_TEST_FLOAT(TestConstant<float>("output = smootherstep(0.4, 0.2, 0.8)"), ezMath::SmootherStep(0.4f, 0.2f, 0.8f), ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Local variables")
  {
    ezExpressionByteCode referenceByteCode;
    {
      ezStringView code = "output = (a + b) * 2";
      Compile<float>(code, referenceByteCode);
    }

    ezExpressionByteCode testByteCode;

    ezStringView code = "var e = a + b; output = e * 2";
    Compile<float>(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e = e * 2; output = e";
    Compile<float>(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e *= 2; output = e";
    Compile<float>(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; var f = e; e = 2; output = f * e";
    Compile<float>(code, testByteCode);
    EZ_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    EZ_TEST_FLOAT(Execute(testByteCode, 2.0f, 3.0f), 10.0f, ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Assignment")
  {
    {
      ezStringView testCode = "output = 40; output += 2";
      ezStringView referenceCode = "output = 42";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      EZ_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, ezMath::DefaultEpsilon<float>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Integer arithmetic")
  {
    ezExpressionByteCode testByteCode;

    ezStringView code = "output = ((a & 0xFF) << 8) | (b & 0xFFFF >> 8)";
    Compile<int>(code, testByteCode);

    const int a = 0xABABABAB;
    const int b = 0xCDCDCDCD;
    EZ_TEST_INT(Execute(testByteCode, a, b), 0xABCD);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constant folding")
  {
    ezStringView testCode = "var x = abs(-7) + saturate(2) + 2\n"
                            "var v = (sqrt(25) - 4) * 5\n"
                            "var m = min(300, 1000) / max(1, 3);"
                            "var r = m - x * 5 - v - clamp(13, 1, 3);\n"
                            "output = r";

    ezStringView referenceCode = "output = 42";

    {
      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      EZ_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, ezMath::DefaultEpsilon<float>());
    }

    {
      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));

      EZ_TEST_INT(Execute<int>(testByteCode), 42);
    }

    testCode = "";
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constant instructions")
  {
    // There are special instructions in the vm which take the constant as the first operand in place and
    // don't require an extra mov for the constant.
    // This test checks whether the compiler transforms operations with constants as second operands to the preferred form.

    ezStringView testCode = "output = (2 + a) + (-1 + b) + (2 * c) + (d / 5) + min(1, c) + max(2, d)";

    {
      ezStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d * 0.2) + min(c, 1) + max(d, 2)";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      EZ_TEST_INT(testByteCode.GetNumInstructions(), 16);
      EZ_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      EZ_TEST_FLOAT(Execute(testByteCode, 1.0f, 2.0f, 3.0f, 40.f), 59.0f, ezMath::DefaultEpsilon<float>());
    }

    {
      ezStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d / 5) + min(c, 1) + max(d, 2)";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      EZ_TEST_INT(testByteCode.GetNumInstructions(), 16);
      EZ_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      EZ_TEST_INT(Execute(testByteCode, 1, 2, 3, 40), 59);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Integer and float conversions")
  {
    ezStringView testCode = "var x = 7; var y = 0.6\n"
                            "var e = a * x * b * y\n"
                            "int i = c * 2; i *= i; e += i\n"
                            "output = e";

    ezStringView referenceCode = "int i = (int(c) * 2); output = int((float(a * 7 * b) * 0.6) + float(i * i))";

    ezExpressionByteCode testByteCode;
    EZ_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
    EZ_TEST_INT(Execute(testByteCode, 1, 2, 3), 44);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Bool conversions")
  {
    ezStringView testCode = "var x = true\n"
                            "bool y = a\n"
                            "output = x == y";

    {
      ezStringView referenceCode = "bool r = true == (a != 0); output = r ? 1 : 0";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      EZ_TEST_INT(Execute(testByteCode, 14), 1);
    }

    {
      ezStringView referenceCode = "bool r = true == (a != 0); output = r ? 1.0 : 0.0";

      ezExpressionByteCode testByteCode;
      EZ_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      EZ_TEST_FLOAT(Execute(testByteCode, 15.0f), 1.0f, ezMath::DefaultEpsilon<float>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Load Inputs/Store Outputs")
  {
    TestInputOutput<float>();
    TestInputOutput<ezFloat16>();

    TestInputOutput<int>();
    TestInputOutput<ezInt16>();
    TestInputOutput<ezInt8>();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Function overloads")
  {
    s_pParser->RegisterFunction(s_TestFunc1.m_Desc);
    s_pParser->RegisterFunction(s_TestFunc2.m_Desc);

    s_pVM->RegisterFunction(s_TestFunc1);
    s_pVM->RegisterFunction(s_TestFunc2);

    {
      // take TestFunc1 overload for all ints
      ezStringView testCode = "output = TestFunc(1, 2, 3)";
      ezExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      EZ_TEST_INT(Execute<int>(testByteCode), 2);
    }

    {
      // take TestFunc1 overload for float, int
      ezStringView testCode = "output = TestFunc(1.0, 2, 3)";
      ezExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      EZ_TEST_INT(Execute<int>(testByteCode), 2);
    }

    {
      // take TestFunc2 overload for int, float
      ezStringView testCode = "output = TestFunc(1, 2.0, 3)";
      ezExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      EZ_TEST_INT(Execute<int>(testByteCode), 7);
    }

    {
      // take TestFunc2 overload for all float
      ezStringView testCode = "output = TestFunc(1.0, 2.0, 3)";
      ezExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      EZ_TEST_INT(Execute<int>(testByteCode), 7);
    }

    {
      // take TestFunc1 overload when only two params are given
      ezStringView testCode = "output = TestFunc(1.0, 2.0)";
      ezExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      EZ_TEST_INT(Execute<int>(testByteCode), 2);
    }

    s_pParser->UnregisterFunction(s_TestFunc1.m_Desc);
    s_pParser->UnregisterFunction(s_TestFunc2.m_Desc);

    s_pVM->UnregisterFunction(s_TestFunc1);
    s_pVM->UnregisterFunction(s_TestFunc2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Common subexpression elimination")
  {
    ezStringView testCode = "var x1 = a * max(b, c)\n"
                            "var x2 = max(c, b) * a\n"
                            "var y1 = a * pow(2, 3)\n"
                            "var y2 = 8 * a\n"
                            "output = x1 + x2 + y1 + y2";

    ezStringView referenceCode = "var x = a * max(b, c); var y = a * 8; output = x + x + y + y";

    ezExpressionByteCode testByteCode;
    EZ_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
    EZ_TEST_INT(Execute(testByteCode, 2, 4, 8), 64);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Vector constructors")
  {
    {
      ezStringView testCode = "var x = vec3(1, 2, 3)\n"
                              "var y = vec4(x, 4)\n"
                              "vec3 z = vec2(1, 2)\n"
                              "var w = vec4()\n"
                              "output = vec4(x) + y + vec4(z) + w";

      ezExpressionByteCode testByteCode;
      Compile<ezVec3>(testCode, testByteCode);
      EZ_TEST_VEC3(Execute<ezVec3>(testByteCode), ezVec3(3, 6, 6), ezMath::DefaultEpsilon<float>());
    }

    {
      ezStringView testCode = "var x = vec4(a.xy, (vec2(6, 8) - vec2(3, 4)).xy)\n"
                              "var y = vec4(1, vec2(2, 3), 4)\n"
                              "var z = vec4(1, vec3(2, 3, 4))\n"
                              "var w = vec4(1, 2, a.zw)\n"
                              "var one = vec4(1)\n"
                              "output = vec4(x) + y + vec4(z) + w + one";

      ezExpressionByteCode testByteCode;
      Compile<ezVec3>(testCode, testByteCode);
      EZ_TEST_VEC3(Execute(testByteCode, ezVec3(1, 2, 3)), ezVec3(5, 9, 13), ezMath::DefaultEpsilon<float>());
    }

    {
      ezStringView testCode = "var x = vec4(1, 2, 3, 4)\n"
                              "var y = x.z\n"
                              "x.yz = 7\n"
                              "x.xz = vec2(2, 7)\n"
                              "output = x * y";

      ezExpressionByteCode testByteCode;
      Compile<ezVec3>(testCode, testByteCode);
      EZ_TEST_VEC3(Execute<ezVec3>(testByteCode), ezVec3(6, 21, 21), ezMath::DefaultEpsilon<float>());
    }

    {
      ezStringView testCode = "var x = 1\n"
                              "x.z = 7.5\n"
                              "output = x";

      ezExpressionByteCode testByteCode;
      Compile<ezVec3>(testCode, testByteCode);
      EZ_TEST_VEC3(Execute<ezVec3>(testByteCode), ezVec3(1, 0, 7), ezMath::DefaultEpsilon<float>());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Vector instructions")
  {
    // The VM does only support scalar data types.
    // This test checks whether the compiler transforms everything correctly to scalar operation.

    ezStringView testCode = "output = a * vec3(1, 2, 3) + sqrt(b)";

    ezStringView referenceCode = "output.x = a.x + sqrt(b.x)\n"
                                 "output.y = a.y * 2 + sqrt(b.y)\n"
                                 "output.z = a.z * 3 + sqrt(b.z)";

    ezExpressionByteCode testByteCode;
    EZ_TEST_BOOL(CompareCode<ezVec3>(testCode, referenceCode, testByteCode));
    EZ_TEST_VEC3(Execute(testByteCode, ezVec3(1, 3, 5), ezVec3(4, 9, 16)), ezVec3(3, 9, 19), ezMath::DefaultEpsilon<float>());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Vector swizzle")
  {
    ezStringView testCode = "var n = vec4(1, 2, 3, 4)\n"
                            "var m = vec4(5, 6, 7, 8)\n"
                            "var p = n.xxyy + m.zzww * m.abgr + n.w\n"
                            "output = p";

    // vec3(1, 1, 2) + vec3(7, 7, 8) * vec3(8, 7, 6) + 4
    // output.x = 1 + 7 * 8 + 4 = 61
    // output.y = 1 + 7 * 7 + 4 = 54
    // output.z = 2 + 8 * 6 + 4 = 54

    ezExpressionByteCode testByteCode;
    Compile<ezVec3>(testCode, testByteCode);
    EZ_TEST_VEC3(Execute<ezVec3>(testByteCode), ezVec3(61, 54, 54), ezMath::DefaultEpsilon<float>());
  }
}
