#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Types/UniquePtr.h>

class EZ_FOUNDATION_DLL ezExpressionVM
{
public:
  ezExpressionVM();
  ~ezExpressionVM();

  void RegisterFunction(const ezExpressionFunction& func);
  void UnregisterFunction(const ezExpressionFunction& func);

  ezResult Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs, ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData = ezExpression::GlobalData());

private:
  void RegisterDefaultFunctions();

  ezResult ScalarizeStreams(ezArrayPtr<const ezProcessingStream> streams, ezDynamicArray<ezProcessingStream>& out_ScalarizedStreams);
  ezResult MapStreams(ezArrayPtr<const ezExpression::StreamDesc> streamDescs, ezArrayPtr<ezProcessingStream> streams, const char* szStreamType, ezUInt32 uiNumInstances, ezDynamicArray<ezProcessingStream*>& out_MappedStreams);
  ezResult MapFunctions(ezArrayPtr<const ezExpression::FunctionDesc> functionDescs, const ezExpression::GlobalData& globalData);

  ezDynamicArray<ezExpression::Register, ezAlignedAllocatorWrapper> m_Registers;

  ezDynamicArray<ezProcessingStream> m_ScalarizedInputs;
  ezDynamicArray<ezProcessingStream> m_ScalarizedOutputs;

  ezDynamicArray<ezProcessingStream*> m_MappedInputs;
  ezDynamicArray<ezProcessingStream*> m_MappedOutputs;
  ezDynamicArray<const ezExpressionFunction*> m_MappedFunctions;

  ezDynamicArray<ezExpressionFunction> m_Functions;
  ezHashTable<ezHashedString, ezUInt32> m_FunctionNamesToIndex;
};
