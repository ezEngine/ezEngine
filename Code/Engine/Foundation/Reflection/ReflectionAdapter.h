#pragma once

#include <Foundation/Reflection/Reflection.h>

struct EZ_FOUNDATION_DLL ezReflectedObjectWrapper
{
  ezReflectedObjectWrapper() : m_pType(nullptr), m_pObject(nullptr) {}
  ezReflectedObjectWrapper(const ezRTTI* pType, void* pObject) : m_pType(pType), m_pObject(pObject) {}

  EZ_DECLARE_POD_TYPE();

  const ezRTTI* m_pType;
  void* m_pObject;
};


struct EZ_FOUNDATION_DLL ezReflectedTypeWrapper
{
  EZ_DECLARE_POD_TYPE();

  const char* m_szName;
  ezBitflags<ezTypeFlags> m_Flags;
  const ezRTTI* m_pType;
};


struct EZ_FOUNDATION_DLL ezReflectedPropertyWrapper
{
  EZ_DECLARE_POD_TYPE();

  const char* m_szName;
  ezEnum<ezPropertyCategory> m_Category;
  ezBitflags<ezPropertyFlags> m_Flags;
  const ezRTTI* m_pType;
  void* m_pProperty;
};


class EZ_FOUNDATION_DLL ezReflectedSerializationContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) = 0;
  virtual void DeleteObject(const ezUuid& guid) = 0;
  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject) = 0;
  virtual ezReflectedObjectWrapper* GetObjectByGUID(const ezUuid& guid) const = 0;
  virtual ezUuid GetObjectGUID(void* pObject) const = 0;

  virtual ezUuid EnqueObject(void* pObject, const ezRTTI* pRtti) = 0;
  virtual ezReflectedObjectWrapper DequeueObject() = 0;
};


class EZ_FOUNDATION_DLL ezRttiSerializationContext : public ezReflectedSerializationContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override;
  virtual void DeleteObject(const ezUuid& guid) override;
  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject) override;
  virtual ezReflectedObjectWrapper* GetObjectByGUID(const ezUuid& guid) const override;
  virtual ezUuid GetObjectGUID(void* pObject) const override;

  virtual ezUuid EnqueObject(void* pObject, const ezRTTI* pRtti) override;
  virtual ezReflectedObjectWrapper DequeueObject() override;

private:
  ezHashTable<ezUuid, ezReflectedObjectWrapper> m_Objects;
  ezHashTable<void*, ezUuid> m_PtrLookup;
  ezSet<ezUuid> m_PendingObjects;
};


class EZ_FOUNDATION_DLL ezReflectionAdapter
{
public:
  ezReflectionAdapter(ezReflectedSerializationContext* pContext) : m_pContext(pContext) {}
  ezReflectedSerializationContext* GetContext() { return m_pContext; }

  // Type Info
  virtual ezReflectedTypeWrapper GetTypeInfo(const ezRTTI* pRtti) const = 0;
  virtual const ezRTTI* FindTypeByName(const char* szTypeName) const = 0;
  virtual const ezRTTI* GetParentType(const ezRTTI* pRtti) const = 0;
  virtual bool IsStandardType(const ezRTTI* pRtti) const = 0;

  // Property Info
  virtual ezReflectedPropertyWrapper GetPropertyInfo(void* pProp) const = 0;
  virtual ezUInt32 GetPropertyCount(const ezRTTI* pRtti) const = 0;
  virtual void* GetProperty(const ezRTTI* pRtti, ezUInt32 uiIndex) const = 0;
  virtual void* FindPropertyByName(const ezRTTI* pRtti, const char* szPropName) const = 0;

  // Property Access
  virtual ezVariant GetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const = 0;
  virtual void SetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezVariant& value) = 0;

  virtual void GetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, ezReflectedObjectWrapper& value) const = 0;
  virtual void SetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezReflectedObjectWrapper& value) = 0;

  virtual bool CanGetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const = 0;
  virtual ezReflectedObjectWrapper GetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) = 0;

  // Array
  virtual ezUInt32 GetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop) const = 0;
  virtual void SetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezUInt32 uiCount) = 0;

  // Set
  virtual void GetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezHybridArray<ezVariant, 16>& out_Keys) const = 0;
  virtual void SetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezHybridArray<ezVariant, 16>& keys) = 0;

  // Allocate
  virtual bool CanCreateObject(const ezRTTI* pRtti) = 0;
  virtual ezReflectedObjectWrapper CreateObject(const ezRTTI* pRtti) = 0;
  virtual void DeleteObject(ezReflectedObjectWrapper& object) = 0;

protected:
  ezReflectedSerializationContext* m_pContext;
};


class EZ_FOUNDATION_DLL ezRttiAdapter : public ezReflectionAdapter
{
public:
  ezRttiAdapter(ezReflectedSerializationContext* pContext);

  // Type Info
  virtual ezReflectedTypeWrapper GetTypeInfo(const ezRTTI* pRtti) const override;
  virtual const ezRTTI* FindTypeByName(const char* szTypeName) const override;
  virtual const ezRTTI* GetParentType(const ezRTTI* pRtti) const override;
  virtual bool IsStandardType(const ezRTTI* pRtti) const override;

  // Property Info
  virtual ezReflectedPropertyWrapper GetPropertyInfo(void* pPropm) const override;
  virtual ezUInt32 GetPropertyCount(const ezRTTI* pRtti) const override;
  virtual void* GetProperty(const ezRTTI* pRtti, ezUInt32 uiIndex) const override;
  virtual void* FindPropertyByName(const ezRTTI* pRtti, const char* szPropName) const override;

  // Property Access
  virtual ezVariant GetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const override;
  virtual void SetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezVariant& value) override;

  virtual void GetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, ezReflectedObjectWrapper& value) const override;
  virtual void SetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezReflectedObjectWrapper& value) override;

  virtual bool CanGetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const;
  virtual ezReflectedObjectWrapper GetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index);

  // Array
  virtual ezUInt32 GetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop) const override;
  virtual void SetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezUInt32 uiCount) override;

  // Set
  virtual void GetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezHybridArray<ezVariant, 16>& out_Keys) const override;
  virtual void SetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezHybridArray<ezVariant, 16>& keys) override;

  // Allocate
  virtual bool CanCreateObject(const ezRTTI* pRtti) override;
  virtual ezReflectedObjectWrapper CreateObject(const ezRTTI* pRtti) override;
  virtual void DeleteObject(ezReflectedObjectWrapper& object) override;
};
