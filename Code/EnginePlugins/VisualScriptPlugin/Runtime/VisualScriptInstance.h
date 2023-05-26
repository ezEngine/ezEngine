#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Containers/Blob.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptInstance : public ezScriptInstance
{
public:
  ezVisualScriptInstance(ezReflectedClass& owner, ezWorld* pWorld, const ezSharedPtr<const ezVisualScriptDataStorage>& pConstantDataStorage, const ezSharedPtr<const ezVisualScriptDataDescription>& pVariableDataDesc);

  virtual void ApplyParameters(const ezArrayMap<ezHashedString, ezVariant>& parameters) override;

  ezReflectedClass& GetOwner() { return m_Owner; }
  ezWorld* GetWorld() { return m_pWorld; }

  using DataOffset = ezVisualScriptNodeDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  ezTypedPointer GetPointerData(DataOffset dataOffset);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const ezRTTI* pType = nullptr);

  ezVariant GetDataAsVariant(DataOffset dataOffset, ezVariantType::Enum expectedType) const;
  void SetDataFromVariant(DataOffset dataOffset, const ezVariant& value);

  ezUInt32 GetExecutionCounter() const { return m_uiExecutionCounter; }

private:
  ezReflectedClass& m_Owner;
  ezWorld* m_pWorld = nullptr;

  ezSharedPtr<const ezVisualScriptDataStorage> m_pConstantDataStorage;
  ezUniquePtr<ezVisualScriptDataStorage> m_pVariableDataStorage;

  friend class ezVisualScriptExecutionContext;
  ezUInt32 m_uiExecutionCounter = 0;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptExecutionContext
{
public:
  ezVisualScriptExecutionContext(ezUniquePtr<ezVisualScriptGraphDescription>&& pDesc);

  ezResult Initialize(ezVisualScriptInstance& instance, ezArrayPtr<ezVariant> arguments, ezVariant& returnValue);

  using ReturnValue = ezVisualScriptGraphDescription::ReturnValue;
  ReturnValue::Enum Execute();

private:
  ezUniquePtr<ezVisualScriptGraphDescription> m_pDesc;
  ezVisualScriptInstance* m_pInstance = nullptr;
  ezUInt32 m_uiCurrentNode = 0;
};

class EZ_VISUALSCRIPTPLUGIN_DLL ezVisualScriptFunctionProperty : public ezAbstractFunctionProperty
{
public:
  ezVisualScriptFunctionProperty(const char* szPropertyName, ezUniquePtr<ezVisualScriptGraphDescription>&& pDesc);
  ~ezVisualScriptFunctionProperty();

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Member; }
  virtual const ezRTTI* GetReturnType() const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override { return ezPropertyFlags::Void; }
  virtual ezUInt32 GetArgumentCount() const override { return 0; }
  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override { return nullptr; }
  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override { return ezPropertyFlags::Void; }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& returnValue) const override;

private:
  ezHashedString m_sPropertyNameStorage;
  mutable ezVisualScriptExecutionContext m_ExecutionContext;
};

#include <VisualScriptPlugin/Runtime/VisualScriptInstance_inl.h>
