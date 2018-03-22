#pragma once

#include <ProceduralPlacementPlugin/Basics.h>
#include <Foundation/Containers/DynamicArray.h>

struct ezExpressionByteCode
{
  struct OpCode
  {
    enum Enum
    {
      Add,
      Sub,
      Mul,
      Div,
      Sqrt
    };
  };

  // E.g. Add instruction is encoded like
  // Op-Code (4bytes), result register(4bytes), arg1 register(4bytes), arg2 register(4bytes)
  ezDynamicArray<ezUInt32> m_ByteCode;

  ezDynamicArray<float> m_Constants;
  ezUInt32 m_uiNumInputRegisters;
  ezUInt32 m_uiNumTempRegisters;
};

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezExpressionVM
{
public:
  ezExpressionVM(ezUInt32 uiTempRegistersInBytes = 256 * 1024);
  ~ezExpressionVM();

  void Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const float> inputs, ezArrayPtr<float> outputs, ezUInt32 uiNumInstances);


  // TODO: write proper tests
  static void Test();

private:
  ezDynamicArray<ezSimdVec4f, ezAlignedAllocatorWrapper> m_TempRegisters;
};

