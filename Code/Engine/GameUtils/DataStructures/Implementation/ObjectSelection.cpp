#include <GameUtils/PCH.h>
#include <GameUtils/DataStructures/ObjectSelection.h>

ezObjectSelection::ezObjectSelection()
{
  m_pWorld = NULL;
}

void ezObjectSelection::SetWorld(ezWorld* pWorld)
{
  EZ_ASSERT((m_pWorld == NULL) || (m_pWorld == pWorld) || m_Objects.IsEmpty(), "You cannot change the world for this selection.");

  m_pWorld = pWorld;
}

void ezObjectSelection::RemoveDeadObjects()
{
  EZ_ASSERT(m_pWorld != NULL, "The world has not been set.");

  for (ezUInt32 i = m_Objects.GetCount(); i > 0; )
  {
    ezGameObject* pObject;
    if (!m_pWorld->TryGetObject(m_Objects[i - 1], pObject))
    {
      m_Objects.RemoveAt(i - 1); // keep the order
    }
    else
       --i;
  }
}

void ezObjectSelection::AddObject(ezGameObjectHandle hObject)
{
  EZ_ASSERT(m_pWorld != NULL, "The world has not been set.");

  // only insert valid objects
  ezGameObject* pObject;
  if (!m_pWorld->TryGetObject(hObject, pObject))
    return;

  m_Objects.PushBack(hObject);
}

void ezObjectSelection::RemoveObject(ezGameObjectHandle hObject)
{
  m_Objects.Remove(hObject);
}

void ezObjectSelection::ToggleSelection(ezGameObjectHandle hObject)
{
  for (ezUInt32 i = 0; i < m_Objects.GetCount(); ++i)
  {
    if (m_Objects[i] == hObject)
    {
      m_Objects.RemoveAt(i); // keep the order
      return;
    }
  }

  // ensures invalid objects don't get added
  AddObject(hObject);
}



EZ_STATICLINK_FILE(GameUtils, GameUtils_DataStructures_Implementation_ObjectSelection);

