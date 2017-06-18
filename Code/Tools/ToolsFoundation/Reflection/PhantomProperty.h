#pragma once

#include <Foundation/Reflection/Reflection.h>

struct ezReflectedPropertyDescriptor;

class ezPhantomConstantProperty : public ezAbstractConstantProperty
{
public:
  ezPhantomConstantProperty(const ezReflectedPropertyDescriptor* pDesc);
  ~ezPhantomConstantProperty();

  virtual const ezRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer() const override;
  virtual ezVariant GetConstant() const override { return m_Value; }

private:
  ezVariant m_Value;
  ezString m_sPropertyNameStorage;
  ezRTTI* m_pPropertyType;
};

class ezPhantomMemberProperty : public ezAbstractMemberProperty
{
public:
  ezPhantomMemberProperty(const ezReflectedPropertyDescriptor* pDesc);
  ~ezPhantomMemberProperty();

  virtual const ezRTTI* GetSpecificType() const override;
  virtual void* GetPropertyPointer(const void* pInstance) const override { return nullptr; }
  virtual void GetValuePtr(const void* pInstance, void* pObject) const override {}
  virtual void SetValuePtr(void* pInstance, void* pObject) override {}

private:
  ezString m_sPropertyNameStorage;
  ezRTTI* m_pPropertyType;
};

class ezPhantomFunctionProperty : public ezAbstractFunctionProperty
{
public:
  ezPhantomFunctionProperty(const ezReflectedPropertyDescriptor* pDesc);
  ~ezPhantomFunctionProperty();

  virtual const ezRTTI* GetSpecificType() const override;;
  virtual ezFunctionPropertyType::Enum GetFunctionType() const override;
  virtual const ezRTTI* GetReturnType() const override;
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override;
  virtual ezUInt32 GetArgumentCount() const override;
  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override;
  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override;
  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> values, ezVariant& returnValue) const override;

private:
  ezString m_sPropertyNameStorage;
  ezEnum<ezFunctionPropertyType> m_FunctionType;
  ezRTTI* m_pReturnType;
  ezBitflags<ezPropertyFlags> m_ReturnFlags;
  ezDynamicArray<ezRTTI*> m_ParameterTypes;
  ezDynamicArray<ezBitflags<ezPropertyFlags>> m_ParameterFlags;
};


class ezPhantomArrayProperty : public ezAbstractArrayProperty
{
public:
  ezPhantomArrayProperty(const ezReflectedPropertyDescriptor* pDesc);
  ~ezPhantomArrayProperty();

  virtual const ezRTTI* GetSpecificType() const override;
  virtual ezUInt32 GetCount(const void* pInstance) const override { return 0; }
  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const override {}
  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) override {}
  virtual void Insert(void* pInstance, ezUInt32 uiIndex, const void* pObject) override {}
  virtual void Remove(void* pInstance, ezUInt32 uiIndex) override {}
  virtual void Clear(void* pInstance) override {}
  virtual void SetCount(void* pInstance, ezUInt32 uiCount) override {}


private:
  ezString m_sPropertyNameStorage;
  ezRTTI* m_pPropertyType;
};


class ezPhantomSetProperty : public ezAbstractSetProperty
{
public:
  ezPhantomSetProperty(const ezReflectedPropertyDescriptor* pDesc);
  ~ezPhantomSetProperty();

  virtual const ezRTTI* GetSpecificType() const override;
  virtual bool IsEmpty(const void* pInstance) const override { return true; }
  virtual void Clear(void* pInstance) override {}
  virtual void Insert(void* pInstance, void* pObject) override {}
  virtual void Remove(void* pInstance, void* pObject) override {}
  virtual bool Contains(const void* pInstance, void* pObject) const override { return false; }
  virtual void GetValues(const void* pInstance, ezHybridArray<ezVariant, 16>& out_keys) const override {}

private:
  ezString m_sPropertyNameStorage;
  ezRTTI* m_pPropertyType;
};


class ezPhantomMapProperty : public ezAbstractMapProperty
{
public:
  ezPhantomMapProperty(const ezReflectedPropertyDescriptor* pDesc);
  ~ezPhantomMapProperty();

  virtual const ezRTTI* GetSpecificType() const override;
  virtual bool IsEmpty(const void* pInstance) const override { return true; }
  virtual void Clear(void* pInstance) override {}
  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) override {}
  virtual void Remove(void* pInstance, const char* szKey) override {}
  virtual bool Contains(const void* pInstance, const char* szKey) const override { return false; }
  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override { return false; }
  virtual const void* GetValue(const void* pInstance, const char* szKey) const override { return nullptr; }
  virtual void GetKeys(const void* pInstance, ezHybridArray<ezString, 16>& out_keys) const override {}

private:
  ezString m_sPropertyNameStorage;
  ezRTTI* m_pPropertyType;
};
