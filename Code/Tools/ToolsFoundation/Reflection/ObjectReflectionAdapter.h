#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Reflection/ReflectionAdapter.h>


class EZ_TOOLSFOUNDATION_DLL ezObjectSerializationContext : public ezReflectedSerializationContext
{
public:
  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pType) override;
  virtual void DeleteObject(const ezUuid& guid) override;
  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pType, void* pObject) override;
  virtual ezReflectedObjectWrapper* GetObjectByGUID(const ezUuid& guid) const override;
  virtual ezUuid GetObjectGUID(void* pObject) const override;

  virtual ezUuid EnqueObject(void* pObject, const ezRTTI* pType) override;
  virtual ezReflectedObjectWrapper DequeueObject() override;

private:
  ezHashTable<ezUuid, ezReflectedObjectWrapper> m_Objects;
  ezHashTable<void*, ezUuid> m_PtrLookup;
  ezSet<ezUuid> m_PendingObjects;
};


class EZ_TOOLSFOUNDATION_DLL ezObjectReflectionAdapter : public ezRttiAdapter
{
public:
  ezObjectReflectionAdapter(ezObjectSerializationContext* pContext);

  // Property Access
  virtual ezVariant GetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const override;
  virtual void SetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezVariant& value) override;

  virtual void GetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, ezReflectedObjectWrapper& value) const override;
  virtual void SetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezReflectedObjectWrapper& value) override {}

  virtual bool CanGetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const;
  virtual ezReflectedObjectWrapper GetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index);

  // Array
  virtual ezUInt32 GetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop) const override;
  virtual void SetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezUInt32 uiCount) override;

  // Set
  virtual void GetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezHybridArray<ezVariant, 16>& out_Keys) const override;
  virtual void SetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezHybridArray<ezVariant, 16>& keys) override;

  // Allocate
  virtual bool CanCreateObject(const ezRTTI* pType) override;
  virtual ezReflectedObjectWrapper CreateObject(const ezRTTI* pType) override;
  virtual void DeleteObject(ezReflectedObjectWrapper& object) override;
};
