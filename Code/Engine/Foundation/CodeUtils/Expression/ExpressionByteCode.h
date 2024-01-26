#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Containers/Blob.h>

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

    static const char* GetName(Enum code);
  };

  using StorageType = ezUInt32;

  ezExpressionByteCode();
  ezExpressionByteCode(const ezExpressionByteCode& other);
  ~ezExpressionByteCode();

  void operator=(const ezExpressionByteCode& other);

  bool operator==(const ezExpressionByteCode& other) const;
  bool operator!=(const ezExpressionByteCode& other) const { return !(*this == other); }

  void Clear();
  bool IsEmpty() const { return m_uiByteCodeCount == 0; }

  const StorageType* GetByteCodeStart() const;
  const StorageType* GetByteCodeEnd() const;
  ezArrayPtr<const StorageType> GetByteCode() const;

  ezUInt32 GetNumInstructions() const;
  ezUInt32 GetNumTempRegisters() const;
  ezArrayPtr<const ezExpression::StreamDesc> GetInputs() const;
  ezArrayPtr<const ezExpression::StreamDesc> GetOutputs() const;
  ezArrayPtr<const ezExpression::FunctionDesc> GetFunctions() const;

  static OpCode::Enum GetOpCode(const StorageType*& ref_pByteCode);
  static ezUInt32 GetRegisterIndex(const StorageType*& ref_pByteCode);
  static ezExpression::Register GetConstant(const StorageType*& ref_pByteCode);
  static ezUInt32 GetFunctionIndex(const StorageType*& ref_pByteCode);
  static ezUInt32 GetFunctionArgCount(const StorageType*& ref_pByteCode);

  void Disassemble(ezStringBuilder& out_sDisassembly) const;

  ezResult Save(ezStreamWriter& inout_stream) const;
  ezResult Load(ezStreamReader& inout_stream, ezByteArrayPtr externalMemory = ezByteArrayPtr());

  ezConstByteBlobPtr GetDataBlob() const { return m_Data.GetByteBlobPtr(); }

private:
  friend class ezExpressionCompiler;

  void Init(ezArrayPtr<const StorageType> byteCode, ezArrayPtr<const ezExpression::StreamDesc> inputs, ezArrayPtr<const ezExpression::StreamDesc> outputs, ezArrayPtr<const ezExpression::FunctionDesc> functions, ezUInt32 uiNumTempRegisters, ezUInt32 uiNumInstructions);

  ezBlob m_Data;

  ezExpression::StreamDesc* m_pInputs = nullptr;
  ezExpression::StreamDesc* m_pOutputs = nullptr;
  ezExpression::FunctionDesc* m_pFunctions = nullptr;
  StorageType* m_pByteCode = nullptr;

  ezUInt32 m_uiByteCodeCount = 0;
  ezUInt16 m_uiNumInputs = 0;
  ezUInt16 m_uiNumOutputs = 0;
  ezUInt16 m_uiNumFunctions = 0;

  ezUInt16 m_uiNumTempRegisters = 0;
  ezUInt32 m_uiNumInstructions = 0;
};

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
static_assert(sizeof(ezExpressionByteCode) == 64);
#endif

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezExpressionByteCode);
EZ_DECLARE_CUSTOM_VARIANT_TYPE(ezExpressionByteCode);

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionByteCode_inl.h>
