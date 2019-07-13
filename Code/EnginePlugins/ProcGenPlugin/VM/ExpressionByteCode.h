#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>

class EZ_PROCGENPLUGIN_DLL ezExpressionByteCode
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
      Mov_I,
      Mov_O,

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

      Count
    };
  };

  typedef ezUInt32 StorageType;

  ezExpressionByteCode();
  ~ezExpressionByteCode();

  const StorageType* GetByteCode() const;
  const StorageType* GetByteCodeEnd() const;

  ezUInt32 GetNumInstructions() const;
  ezUInt32 GetNumTempRegisters() const;
  ezArrayPtr<const ezHashedString> GetInputs() const;
  ezArrayPtr<const ezHashedString> GetOutputs() const;

  static OpCode::Enum GetOpCode(const StorageType*& pByteCode);
  static ezUInt32 GetRegisterIndex(const StorageType*& pByteCode, ezUInt32 uiNumRegisters);
  static ezSimdVec4f GetConstant(const StorageType*& pByteCode);
  static ezUInt32 GetFunctionNameHash(const StorageType*& pByteCode);
  static ezUInt32 GetFunctionArgCount(const StorageType*& pByteCode);

  void Disassemble(ezStringBuilder& out_sDisassembly) const;

  void Save(ezStreamWriter& stream) const;
  ezResult Load(ezStreamReader& stream);

private:
  friend class ezExpressionCompiler;

  ezDynamicArray<StorageType> m_ByteCode;
  ezDynamicArray<ezHashedString> m_Inputs;
  ezDynamicArray<ezHashedString> m_Outputs;

  ezUInt32 m_uiNumInstructions;
  ezUInt32 m_uiNumTempRegisters;
};

#include <ProcGenPlugin/VM/Implementation/ExpressionByteCode_inl.h>
