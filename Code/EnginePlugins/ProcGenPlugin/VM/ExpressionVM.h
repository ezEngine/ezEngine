#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ProcGenPlugin/VM/ExpressionFunctions.h>

class ezExpressionByteCode;

namespace ezExpression
{
  struct Stream
  {
    struct Type
    {
      typedef ezUInt16 StorageType;

      enum Enum
      {
        Float,
        /*Float2, // not supported yet
        Float3, // not supported yet
        Float4, // not supported yet

        Int, // not supported yet
        Int2, // not supported yet
        Int3, // not supported yet
        Int4, // not supported yet
        */

        Count
      };
    };

    Stream();
    Stream(const ezHashedString& sName, Type::Enum type, ezUInt32 uiByteStride);
    Stream(const ezHashedString& sName, Type::Enum type, ezArrayPtr<ezUInt8> data, ezUInt32 uiByteStride);
    ~Stream();

    ezUInt32 GetElementSize() const;
    void ValidateDataSize(ezUInt32 uiNumInstances, const char* szDataName) const;

    ezHashedString m_sName;
    ezArrayPtr<ezUInt8> m_Data;
    ezEnum<Type> m_Type;
    ezUInt16 m_uiByteStride;
  };

  template <typename T>
  Stream MakeStream(ezArrayPtr<T> data, ezUInt32 uiOffset, const ezHashedString& sName)
  {
    auto byteData = data.ToByteArray().GetSubArray(uiOffset);

    return Stream(sName, Stream::Type::Float, byteData, sizeof(T));
  }
} // namespace ezExpression

class EZ_PROCGENPLUGIN_DLL ezExpressionVM
{
public:
  ezExpressionVM();
  ~ezExpressionVM();

  void RegisterFunction(const char* szName, ezExpressionFunction func,
    ezExpressionValidateGlobalData validationFunc = ezExpressionValidateGlobalData());

  void RegisterDefaultFunctions();

  ezResult Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezExpression::Stream> inputs, ezArrayPtr<ezExpression::Stream> outputs,
    ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData = ezExpression::GlobalData());

private:
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
