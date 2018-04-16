#pragma once

#include <ProceduralPlacementPlugin/Basics.h>
#include <Foundation/Containers/DynamicArray.h>

class ezExpressionByteCode;

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezExpressionVM
{
public:
  ezExpressionVM(ezUInt32 uiTempRegistersInBytes = 256 * 1024);
  ~ezExpressionVM();

  void Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const float> inputs, ezArrayPtr<float> outputs, ezUInt32 uiNumInstances);


  // TODO: write proper tests
  static void Test();

private:
  ezDynamicArray<ezSimdVec4f, ezAlignedAllocatorWrapper> m_Registers;
};

