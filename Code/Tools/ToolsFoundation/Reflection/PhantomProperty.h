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

  virtual const ezRTTI* GetSpecificType() const override { return nullptr; };

  virtual void Execute(void* pInstance) const
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

private:
  ezString m_sPropertyNameStorage;
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
  virtual void Insert(void* pInstance, ezUInt32 uiIndex, void* pObject) override {}
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
  virtual ezUInt32 GetCount(const void* pInstance) const override { return 0; }
  virtual void Clear(void* pInstance) override {}
  virtual void Insert(void* pInstance, void* pObject) override {}
  virtual void Remove(void* pInstance, void* pObject) override {}
  virtual bool Contains(const void* pInstance, void* pObject) const override { return false; }
  virtual void GetValues(const void* pInstance, ezHybridArray<ezVariant, 16>& out_keys) const override {}

private:
  ezString m_sPropertyNameStorage;
  ezRTTI* m_pPropertyType;
};

