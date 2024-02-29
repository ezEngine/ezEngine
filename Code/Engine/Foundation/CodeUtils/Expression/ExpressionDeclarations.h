#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/Variant.h>

class ezStreamWriter;
class ezStreamReader;

namespace ezExpression
{
  struct Register
  {
    EZ_DECLARE_POD_TYPE();

    Register(){}; // NOLINT: using = default doesn't work here.

    union
    {
      ezSimdVec4b b;
      ezSimdVec4i i;
      ezSimdVec4f f;
    };
  };

  struct RegisterType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Unknown,

      Bool,
      Int,
      Float,

      Count,

      Default = Float,
      MaxNumBits = 4,
    };

    static const char* GetName(Enum registerType);
  };

  using Output = ezArrayPtr<Register>;
  using Inputs = ezArrayPtr<ezArrayPtr<const Register>>; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  using GlobalData = ezHashTable<ezHashedString, ezVariant>;

  /// \brief Describes an input or output stream for a expression VM
  struct StreamDesc
  {
    ezHashedString m_sName;
    ezProcessingStream::DataType m_DataType;

    bool operator==(const StreamDesc& other) const
    {
      return m_sName == other.m_sName && m_DataType == other.m_DataType;
    }

    ezResult Serialize(ezStreamWriter& inout_stream) const;
    ezResult Deserialize(ezStreamReader& inout_stream);
  };

  /// \brief Describes an expression function and its signature, e.g. how many input parameter it has and their type
  struct FunctionDesc
  {
    using TypeList = ezSmallArray<ezEnum<ezExpression::RegisterType>, 8>;

    ezHashedString m_sName;
    TypeList m_InputTypes;
    ezUInt8 m_uiNumRequiredInputs = 0;
    ezEnum<ezExpression::RegisterType> m_OutputType;

    bool operator==(const FunctionDesc& other) const
    {
      return m_sName == other.m_sName &&
             m_InputTypes == other.m_InputTypes &&
             m_uiNumRequiredInputs == other.m_uiNumRequiredInputs &&
             m_OutputType == other.m_OutputType;
    }

    bool operator<(const FunctionDesc& other) const;

    ezResult Serialize(ezStreamWriter& inout_stream) const;
    ezResult Deserialize(ezStreamReader& inout_stream);

    ezHashedString GetMangledName() const;
  };

  using Function = void (*)(ezExpression::Inputs, ezExpression::Output, const ezExpression::GlobalData&);
  using ValidateGlobalDataFunction = ezResult (*)(const ezExpression::GlobalData&);

} // namespace ezExpression

/// \brief Describes an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
struct ezExpressionFunction
{
  ezExpression::FunctionDesc m_Desc;

  ezExpression::Function m_Func;

  // Optional validation function used to validate required global data for an expression function
  ezExpression::ValidateGlobalDataFunction m_ValidateGlobalDataFunc;
};

struct EZ_FOUNDATION_DLL ezDefaultExpressionFunctions
{
  static ezExpressionFunction s_RandomFunc;
  static ezExpressionFunction s_PerlinNoiseFunc;
};

/// \brief Add this attribute a string property that should be interpreted as expression source.
///
/// The Inputs/Outputs property reference another array property on the same object that contains objects
/// with a name and a type property that can be used for real time error checking of the expression source.
class EZ_FOUNDATION_DLL ezExpressionWidgetAttribute : public ezTypeWidgetAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExpressionWidgetAttribute, ezTypeWidgetAttribute);

public:
  ezExpressionWidgetAttribute() = default;
  ezExpressionWidgetAttribute(const char* szInputsProperty, const char* szOutputProperty)
    : m_sInputsProperty(szInputsProperty)
    , m_sOutputsProperty(szOutputProperty)
  {
  }

  const char* GetInputsProperty() const { return m_sInputsProperty; }
  const char* GetOutputsProperty() const { return m_sOutputsProperty; }

private:
  ezUntrackedString m_sInputsProperty;
  ezUntrackedString m_sOutputsProperty;
};
