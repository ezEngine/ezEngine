#include <UtilitiesPCH.h>

#include <Utilities/DataStructures/ObjectSelection.h>

ezObjectSelection::ezObjectSelection()
{
  m_pWorld = nullptr;
}

void ezObjectSelection::SetWorld(ezWorld* pWorld)
{
  EZ_ASSERT_DEV((m_pWorld == nullptr) || (m_pWorld == pWorld) || m_Objects.IsEmpty(), "You cannot change the world for this selection.");

  m_pWorld = pWorld;
}

void ezObjectSelection::RemoveDeadObjects()
{
  EZ_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  for (ezUInt32 i = m_Objects.GetCount(); i > 0; --i)
  {
    ezGameObject* pObject;
    if (!m_pWorld->TryGetObject(m_Objects[i - 1], pObject))
    {
      m_Objects.RemoveAtAndCopy(i - 1); // keep the order
    }
  }
}

void ezObjectSelection::AddObject(ezGameObjectHandle hObject, bool bDontAddTwice)
{
  EZ_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  // only insert valid objects
  ezGameObject* pObject;
  if (!m_pWorld->TryGetObject(hObject, pObject))
    return;

  if (m_Objects.IndexOf(hObject) != ezInvalidIndex)
    return;

  m_Objects.PushBack(hObject);
}

bool ezObjectSelection::RemoveObject(ezGameObjectHandle hObject)
{
  return m_Objects.RemoveAndCopy(hObject);
}

void ezObjectSelection::ToggleSelection(ezGameObjectHandle hObject)
{
  for (ezUInt32 i = 0; i < m_Objects.GetCount(); ++i)
  {
    if (m_Objects[i] == hObject)
    {
      m_Objects.RemoveAtAndCopy(i); // keep the order
      return;
    }
  }

  // ensures invalid objects don't get added
  AddObject(hObject);
}



EZ_STATICLINK_FILE(Utilities, Utilities_DataStructures_Implementation_ObjectSelection);

