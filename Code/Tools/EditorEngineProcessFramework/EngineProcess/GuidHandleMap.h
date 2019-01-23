#pragma once

#include <EditorEngineProcessFramework/Plugin.h>

template<typename HandleType>
class ezEditorGuidEngineHandleMap
{
public:
  void Clear()
  {
    m_GuidToHandle.Clear();
    m_HandleToGuid.Clear();
  }

  void RegisterObject(ezUuid guid, HandleType handle)
  {
    m_GuidToHandle[guid] = handle;
    m_HandleToGuid[handle] = guid;

    // apparently this happens during undo/redo (same guid, new handle on undo)
    //EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(ezUuid guid)
  {
    const HandleType handle = m_GuidToHandle[guid];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    // apparently this happens during undo/redo (same guid, new handle on undo)
    //EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(HandleType handle)
  {
    const ezUuid guid = m_HandleToGuid[handle];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    // apparently this happens during undo/redo (same guid, new handle on undo)
    //EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  HandleType GetHandle(ezUuid guid) const
  {
    HandleType res = HandleType();
    m_GuidToHandle.TryGetValue(guid, res);
    return res;
  }

  ezUuid GetGuid(HandleType handle) const
  {
    return m_HandleToGuid.GetValueOrDefault(handle, ezUuid());
  }

  const ezMap<HandleType, ezUuid>& GetHandleToGuidMap() const
  {
    return m_HandleToGuid;
  }

private:
  ezHashTable<ezUuid, HandleType> m_GuidToHandle;
  ezMap<HandleType, ezUuid> m_HandleToGuid;
};