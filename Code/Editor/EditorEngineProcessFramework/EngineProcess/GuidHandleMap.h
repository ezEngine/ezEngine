#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

template <typename HandleType>
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
    auto it = m_GuidToHandle.Find(guid);
    if (it.IsValid())
    {
      // During undo/redo we may register the same object again. In that case, just use the new version.
      UnregisterObject(guid);
    }
    m_GuidToHandle[guid] = handle;
    m_HandleToGuid[handle] = guid;

    EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(ezUuid guid)
  {
    const HandleType handle = m_GuidToHandle[guid];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  void UnregisterObject(HandleType handle)
  {
    const ezUuid guid = m_HandleToGuid[handle];
    m_GuidToHandle.Remove(guid);
    m_HandleToGuid.Remove(handle);

    EZ_ASSERT_DEV(m_GuidToHandle.GetCount() == m_HandleToGuid.GetCount(), "1:1 relationship is broken. Check operator< for handle type.");
  }

  HandleType GetHandle(ezUuid guid) const
  {
    HandleType res = HandleType();
    m_GuidToHandle.TryGetValue(guid, res);
    return res;
  }

  ezUuid GetGuid(HandleType handle) const { return m_HandleToGuid.GetValueOrDefault(handle, ezUuid()); }

  const ezMap<HandleType, ezUuid>& GetHandleToGuidMap() const { return m_HandleToGuid; }

private:
  ezHashTable<ezUuid, HandleType> m_GuidToHandle;
  ezMap<HandleType, ezUuid> m_HandleToGuid;
};
