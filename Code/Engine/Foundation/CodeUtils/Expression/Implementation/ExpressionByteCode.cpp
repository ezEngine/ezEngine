#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Logging/Log.h>

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

const char* ezExpressionByteCode::OpCode::GetName(Enum opCode)
{
  EZ_ASSERT_DEBUG(opCode >= 0 && opCode < EZ_ARRAY_SIZE(s_szOpCodeNames), "Out of bounds access");
  return s_szOpCodeNames[opCode];
}

//////////////////////////////////////////////////////////////////////////

ezExpressionByteCode::ezExpressionByteCode() = default;
ezExpressionByteCode::~ezExpressionByteCode() = default;

bool ezExpressionByteCode::operator==(const ezExpressionByteCode& other) const
{
  return m_ByteCode == other.m_ByteCode &&
         m_Inputs == other.m_Inputs &&
         m_Outputs == other.m_Outputs &&
         m_Functions == other.m_Functions;
}

void ezExpressionByteCode::Clear()
{
  m_ByteCode.Clear();
  m_Inputs.Clear();
  m_Outputs.Clear();
  m_Functions.Clear();

  m_uiNumInstructions = 0;
  m_uiNumTempRegisters = 0;
}

void ezExpressionByteCode::Disassemble(ezStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.Append("// Inputs:\n");
  for (ezUInt32 i = 0; i < m_Inputs.GetCount(); ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_Inputs[i].m_sName, ezProcessingStream::GetDataTypeName(m_Inputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Outputs:\n");
  for (ezUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_Outputs[i].m_sName, ezProcessingStream::GetDataTypeName(m_Outputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Functions:\n");
  for (ezUInt32 i = 0; i < m_Functions.GetCount(); ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {} {}(", i, ezExpression::RegisterType::GetName(m_Functions[i].m_OutputType), m_Functions[i].m_sName);
    const ezUInt32 uiNumArguments = m_Functions[i].m_InputTypes.GetCount();
    for (ezUInt32 j = 0; j < uiNumArguments; ++j)
    {
      out_sDisassembly.Append(ezExpression::RegisterType::GetName(m_Functions[i].m_InputTypes[j]));
      if (j < uiNumArguments - 1)
      {
        out_sDisassembly.Append(", ");
      }
    }
    out_sDisassembly.Append(")\n");
  }

  out_sDisassembly.AppendFormat("\n// Temp Registers: {}\n", m_uiNumTempRegisters);
  out_sDisassembly.AppendFormat("// Instructions: {}\n\n", m_uiNumInstructions);

  auto AppendConstant = [](ezUInt32 x, ezStringBuilder& out_String) {
    out_String.AppendFormat("0x{}({})", ezArgU(x, 8, true, 16), ezArgF(*reinterpret_cast<float*>(&x), 6));
  };

  const StorageType* pByteCode = GetByteCode();
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

      out_sDisassembly.AppendFormat("r{} i{}({})\n", r, i, m_Inputs[i].m_sName);
    }
    else if (opCode == OpCode::StoreF || opCode == OpCode::StoreI)
    {
      ezUInt32 o = GetRegisterIndex(pByteCode);
      ezUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("o{}({}) r{}\n", o, m_Outputs[o].m_sName, r);
    }
    else if (opCode == OpCode::Call)
    {
      ezUInt32 uiIndex = GetFunctionIndex(pByteCode);
      const char* szName = m_Functions[uiIndex].m_sName;

      ezStringBuilder sName;
      if (ezStringUtils::IsNullOrEmpty(szName))
      {
        sName.Format("Unknown_{0}", uiIndex);
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

static constexpr ezUInt32 s_uiMetaDataVersion = 4;
static constexpr ezUInt32 s_uiCodeVersion = 3;

void ezExpressionByteCode::Save(ezStreamWriter& stream) const
{
  ezChunkStreamWriter chunk(stream);

  chunk.BeginStream(1);

  {
    chunk.BeginChunk("MetaData", s_uiMetaDataVersion);

    chunk << m_uiNumInstructions;
    chunk << m_uiNumTempRegisters;
    chunk.WriteArray(m_Inputs).IgnoreResult();
    chunk.WriteArray(m_Outputs).IgnoreResult();
    chunk.WriteArray(m_Functions).IgnoreResult();

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("Code", s_uiCodeVersion);

    chunk << m_ByteCode.GetCount();
    chunk.WriteBytes(m_ByteCode.GetData(), m_ByteCode.GetCount() * sizeof(StorageType)).IgnoreResult();

    chunk.EndChunk();
  }

  chunk.EndStream();
}

ezResult ezExpressionByteCode::Load(ezStreamReader& stream)
{
  ezChunkStreamReader chunk(stream);
  chunk.SetEndChunkFileMode(ezChunkStreamReader::EndChunkFileMode::SkipToEnd);

  chunk.BeginStream();

  while (chunk.GetCurrentChunk().m_bValid)
  {
    if (chunk.GetCurrentChunk().m_sChunkName == "MetaData")
    {
      if (chunk.GetCurrentChunk().m_uiChunkVersion >= s_uiMetaDataVersion)
      {
        chunk >> m_uiNumInstructions;
        chunk >> m_uiNumTempRegisters;
        EZ_SUCCEED_OR_RETURN(chunk.ReadArray(m_Inputs));
        EZ_SUCCEED_OR_RETURN(chunk.ReadArray(m_Outputs));
        EZ_SUCCEED_OR_RETURN(chunk.ReadArray(m_Functions));
      }
      else
      {
        ezLog::Error("Invalid MetaData Chunk Version {}. Expected >= {}", chunk.GetCurrentChunk().m_uiChunkVersion, s_uiMetaDataVersion);

        chunk.EndStream();
        return EZ_FAILURE;
      }
    }
    else if (chunk.GetCurrentChunk().m_sChunkName == "Code")
    {
      if (chunk.GetCurrentChunk().m_uiChunkVersion >= s_uiCodeVersion)
      {
        ezUInt32 uiByteCodeCount = 0;
        chunk >> uiByteCodeCount;

        m_ByteCode.SetCountUninitialized(uiByteCodeCount);
        chunk.ReadBytes(m_ByteCode.GetData(), uiByteCodeCount * sizeof(StorageType));
      }
      else
      {
        ezLog::Error("Invalid Code Chunk Version {}. Expected >= {}", chunk.GetCurrentChunk().m_uiChunkVersion, s_uiCodeVersion);

        chunk.EndStream();
        return EZ_FAILURE;
      }
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionByteCode);
