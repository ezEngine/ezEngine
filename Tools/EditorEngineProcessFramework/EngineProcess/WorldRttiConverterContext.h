#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>
#include <EditorEngineProcessFramework/EngineProcess/GuidHandleMap.h>
#include <Core/World/World.h>

/// \brief The world rtti converter context tracks created objects and is capable of also handling
///  components / game objects. Used by the ezIPCObjectMirror to create / destroy objects.
///
/// Atm it does not remove owner ptr when a parent is deleted, so it will accumulate zombie entries.
/// As requests to dead objects shouldn't generally happen this is for the time being not a problem.
class ezWorldRttiConverterContext : public ezRttiConverterContext
{
public:
  ezWorldRttiConverterContext() : m_pWorld(nullptr), m_uiNextComponentPickingID(1), m_uiHighlightID(1) {}

  virtual void Clear() override;
  void DeleteExistingObjects();

  virtual void* CreateObject(const ezUuid& guid, const ezRTTI* pRtti) override;
  virtual void DeleteObject(const ezUuid& guid) override;

  virtual void RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject) override;
  virtual void UnregisterObject(const ezUuid& guid) override;

  virtual ezRttiConverterObject GetObjectByGUID(const ezUuid& guid) const override;
  virtual ezUuid GetObjectGUID(const ezRTTI* pRtti, const void* pObject) const override;

  ezWorld* m_pWorld;
  ezEditorGuidEngineHandleMap<ezGameObjectHandle> m_GameObjectMap;
  ezEditorGuidEngineHandleMap<ezComponentHandle> m_ComponentMap;

  ezEditorGuidEngineHandleMap<ezUInt32> m_OtherPickingMap;
  ezEditorGuidEngineHandleMap<ezUInt32> m_ComponentPickingMap;
  ezUInt32 m_uiNextComponentPickingID;
  ezUInt32 m_uiHighlightID;
};
