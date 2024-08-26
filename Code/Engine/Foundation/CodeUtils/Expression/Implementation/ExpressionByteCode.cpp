#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

namespace
{
  static constexpr const char* s_szOpCodeNames[] = {
    "Nop",

    "",

    "AbsF_R",
    "AbsI_R",
    "SqrtF_R",

    "ExpF_R",
    "LnF_R",
    "Log2F_R",
    "Log2I_R",
    "Log10F_R",
    "Pow2F_R",

    "SinF_R",
    "CosF_R",
    "TanF_R",

    "ASinF_R",
    "ACosF_R",
    "ATanF_R",

    "RoundF_R",
    "FloorF_R",
    "CeilF_R",
    "TruncF_R",

    "NotB_R",
    "NotI_R",

    "IToF_R",
    "FToI_R",

    "",
    "",

    "AddF_RR",
    "AddI_RR",

    "SubF_RR",
    "SubI_RR",

    "MulF_RR",
    "MulI_RR",

    "DivF_RR",
    "DivI_RR",

    "MinF_RR",
    "MinI_RR",

    "MaxF_RR",
    "MaxI_RR",

    "ShlI_RR",
    "ShrI_RR",
    "AndI_RR",
    "XorI_RR",
    "OrI_RR",

    "EqF_RR",
    "EqI_RR",
    "EqB_RR",

    "NEqF_RR",
    "NEqI_RR",
    "NEqB_RR",

    "LtF_RR",
    "LtI_RR",

    "LEqF_RR",
    "LEqI_RR",

    "GtF_RR",
    "GtI_RR",

    "GEqF_RR",
    "GEqI_RR",

    "AndB_RR",
    "OrB_RR",

    "",
    "",

    "AddF_RC",
    "AddI_RC",

    "SubF_RC",
    "SubI_RC",

    "MulF_RC",
    "MulI_RC",

    "DivF_RC",
    "DivI_RC",

    "MinF_RC",
    "MinI_RC",

    "MaxF_RC",
    "MaxI_RC",

    "ShlI_RC",
    "ShrI_RC",
    "AndI_RC",
    "XorI_RC",
    "OrI_RC",

    "EqF_RC",
    "EqI_RC",
    "EqB_RC",

    "NEqF_RC",
    "NEqI_RC",
    "NEqB_RC",

    "LtF_RC",
    "LtI_RC",

    "LEqF_RC",
    "LEqI_RC",

    "GtF_RC",
    "GtI_RC",

    "GEqF_RC",
    "GEqI_RC",

    "AndB_RC",
    "OrB_RC",

    "",
    "",

    "SelF_RRR",
    "SelI_RRR",
    "SelB_RRR",

    "",
    "",

    "MovX_R",
    "MovX_C",
    "LoadF",
    "LoadI",
    "StoreF",
    "StoreI",

    "Call",

    "",
  };

  static_assert(EZ_ARRAY_SIZE(s_szOpCodeNames) == ezExpressionByteCode::OpCode::Count);
  static_assert(ezExpressionByteCode::OpCode::LastBinary - ezExpressionByteCode::OpCode::FirstBinary == ezExpressionByteCode::OpCode::LastBinaryWithConstant - ezExpressionByteCode::OpCode::FirstBinaryWithConstant);


  static constexpr ezUInt32 GetMaxOpCodeLength()
  {
    ezUInt32 uiMaxLength = 0;
    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_szOpCodeNames); ++i)
    {
      uiMaxLength = ezMath::Max(uiMaxLength, ezStringUtils::GetStringElementCount(s_szOpCodeNames[i]));
    }
    return uiMaxLength;
  }

  static constexpr ezUInt32 s_uiMaxOpCodeLength = GetMaxOpCodeLength();

} // namespace

const char* ezExpressionByteCode::OpCode::GetName(Enum code)
{
  EZ_ASSERT_DEBUG(code >= 0 && code < EZ_ARRAY_SIZE(s_szOpCodeNames), "Out of bounds access");
  return s_szOpCodeNames[code];
}

//////////////////////////////////////////////////////////////////////////

//clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezExpressionByteCode, ezNoBase, 1, ezRTTINoAllocator)
EZ_END_STATIC_REFLECTED_TYPE;
//clang-format on

ezExpressionByteCode::ezExpressionByteCode() = default;

ezExpressionByteCode::ezExpressionByteCode(const ezExpressionByteCode& other)
{
  *this = other;
}

ezExpressionByteCode::~ezExpressionByteCode()
{
  Clear();
}

void ezExpressionByteCode::operator=(const ezExpressionByteCode& other)
{
  Clear();
  Init(other.GetByteCode(), other.GetInputs(), other.GetOutputs(), other.GetFunctions(), other.GetNumTempRegisters(), other.GetNumInstructions());
}

bool ezExpressionByteCode::operator==(const ezExpressionByteCode& other) const
{
  return GetByteCode() == other.GetByteCode() &&
         GetInputs() == other.GetInputs() &&
         GetOutputs() == other.GetOutputs() &&
         GetFunctions() == other.GetFunctions();
}

void ezExpressionByteCode::Clear()
{
  ezMemoryUtils::Destruct(m_pInputs, m_uiNumInputs);
  ezMemoryUtils::Destruct(m_pOutputs, m_uiNumOutputs);
  ezMemoryUtils::Destruct(m_pFunctions, m_uiNumFunctions);

  m_pInputs = nullptr;
  m_pOutputs = nullptr;
  m_pFunctions = nullptr;
  m_pByteCode = nullptr;

  m_uiByteCodeCount = 0;
  m_uiNumInputs = 0;
  m_uiNumOutputs = 0;
  m_uiNumFunctions = 0;

  m_uiNumTempRegisters = 0;
  m_uiNumInstructions = 0;

  m_Data.Clear();
}

void ezExpressionByteCode::Disassemble(ezStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.Append("// Inputs:\n");
  for (ezUInt32 i = 0; i < m_uiNumInputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pInputs[i].m_sName, ezProcessingStream::GetDataTypeName(m_pInputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Outputs:\n");
  for (ezUInt32 i = 0; i < m_uiNumOutputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pOutputs[i].m_sName, ezProcessingStream::GetDataTypeName(m_pOutputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Functions:\n");
  for (ezUInt32 i = 0; i < m_uiNumFunctions; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {} {}(", i, ezExpression::RegisterType::GetName(m_pFunctions[i].m_OutputType), m_pFunctions[i].m_sName);
    const ezUInt32 uiNumArguments = m_pFunctions[i].m_InputTypes.GetCount();
    for (ezUInt32 j = 0; j < uiNumArguments; ++j)
    {
      out_sDisassembly.Append(ezExpression::RegisterType::GetName(m_pFunctions[i].m_InputTypes[j]));
      if (j < uiNumArguments - 1)
      {
        out_sDisassembly.Append(", ");
      }
    }
    out_sDisassembly.Append(")\n");
  }

  out_sDisassembly.AppendFormat("\n// Temp Registers: {}\n", GetNumTempRegisters());
  out_sDisassembly.AppendFormat("// Instructions: {}\n\n", GetNumInstructions());

  auto AppendConstant = [](ezUInt32 x, ezStringBuilder& out_sString)
  {
    out_sString.AppendFormat("0x{}({})", ezArgU(x, 8, true, 16), ezArgF(*reinterpret_cast<float*>(&x), 6));
  };

  const StorageType* pByteCode = GetByteCodeStart();
  const StorageType* pByteCodeEnd = GetByteCodeEnd();

  while (pByteCode < pByteCodeEnd)
  {
    OpCode::Enum opCode = GetOpCode(pByteCode);
    {
      const char* szOpCode = OpCode::GetName(opCode);
      ezUInt32 uiOpCodeLength = ezStringUtils::GetStringElementCount(szOpCode);

      out_sDisassembly.Append(szOpCode);
      for (ezUInt32 i = uiOpCodeLength; i < s_uiMaxOpCodeLength + 1; ++i)
      {
        out_sDisassembly.Append(" ");
      }
    }

    if (opCode > OpCode::FirstUnary && opCode < OpCode::LastUnary)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode);
      ezUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{}\n", r, x);
    }
    else if (opCode > OpCode::FirstBinary && opCode < OpCode::LastBinary)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode);
      ezUInt32 a = GetRegisterIndex(pByteCode);
      ezUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{}\n", r, a, b);
    }
    else if (opCode > OpCode::FirstBinaryWithConstant && opCode < OpCode::LastBinaryWithConstant)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode);
      ezUInt32 a = GetRegisterIndex(pByteCode);
      ezUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} ", r, a);
      AppendConstant(b, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode > OpCode::FirstTernary && opCode < OpCode::LastTernary)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode);
      ezUInt32 a = GetRegisterIndex(pByteCode);
      ezUInt32 b = GetRegisterIndex(pByteCode);
      ezUInt32 c = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{} r{}\n", r, a, b, c);
    }
    else if (opCode == OpCode::MovX_C)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode);
      ezUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} ", r);
      AppendConstant(x, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode == OpCode::LoadF || opCode == OpCode::LoadI)
    {
      ezUInt32 r = GetRegisterIndex(pByteCode);
      ezUInt32 i = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} i{}({})\n", r, i, m_pInputs[i].m_sName);
    }
    else if (opCode == OpCode::StoreF || opCode == OpCode::StoreI)
    {
      ezUInt32 o = GetRegisterIndex(pByteCode);
      ezUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("o{}({}) r{}\n", o, m_pOutputs[o].m_sName, r);
    }
    else if (opCode == OpCode::Call)
    {
      ezUInt32 uiIndex = GetFunctionIndex(pByteCode);
      const char* szName = m_pFunctions[uiIndex].m_sName;

      ezStringBuilder sName;
      if (ezStringUtils::IsNullOrEmpty(szName))
      {
        sName.SetFormat("Unknown_{0}", uiIndex);
      }
      else
      {
        sName = szName;
      }

      ezUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("{1} r{2}", sName, r);

      ezUInt32 uiNumArgs = GetFunctionArgCount(pByteCode);
      for (ezUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
      {
        ezUInt32 x = GetRegisterIndex(pByteCode);
        out_sDisassembly.AppendFormat(" r{0}", x);
      }

      out_sDisassembly.Append("\n");
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

static constexpr ezTypeVersion s_uiByteCodeVersion = 6;

ezResult ezExpressionByteCode::Save(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiByteCodeVersion);

  ezUInt32 uiDataSize = static_cast<ezUInt32>(m_Data.GetByteBlobPtr().GetCount());

  inout_stream << uiDataSize;

  inout_stream << m_uiNumInputs;
  for (auto& input : GetInputs())
  {
    EZ_SUCCEED_OR_RETURN(input.Serialize(inout_stream));
  }

  inout_stream << m_uiNumOutputs;
  for (auto& output : GetOutputs())
  {
    EZ_SUCCEED_OR_RETURN(output.Serialize(inout_stream));
  }

  inout_stream << m_uiNumFunctions;
  for (auto& function : GetFunctions())
  {
    EZ_SUCCEED_OR_RETURN(function.Serialize(inout_stream));
  }

  inout_stream << m_uiByteCodeCount;
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType)));

  inout_stream << m_uiNumTempRegisters;
  inout_stream << m_uiNumInstructions;

  return EZ_SUCCESS;
}

ezResult ezExpressionByteCode::Load(ezStreamReader& inout_stream, ezByteArrayPtr externalMemory /*= ezByteArrayPtr()*/)
{
  ezTypeVersion version = inout_stream.ReadVersion(s_uiByteCodeVersion);
  if (version != s_uiByteCodeVersion)
  {
    ezLog::Error("Invalid expression byte code version {}. Expected {}", version, s_uiByteCodeVersion);
    return EZ_FAILURE;
  }

  ezUInt32 uiDataSize = 0;
  inout_stream >> uiDataSize;

  void* pData = nullptr;
  if (externalMemory.IsEmpty())
  {
    m_Data.SetCountUninitialized(uiDataSize);
    m_Data.ZeroFill();
    pData = m_Data.GetByteBlobPtr().GetPtr();
  }
  else
  {
    if (externalMemory.GetCount() < uiDataSize)
    {
      ezLog::Error("External memory is too small. Expected at least {} bytes but got {} bytes.", uiDataSize, externalMemory.GetCount());
      return EZ_FAILURE;
    }

    if (ezMemoryUtils::IsAligned(externalMemory.GetPtr(), EZ_ALIGNMENT_OF(ezExpression::StreamDesc)) == false)
    {
      ezLog::Error("External memory is not properly aligned. Expected an alignment of at least {} bytes.", EZ_ALIGNMENT_OF(ezExpression::StreamDesc));
      return EZ_FAILURE;
    }

    pData = externalMemory.GetPtr();
  }

  // Inputs
  {
    inout_stream >> m_uiNumInputs;
    m_pInputs = static_cast<ezExpression::StreamDesc*>(pData);
    for (ezUInt32 i = 0; i < m_uiNumInputs; ++i)
    {
      EZ_SUCCEED_OR_RETURN(m_pInputs[i].Deserialize(inout_stream));
    }

    pData = ezMemoryUtils::AddByteOffset(pData, GetInputs().ToByteArray().GetCount());
  }

  // Outputs
  {
    inout_stream >> m_uiNumOutputs;
    m_pOutputs = static_cast<ezExpression::StreamDesc*>(pData);
    for (ezUInt32 i = 0; i < m_uiNumOutputs; ++i)
    {
      EZ_SUCCEED_OR_RETURN(m_pOutputs[i].Deserialize(inout_stream));
    }

    pData = ezMemoryUtils::AddByteOffset(pData, GetOutputs().ToByteArray().GetCount());
  }

  // Functions
  {
    pData = ezMemoryUtils::AlignForwards(pData, EZ_ALIGNMENT_OF(ezExpression::FunctionDesc));

    inout_stream >> m_uiNumFunctions;
    m_pFunctions = static_cast<ezExpression::FunctionDesc*>(pData);
    for (ezUInt32 i = 0; i < m_uiNumFunctions; ++i)
    {
      EZ_SUCCEED_OR_RETURN(m_pFunctions[i].Deserialize(inout_stream));
    }

    pData = ezMemoryUtils::AddByteOffset(pData, GetFunctions().ToByteArray().GetCount());
  }

  // ByteCode
  {
    pData = ezMemoryUtils::AlignForwards(pData, EZ_ALIGNMENT_OF(StorageType));

    inout_stream >> m_uiByteCodeCount;
    m_pByteCode = static_cast<StorageType*>(pData);
    inout_stream.ReadBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType));
  }

  inout_stream >> m_uiNumTempRegisters;
  inout_stream >> m_uiNumInstructions;

  return EZ_SUCCESS;
}

void ezExpressionByteCode::Init(ezArrayPtr<const StorageType> byteCode, ezArrayPtr<const ezExpression::StreamDesc> inputs, ezArrayPtr<const ezExpression::StreamDesc> outputs, ezArrayPtr<const ezExpression::FunctionDesc> functions, ezUInt32 uiNumTempRegisters, ezUInt32 uiNumInstructions)
{
  ezUInt32 uiOutputsOffset = 0;
  ezUInt32 uiFunctionsOffset = 0;
  ezUInt32 uiByteCodeOffset = 0;

  ezUInt32 uiDataSize = 0;
  uiDataSize += inputs.ToByteArray().GetCount();
  uiOutputsOffset = uiDataSize;
  uiDataSize += outputs.ToByteArray().GetCount();

  uiDataSize = ezMemoryUtils::AlignSize<ezUInt32>(uiDataSize, EZ_ALIGNMENT_OF(ezExpression::FunctionDesc));
  uiFunctionsOffset = uiDataSize;
  uiDataSize += functions.ToByteArray().GetCount();

  uiDataSize = ezMemoryUtils::AlignSize<ezUInt32>(uiDataSize, EZ_ALIGNMENT_OF(StorageType));
  uiByteCodeOffset = uiDataSize;
  uiDataSize += byteCode.ToByteArray().GetCount();

  m_Data.SetCountUninitialized(uiDataSize);
  m_Data.ZeroFill();

  void* pData = m_Data.GetByteBlobPtr().GetPtr();

  EZ_ASSERT_DEV(inputs.GetCount() < ezSmallInvalidIndex, "Too many inputs");
  m_pInputs = static_cast<ezExpression::StreamDesc*>(pData);
  m_uiNumInputs = static_cast<ezUInt16>(inputs.GetCount());
  ezMemoryUtils::Copy(m_pInputs, inputs.GetPtr(), m_uiNumInputs);

  EZ_ASSERT_DEV(outputs.GetCount() < ezSmallInvalidIndex, "Too many outputs");
  m_pOutputs = static_cast<ezExpression::StreamDesc*>(ezMemoryUtils::AddByteOffset(pData, uiOutputsOffset));
  m_uiNumOutputs = static_cast<ezUInt16>(outputs.GetCount());
  ezMemoryUtils::Copy(m_pOutputs, outputs.GetPtr(), m_uiNumOutputs);

  EZ_ASSERT_DEV(functions.GetCount() < ezSmallInvalidIndex, "Too many functions");
  m_pFunctions = static_cast<ezExpression::FunctionDesc*>(ezMemoryUtils::AddByteOffset(pData, uiFunctionsOffset));
  m_uiNumFunctions = static_cast<ezUInt16>(functions.GetCount());
  ezMemoryUtils::Copy(m_pFunctions, functions.GetPtr(), m_uiNumFunctions);

  m_pByteCode = static_cast<StorageType*>(ezMemoryUtils::AddByteOffset(pData, uiByteCodeOffset));
  m_uiByteCodeCount = byteCode.GetCount();
  ezMemoryUtils::Copy(m_pByteCode, byteCode.GetPtr(), m_uiByteCodeCount);

  EZ_ASSERT_DEV(uiNumTempRegisters < ezSmallInvalidIndex, "Too many temp registers");
  m_uiNumTempRegisters = static_cast<ezUInt16>(uiNumTempRegisters);
  m_uiNumInstructions = uiNumInstructions;
}


EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionByteCode);
