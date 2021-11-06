#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionFunctions.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class ezExpressionByteCode;

class EZ_FOUNDATION_DLL ezExpressionVM
{
public:
  ezExpressionVM();
  ~ezExpressionVM();

  void RegisterFunction(const char* szName, ezExpressionFunction func, ezExpressionValidateGlobalData validationFunc = ezExpressionValidateGlobalData());

  void RegisterDefaultFunctions();

  ezResult Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs, ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData = ezExpression::GlobalData());

private:
  void ValidateDataSize(const ezProcessingStream& stream, ezUInt32 uiNumInstances, const char* szDataName) const;

  ezDynamicArray<ezSimdVec4f, ezAlignedAllocatorWrapper> m_Registers;

  ezDynamicArray<ezUInt32> m_InputMapping;
  ezDynamicArray<ezUInt32> m_OutputMapping;
  ezDynamicArray<ezUInt32> m_FunctionMapping;

  struct FunctionInfo
  {
    ezHashedString m_sName;
    ezExpressionFunction m_Func;
    ezExpressionValidateGlobalData m_ValidationFunc;
  };

  ezDynamicArray<FunctionInfo> m_Functions;
  ezHashTable<ezHashedString, ezUInt32> m_FunctionNamesToIndex;
};
