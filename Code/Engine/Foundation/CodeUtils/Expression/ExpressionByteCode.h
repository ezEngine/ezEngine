#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Containers/DynamicArray.h>

class ezStreamWriter;
class ezStreamReader;

class EZ_FOUNDATION_DLL ezExpressionByteCode
{
public:
  struct OpCode
  {
    enum Enum
    {
      Nop,

      FirstUnary,

      AbsF_R,
      AbsI_R,
      SqrtF_R,

      ExpF_R,
      LnF_R,
      Log2F_R,
      Log2I_R,
      Log10F_R,
      Pow2F_R,

      SinF_R,
      CosF_R,
      TanF_R,

      ASinF_R,
      ACosF_R,
      ATanF_R,

      RoundF_R,
      FloorF_R,
      CeilF_R,
      TruncF_R,

      NotI_R,
      NotB_R,

      IToF_R,
      FToI_R,

      LastUnary,

      FirstBinary,

      AddF_RR,
      AddI_RR,

      SubF_RR,
      SubI_RR,

      MulF_RR,
      MulI_RR,

      DivF_RR,
      DivI_RR,

      MinF_RR,
      MinI_RR,

      MaxF_RR,
      MaxI_RR,

      ShlI_RR,
      ShrI_RR,
      AndI_RR,
      XorI_RR,
      OrI_RR,

      EqF_RR,
      EqI_RR,
      EqB_RR,

      NEqF_RR,
      NEqI_RR,
      NEqB_RR,

      LtF_RR,
      LtI_RR,

      LEqF_RR,
      LEqI_RR,

      GtF_RR,
      GtI_RR,

      GEqF_RR,
      GEqI_RR,

      AndB_RR,
      OrB_RR,

      LastBinary,

      FirstBinaryWithConstant,

      AddF_RC,
      AddI_RC,

      SubF_RC,
      SubI_RC,

      MulF_RC,
      MulI_RC,

      DivF_RC,
      DivI_RC,

      MinF_RC,
      MinI_RC,

      MaxF_RC,
      MaxI_RC,

      ShlI_RC,
      ShrI_RC,
      AndI_RC,
      XorI_RC,
      OrI_RC,

      EqF_RC,
      EqI_RC,
      EqB_RC,

      NEqF_RC,
      NEqI_RC,
      NEqB_RC,

      LtF_RC,
      LtI_RC,

      LEqF_RC,
      LEqI_RC,

      GtF_RC,
      GtI_RC,

      GEqF_RC,
      GEqI_RC,

      AndB_RC,
      OrB_RC,

      LastBinaryWithConstant,

      FirstTernary,

      SelF_RRR,
      SelI_RRR,
      SelB_RRR,

      LastTernary,

      FirstSpecial,

      MovX_R,
      MovX_C,
      LoadF,
      LoadI,
      StoreF,
      StoreI,

      Call,

      LastSpecial,

      Count
    };

    static const char* GetName(Enum opCode);
  };

  using StorageType = ezUInt32;

  ezExpressionByteCode();
  ~ezExpressionByteCode();

  bool operator==(const ezExpressionByteCode& other) const;
  bool operator!=(const ezExpressionByteCode& other) const { return !(*this == other); }

  void Clear();
  bool IsEmpty() const { return m_ByteCode.IsEmpty(); }

  const StorageType* GetByteCode() const;
  const StorageType* GetByteCodeEnd() const;

  ezUInt32 GetNumInstructions() const;
  ezUInt32 GetNumTempRegisters() const;
  ezArrayPtr<const ezExpression::StreamDesc> GetInputs() const;
  ezArrayPtr<const ezExpression::StreamDesc> GetOutputs() const;
  ezArrayPtr<const ezExpression::FunctionDesc> GetFunctions() const;

  static OpCode::Enum GetOpCode(const StorageType*& pByteCode);
  static ezUInt32 GetRegisterIndex(const StorageType*& pByteCode);
  static ezExpression::Register GetConstant(const StorageType*& pByteCode);
  static ezUInt32 GetFunctionIndex(const StorageType*& pByteCode);
  static ezUInt32 GetFunctionArgCount(const StorageType*& pByteCode);

  void Disassemble(ezStringBuilder& out_sDisassembly) const;

  void Save(ezStreamWriter& stream) const;
  ezResult Load(ezStreamReader& stream);

private:
  friend class ezExpressionCompiler;

  ezDynamicArray<StorageType> m_ByteCode;
  ezDynamicArray<ezExpression::StreamDesc> m_Inputs;
  ezDynamicArray<ezExpression::StreamDesc> m_Outputs;
  ezDynamicArray<ezExpression::FunctionDesc> m_Functions;

  ezUInt32 m_uiNumInstructions = 0;
  ezUInt32 m_uiNumTempRegisters = 0;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionByteCode_inl.h>
