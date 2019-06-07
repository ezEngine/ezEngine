#pragma once

#include <ProcGenPlugin/VM/ExpressionFunctions.h>
#include <Foundation/Containers/DynamicArray.h>

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
}

class EZ_PROCGENPLUGIN_DLL ezExpressionVM
{
public:
  ezExpressionVM(ezUInt32 uiTempRegistersInBytes = 256 * 1024);
  ~ezExpressionVM();

  void Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezExpression::Stream> inputs, ezArrayPtr<ezExpression::Stream> outputs, ezUInt32 uiNumInstances);

private:
  ezDynamicArray<ezSimdVec4f, ezAlignedAllocatorWrapper> m_Registers;

  ezDynamicArray<ezUInt32> m_InputMapping;
  ezDynamicArray<ezUInt32> m_OutputMapping;
};

