#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>

class EZ_FOUNDATION_DLL ezExpressionVM
{
public:
  ezExpressionVM();
  ~ezExpressionVM();

  void RegisterFunction(const ezExpressionFunction& func);

  ezResult Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs, ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData = ezExpression::GlobalData());

private:
  void RegisterDefaultFunctions();

  template <typename StreamType>
  ezResult MapStreams(ezArrayPtr<const ezExpression::StreamDesc> streamDescs, ezArrayPtr<StreamType> streams, const char* szStreamType, ezUInt32 uiNumInstances, ezDynamicArray<StreamType*>& out_MappedStreams);
  ezResult MapFunctions(ezArrayPtr<const ezExpression::FunctionDesc> functionDescs, const ezExpression::GlobalData& globalData);

  ezDynamicArray<ezExpression::Register, ezAlignedAllocatorWrapper> m_Registers;

  ezDynamicArray<const ezProcessingStream*> m_MappedInputs;
  ezDynamicArray<ezProcessingStream*> m_MappedOutputs;
  ezDynamicArray<ezExpressionFunction*> m_MappedFunctions;

  ezDynamicArray<ezExpressionFunction> m_Functions;
  ezHashTable<ezHashedString, ezUInt32> m_FunctionNamesToIndex;  
};
