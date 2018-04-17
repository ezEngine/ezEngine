#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionByteCode.h>
#include <ProceduralPlacementPlugin/VM/ExpressionVM.h>

namespace
{
  //#define DEBUG_VM

  #ifdef DEBUG_VM
  #define VM_INLINE
  #else
  #define VM_INLINE EZ_ALWAYS_INLINE
  #endif

  template <typename Func>
  VM_INLINE void VMOperation1(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* x = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;
    while (r != re)
    {
      *r = func(*x);
      ++r; ++x;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation1_C(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f x = ezExpressionByteCode::GetConstant(pByteCode);
    ezSimdVec4f* re = r + uiNumRegisters;
    while (r != re)
    {
      *r = func(x);
      ++r;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* a = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* b = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;
    while (r != re)
    {
      *r = func(*a, *b);
      ++r; ++a; ++b;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2_C(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f a = ezExpressionByteCode::GetConstant(pByteCode);
    ezSimdVec4f* b = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;
    while (r != re)
    {
      *r = func(a, *b);
      ++r; ++b;
    }
  }
}

ezExpressionVM::ezExpressionVM(ezUInt32 uiTempRegistersInBytes /*= 256 * 1024*/)
{
  ezUInt32 uiNumTempRegisters = (uiTempRegistersInBytes + sizeof(ezSimdVec4f) - 1) / sizeof(ezSimdVec4f);
  m_Registers.SetCountUninitialized(uiNumTempRegisters);
}

ezExpressionVM::~ezExpressionVM()
{

}

void ezExpressionVM::Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const float> inputs, ezArrayPtr<float> outputs, ezUInt32 uiNumInstances)
{
  EZ_ASSERT_DEV(inputs.GetCount() % uiNumInstances == 0, "Num inputs must be multiple of num instances");
  EZ_ASSERT_DEV(outputs.GetCount() % uiNumInstances == 0, "Num outputs must be multiple of num instances");

  ezSimdVec4f* pRegisters = m_Registers.GetData();
  const ezUInt32 uiNumRegisters = (uiNumInstances + 3) / 4;
  const ezUInt32 uiLastInstanceIndex = uiNumInstances - 1;

  const ezUInt32 uiTotalNumRegisters = (byteCode.GetNumInputRegisters() + byteCode.GetNumTempRegisters()) * uiNumRegisters;
  if (uiTotalNumRegisters > m_Registers.GetCount())
  {
    EZ_REPORT_FAILURE("Not enough registers to execute bytecode. Needs {0} but only has {1}.",
      uiTotalNumRegisters, m_Registers.GetCount());
    return;
  }

  // Load inputs
  const ezUInt32 uiNumInputs = inputs.GetCount() / uiNumInstances;
  EZ_ASSERT_DEV(uiNumInputs == byteCode.GetNumInputRegisters(), "Provided num inputs {0} does not match bytecode inputs {1}", uiNumInputs, byteCode.GetNumInputRegisters());
  for (ezUInt32 uiInputIndex = 0; uiInputIndex < uiNumInputs; ++uiInputIndex)
  {
    ezUInt32 s = uiInputIndex * uiNumInstances;
    ezUInt32 i = 0;
    ezSimdVec4f* r = pRegisters + uiInputIndex * uiNumRegisters;
    ezSimdVec4f* re = r + uiNumRegisters;
    while (r != re)
    {
      float x = inputs[s+i]; i += i < uiLastInstanceIndex;
      float y = inputs[s+i]; i += i < uiLastInstanceIndex;
      float z = inputs[s+i]; i += i < uiLastInstanceIndex;
      float w = inputs[s+i]; i += i < uiLastInstanceIndex;

      r->Set(x, y, z, w);
      ++r;
    }
  }

  // Execute bytecode
  const ezExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCode();
  const ezExpressionByteCode::StorageType* pEndByteCode = byteCode.GetByteCodeEnd();

  while (pByteCode < pEndByteCode)
  {
    ezExpressionByteCode::OpCode::Enum opCode = ezExpressionByteCode::GetOpCode(pByteCode);

    switch (opCode)
    {
    case ezExpressionByteCode::OpCode::Add_RR:
      VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a + b; });
      break;

    case ezExpressionByteCode::OpCode::Add_CR:
      VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a + b; });
      break;

    case ezExpressionByteCode::OpCode::Sub_RR:
      VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a - b; });
      break;

    case ezExpressionByteCode::OpCode::Sub_CR:
      VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a - b; });
      break;

    case ezExpressionByteCode::OpCode::Mul_RR:
      VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMul(b); });
      break;

    case ezExpressionByteCode::OpCode::Mul_CR:
      VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMul(b); });
      break;

    case ezExpressionByteCode::OpCode::Div_RR:
      VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompDiv(b); });
      break;

    case ezExpressionByteCode::OpCode::Div_CR:
      VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompDiv(b); });
      break;

    case ezExpressionByteCode::OpCode::Min_RR:
      VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMin(b); });
      break;

    case ezExpressionByteCode::OpCode::Min_CR:
      VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMin(b); });
      break;

    case ezExpressionByteCode::OpCode::Max_RR:
      VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMax(b); });
      break;

    case ezExpressionByteCode::OpCode::Max_CR:
      VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMax(b); });
      break;

    case ezExpressionByteCode::OpCode::Sqrt_R:
      VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x.GetSqrt(); });
      break;

    case ezExpressionByteCode::OpCode::Mov_R:
      VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x; });
      break;

    case ezExpressionByteCode::OpCode::Mov_C:
      VMOperation1_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x; });
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return;
    }
  }

  // Store outputs
  const ezUInt32 uiNumOutputs = outputs.GetCount() / uiNumInstances;
  for (ezUInt32 uiOutputIndex = 0; uiOutputIndex < uiNumOutputs; ++uiOutputIndex)
  {
    ezUInt32 s = uiOutputIndex * uiNumInstances;
    ezUInt32 i = 0;
    ezSimdVec4f* r = pRegisters + uiOutputIndex * uiNumRegisters;
    ezSimdVec4f* re = r + uiNumRegisters;
    while (r != re)
    {
      float output[4];
      r->Store<4>(output);

      outputs[s + i] = output[0]; i += i < uiLastInstanceIndex;
      outputs[s + i] = output[1]; i += i < uiLastInstanceIndex;
      outputs[s + i] = output[2]; i += i < uiLastInstanceIndex;
      outputs[s + i] = output[3]; i += i < uiLastInstanceIndex;

      ++r;
    }
  }
}

// static
void ezExpressionVM::Test()
{
#if 0
  float input[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  float output[5] = {};

  ezExpressionByteCode byteCode;

  ezUInt32 instruction[4];
  {
    instruction[0] = ezExpressionByteCode::OpCode::Add_RR;
    instruction[1] = 2;
    instruction[2] = 0;
    instruction[3] = 1;
    byteCode.m_ByteCode.PushBackRange(ezArrayPtr<ezUInt32>(instruction, 4));
  }

  {
    instruction[0] = ezExpressionByteCode::OpCode::Sqrt_R;
    instruction[1] = 2;
    instruction[2] = 2;
    byteCode.m_ByteCode.PushBackRange(ezArrayPtr<ezUInt32>(instruction, 3));
  }

  {
    float c = 10.0f;

    instruction[0] = ezExpressionByteCode::OpCode::Mul_CR;
    instruction[1] = 0;
    instruction[2] = *reinterpret_cast<ezExpressionByteCode::StorageType*>(&c);
    instruction[3] = 2;
    byteCode.m_ByteCode.PushBackRange(ezArrayPtr<ezUInt32>(instruction, 4));
  }

  byteCode.m_uiNumInputRegisters = 2;
  byteCode.m_uiNumTempRegisters = 1;

  ezExpressionVM vm;
  vm.Execute(byteCode, ezMakeArrayPtr(input), ezMakeArrayPtr(output), 5);

  for (int i = 0; i < 5; ++i)
  {
    float cmp = ezMath::Sqrt(input[i] + input[i + 5]) * 10.0f;
    EZ_ASSERT_DEV(ezMath::IsEqual(output[i], cmp, ezMath::BasicType<float>::DefaultEpsilon()), "");
  }
#endif
}


