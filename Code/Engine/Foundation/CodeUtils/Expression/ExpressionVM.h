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

  struct Flags
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      MapStreamsByName = EZ_BIT(0),
      ScalarizeStreams = EZ_BIT(1),

      UserFriendly = MapStreamsByName | ScalarizeStreams,
      BestPerformance = 0,

      Default = UserFriendly
    };

    struct Bits
    {
      StorageType MapStreamsByName : 1;
      StorageType ScalarizeStreams : 1;
    };
  };

  ezResult Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs, ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData = ezExpression::GlobalData(), ezBitflags<Flags> flags = Flags::Default);

private:
  void RegisterDefaultFunctions();

  static ezResult ScalarizeStreams(ezArrayPtr<const ezProcessingStream> streams, ezDynamicArray<ezProcessingStream>& out_ScalarizedStreams);
  static ezResult AreStreamsScalarized(ezArrayPtr<const ezProcessingStream> streams);
  static ezResult ValidateStream(const ezProcessingStream& stream, const ezExpression::StreamDesc& streamDesc, ezStringView sStreamType, ezUInt32 uiNumInstances);

  template <typename T>
  static ezResult MapStreams(ezArrayPtr<const ezExpression::StreamDesc> streamDescs, ezArrayPtr<T> streams, ezStringView sStreamType, ezUInt32 uiNumInstances, ezBitflags<Flags> flags, ezDynamicArray<T*>& out_MappedStreams);
  ezResult MapFunctions(ezArrayPtr<const ezExpression::FunctionDesc> functionDescs, const ezExpression::GlobalData& globalData);

  ezDynamicArray<ezExpression::Register, ezAlignedAllocatorWrapper> m_Registers;

  ezDynamicArray<ezProcessingStream> m_ScalarizedInputs;
  ezDynamicArray<ezProcessingStream> m_ScalarizedOutputs;

  ezDynamicArray<const ezProcessingStream*> m_MappedInputs;
  ezDynamicArray<ezProcessingStream*> m_MappedOutputs;
  ezDynamicArray<const ezExpressionFunction*> m_MappedFunctions;

  ezDynamicArray<ezExpressionFunction> m_Functions;
  ezHashTable<ezHashedString, ezUInt32> m_FunctionNamesToIndex;
};
