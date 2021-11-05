#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/SimdMath/SimdVec4f.h>

class EZ_FOUNDATION_DLL ezExpressionByteCode
{
public:
  struct OpCode
  {
    enum Enum
    {
      // Unary
      FirstUnary,

      Abs_R,
      Sqrt_R,

      Sin_R,
      Cos_R,
      Tan_R,

      ASin_R,
      ACos_R,
      ATan_R,

      Mov_R,
      Mov_C,
      Load,
      Store,

      LastUnary,

      // Binary
      FirstBinary,

      Add_RR,
      Add_CR,

      Sub_RR,
      Sub_CR,

      Mul_RR,
      Mul_CR,

      Div_RR,
      Div_CR,

      Min_RR,
      Min_CR,

      Max_RR,
      Max_CR,

      LastBinary,

      Call,

      Nop,

      Count
    };
  };

  typedef ezUInt32 StorageType;

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
  ezArrayPtr<const ezHashedString> GetInputs() const;
  ezArrayPtr<const ezHashedString> GetOutputs() const;
  ezArrayPtr<const ezHashedString> GetFunctions() const;

  static OpCode::Enum GetOpCode(const StorageType*& pByteCode);
  static ezUInt32 GetRegisterIndex(const StorageType*& pByteCode, ezUInt32 uiNumRegisters);
  static ezSimdVec4f GetConstant(const StorageType*& pByteCode);
  static ezUInt32 GetFunctionIndex(const StorageType*& pByteCode);
  static ezUInt32 GetFunctionArgCount(const StorageType*& pByteCode);

  void Disassemble(ezStringBuilder& out_sDisassembly) const;
  static const char* GetOpCodeName(OpCode::Enum opCode);

  void Save(ezStreamWriter& stream) const;
  ezResult Load(ezStreamReader& stream);

private:
  friend class ezExpressionCompiler;

  ezDynamicArray<StorageType> m_ByteCode;
  ezDynamicArray<ezHashedString> m_Inputs;
  ezDynamicArray<ezHashedString> m_Outputs;
  ezDynamicArray<ezHashedString> m_Functions;

  ezUInt32 m_uiNumInstructions = 0;
  ezUInt32 m_uiNumTempRegisters = 0;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionByteCode_inl.h>
