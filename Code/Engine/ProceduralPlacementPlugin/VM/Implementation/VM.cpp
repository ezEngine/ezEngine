#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/VM.h>

namespace
{
  EZ_ALWAYS_INLINE ezUInt32 GetRegisterIndex(const ezUInt32*& pCurrent, ezUInt32 uiRoundedNumInstances)
  {
    ezUInt32 uiIndex = *pCurrent * uiRoundedNumInstances;
    ++pCurrent;
    return uiIndex;
  }
}

ezExpressionVM::ezExpressionVM(ezUInt32 uiTempRegistersInBytes /*= 256 * 1024*/)
{
  ezUInt32 uiNumTempRegisters = (uiTempRegistersInBytes + sizeof(ezSimdVec4f) - 1) / sizeof(ezSimdVec4f);
  m_TempRegisters.SetCountUninitialized(uiNumTempRegisters);
}

ezExpressionVM::~ezExpressionVM()
{

}

void ezExpressionVM::Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const float> inputs, ezArrayPtr<float> outputs, ezUInt32 uiNumInstances)
{
  EZ_ASSERT_DEV(inputs.GetCount() % uiNumInstances == 0, "Num inputs must be multiple of num instances");
  EZ_ASSERT_DEV(outputs.GetCount() % uiNumInstances == 0, "Num outputs must be multiple of num instances");

  ezSimdVec4f* pRegisters = m_TempRegisters.GetData();
  const ezUInt32 uiRoundedNumInstances = (uiNumInstances + 3) / 4;
  const ezUInt32 uiLastInstanceIndex = uiNumInstances - 1;

  // Load inputs
  const ezUInt32 uiNumInputs = inputs.GetCount() / uiNumInstances;
  EZ_ASSERT_DEV(uiNumInputs == byteCode.m_uiNumInputRegisters, "Provided num inputs {0} does not match bytecode inputs {1}", uiNumInputs, byteCode.m_uiNumInputRegisters);
  for (ezUInt32 uiInputIndex = 0; uiInputIndex < uiNumInputs; ++uiInputIndex)
  {
    ezUInt32 s = uiInputIndex * uiNumInstances;
    ezUInt32 i = 0;
    ezSimdVec4f* r = pRegisters + uiInputIndex * uiRoundedNumInstances;
    ezSimdVec4f* re = r + uiRoundedNumInstances;
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

  // Load constants
  const ezUInt32 uiConstantsFirstRegister = uiNumInputs + byteCode.m_uiNumTempRegisters;
  const ezUInt32 uiNumConstants = byteCode.m_Constants.GetCount();
  for (ezUInt32 uiConstantIndex = 0; uiConstantIndex < uiNumConstants; ++uiConstantIndex)
  {
    ezSimdVec4f* r = pRegisters + (uiConstantsFirstRegister + uiConstantIndex) * uiRoundedNumInstances;
    ezSimdVec4f* re = r + uiRoundedNumInstances;
    while (r != re)
    {
      r->Set(byteCode.m_Constants[uiConstantIndex]);
      ++r;
    }
  }

  // Execute bytecode
  const ezUInt32* pCurrentByteCode = byteCode.m_ByteCode.GetData();
  const ezUInt32* pEndByteCode = pCurrentByteCode + byteCode.m_ByteCode.GetCount();

  while (pCurrentByteCode < pEndByteCode)
  {
    ezUInt32 uiOpCode = *pCurrentByteCode;
    ++pCurrentByteCode;

    switch (uiOpCode)
    {
    case ezExpressionByteCode::OpCode::Add:
      {
        ezSimdVec4f* r = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* a = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* b = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* re = r + uiRoundedNumInstances;
        while (r != re)
        {
          *r = *a + *b;
          ++r; ++a; ++b;
        }
        break;
      }
    case ezExpressionByteCode::OpCode::Mul:
      {
        ezSimdVec4f* r = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* a = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* b = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* re = r + uiRoundedNumInstances;
        while (r != re)
        {
          *r = a->CompMul(*b);
          ++r; ++a; ++b;
        }
        break;
      }
    case ezExpressionByteCode::OpCode::Sqrt:
      {
        ezSimdVec4f* r = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* x = pRegisters + GetRegisterIndex(pCurrentByteCode, uiRoundedNumInstances);
        ezSimdVec4f* re = r + uiRoundedNumInstances;
        while (r != re)
        {
          *r = x->GetSqrt();
          ++r; ++x;
        }
        break;
      }
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
    ezSimdVec4f* r = pRegisters + uiOutputIndex * uiRoundedNumInstances;
    ezSimdVec4f* re = r + uiRoundedNumInstances;
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
  float input[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
  float output[5] = {};

  ezExpressionByteCode byteCode;

  ezUInt32 instruction[4];
  {
    instruction[0] = ezExpressionByteCode::OpCode::Add;
    instruction[1] = 2;
    instruction[2] = 0;
    instruction[3] = 1;
    byteCode.m_ByteCode.PushBackRange(ezArrayPtr<ezUInt32>(instruction, 4));
  }

  {
    instruction[0] = ezExpressionByteCode::OpCode::Sqrt;
    instruction[1] = 2;
    instruction[2] = 2;
    byteCode.m_ByteCode.PushBackRange(ezArrayPtr<ezUInt32>(instruction, 3));
  }

  {
    instruction[0] = ezExpressionByteCode::OpCode::Mul;
    instruction[1] = 0;
    instruction[2] = 2;
    instruction[3] = 3;
    byteCode.m_ByteCode.PushBackRange(ezArrayPtr<ezUInt32>(instruction, 4));
  }

  byteCode.m_Constants.PushBack(10.0f);
  byteCode.m_uiNumInputRegisters = 2;
  byteCode.m_uiNumTempRegisters = 1;

  ezExpressionVM vm;
  vm.Execute(byteCode, ezMakeArrayPtr(input), ezMakeArrayPtr(output), 5);

  for (int i = 0; i < 5; ++i)
  {
    float cmp = ezMath::Sqrt(input[i] + input[i + 5]) * 10.0f;
    EZ_ASSERT_DEV(ezMath::IsEqual(output[i], cmp, ezMath::BasicType<float>::DefaultEpsilon()), "");
  }
}


