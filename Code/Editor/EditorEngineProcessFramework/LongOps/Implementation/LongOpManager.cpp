#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

void ezLongOpManager::Startup(ezProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(
    ezMakeDelegate(&ezLongOpManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void ezLongOpManager::Shutdown()
{
  EZ_LOCK(m_Mutex);

  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}
