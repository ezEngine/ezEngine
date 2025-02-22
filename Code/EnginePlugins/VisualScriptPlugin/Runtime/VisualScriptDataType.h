#pragma once

#include <Core/World/GameObject.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <VisualScriptPlugin/VisualScriptPluginDLL.h>

/// \brief Data types that are available in visual script. These are a subset of ezVariantType.
///
/// Like with ezVariantType, the order of these types is important as they are used to determine
/// if a type is "bigger" during type deduction. Also the enum values are serialized in visual script files.
struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptDataType
{
  using StorageType = ezUInt8;

  enum Enum : ezUInt8
  {
    Invalid = 0,

    Bool,
    Byte,
    Int,
    Int64,
    Float,
    Double,
    Color,
    Vector3,
    Quaternion,
    Transform,
    Time,
    Angle,
    String,
    HashedString,
    GameObject,
    Component,
    TypedPointer,
    Variant,
    Array,
    Map,
    Coroutine,

    Count,

    EnumValue,
    BitflagValue,

    ExtendedCount,

    AnyPointer = 0xFE,
    Any = 0xFF,

    Default = Invalid,
  };

  EZ_ALWAYS_INLINE static bool IsNumber(Enum dataType) { return dataType >= Bool && dataType <= Double; }
  EZ_ALWAYS_INLINE static bool IsPointer(Enum dataType) { return (dataType >= GameObject && dataType <= TypedPointer) || dataType == Coroutine; }

  static ezVariantType::Enum GetVariantType(Enum dataType);
  static Enum FromVariantType(ezVariantType::Enum variantType);

  static ezProcessingStream::DataType GetStreamDataType(Enum dataType);

  static const ezRTTI* GetRtti(Enum dataType);
  static Enum FromRtti(const ezRTTI* pRtti);

  static ezUInt32 GetStorageSize(Enum dataType);
  static ezUInt32 GetStorageAlignment(Enum dataType);

  static const char* GetName(Enum dataType);

  static bool CanConvertTo(Enum sourceDataType, Enum targetDataType);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_VISUALSCRIPTPLUGIN_DLL, ezVisualScriptDataType);

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptGameObjectHandle
{
  ezGameObjectHandle m_Handle;
  mutable ezGameObject* m_Ptr;
  mutable ezUInt32 m_uiExecutionCounter;

  void AssignHandle(const ezGameObjectHandle& hObject)
  {
    m_Handle = hObject;
    m_Ptr = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(ezGameObject* pObject, ezUInt32 uiExecutionCounter)
  {
    m_Handle = pObject != nullptr ? pObject->GetHandle() : ezGameObjectHandle();
    m_Ptr = pObject;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  ezGameObject* GetPtr(ezUInt32 uiExecutionCounter) const;
};

struct EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptComponentHandle
{
  ezComponentHandle m_Handle;
  mutable ezComponent* m_Ptr;
  mutable ezUInt32 m_uiExecutionCounter;

  void AssignHandle(const ezComponentHandle& hComponent)
  {
    m_Handle = hComponent;
    m_Ptr = nullptr;
    m_uiExecutionCounter = 0;
  }

  void AssignPtr(ezComponent* pComponent, ezUInt32 uiExecutionCounter)
  {
    m_Handle = pComponent != nullptr ? pComponent->GetHandle() : ezComponentHandle();
    m_Ptr = pComponent;
    m_uiExecutionCounter = uiExecutionCounter;
  }

  ezComponent* GetPtr(ezUInt32 uiExecutionCounter) const;
};
