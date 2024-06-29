#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <d3d11.h>

ezHybridArray<ID3D11Query*, 4> ezFencePoolDX11::s_Fences;
ezGALDeviceDX11* ezFencePoolDX11::s_pDevice;


void ezFencePoolDX11::Initialize(ezGALDeviceDX11* pDevice)
{
  s_pDevice = pDevice;
}

void ezFencePoolDX11::DeInitialize()
{
  for (ID3D11Query* pQuery : s_Fences)
  {
    EZ_GAL_DX11_RELEASE(pQuery);
  }
  s_Fences.Clear();
  s_Fences.Compact();

  s_pDevice = nullptr;
}

ID3D11Query* ezFencePoolDX11::RequestFence()
{
  EZ_ASSERT_DEBUG(s_pDevice, "ezFencePoolDX11::Initialize not called");
  if (!s_Fences.IsEmpty())
  {
    ID3D11Query* pFence = s_Fences.PeekBack();
    s_Fences.PopBack();
    return pFence;
  }
  else
  {
    ID3D11Query* pFence;
    D3D11_QUERY_DESC QueryDesc;
    QueryDesc.Query = D3D11_QUERY_EVENT;
    QueryDesc.MiscFlags = 0;
    HRESULT res = s_pDevice->GetDXDevice()->CreateQuery(&QueryDesc, &pFence);
    if (!SUCCEEDED(res))
    {
      EZ_REPORT_FAILURE("Failed to create fence: {}", ezArgErrorCode(res));
    }
    return pFence;
  }
}

void ezFencePoolDX11::ReclaimFence(ID3D11Query*& pFence)
{
  if (pFence)
  {
    EZ_ASSERT_DEBUG(s_pDevice, "ezFencePoolVulkan::Initialize not called");
    s_Fences.PushBack(pFence);
  }
  pFence = nullptr;
}

void ezFencePoolDX11::InsertFence(ID3D11Query* pFence)
{
  s_pDevice->GetDXImmediateContext()->End(pFence);
}

ezEnum<ezGALAsyncResult> ezFencePoolDX11::GetFenceResult(ID3D11Query* pFence, ezTime timeout)
{
  ezTimestamp start = ezTimestamp::CurrentTimestamp();
  do
  {
    BOOL data = FALSE;
    if (s_pDevice->GetDXImmediateContext()->GetData(pFence, &data, sizeof(data), 0) == S_OK)
    {
      EZ_ASSERT_DEV(data != FALSE, "Implementation error");
      return ezGALAsyncResult::Ready;
    }
    ezThreadUtils::YieldTimeSlice();
  } while ((ezTimestamp::CurrentTimestamp() - start) < timeout);

  return ezGALAsyncResult::Pending;
}


ezFenceQueueDX11::ezFenceQueueDX11(ezGALDeviceDX11* pDevice)
  : m_pDevice(pDevice)
{
}

ezFenceQueueDX11::~ezFenceQueueDX11()
{
  while (!m_PendingFences.IsEmpty())
  {
    WaitForNextFence(ezTime::MakeFromHours(1));
  }
}

ezGALFenceHandle ezFenceQueueDX11::GetCurrentFenceHandle()
{
  return m_uiCurrentFenceCounter;
}

ezGALFenceHandle ezFenceQueueDX11::SubmitCurrentFence()
{
  FlushReadyFences();
  ID3D11Query* pFence = ezFencePoolDX11::RequestFence();
  ezFencePoolDX11::InsertFence(pFence);

  m_PendingFences.PushBack({pFence, m_uiCurrentFenceCounter});
  ezGALFenceHandle hCurrent = m_uiCurrentFenceCounter;
  m_uiCurrentFenceCounter++;
  return hCurrent;
}

void ezFenceQueueDX11::FlushReadyFences()
{
  while (!m_PendingFences.IsEmpty())
  {
    if (WaitForNextFence() == ezGALAsyncResult::Pending)
      return;
  }
}

ezEnum<ezGALAsyncResult> ezFenceQueueDX11::GetFenceResult(ezGALFenceHandle hFence, ezTime timeout /*= ezTime::MakeZero()*/)
{
  if (hFence <= m_uiReachedFenceCounter)
    return ezGALAsyncResult::Ready;

  EZ_ASSERT_DEBUG(hFence <= m_uiCurrentFenceCounter, "Invalid fence handle");

  while (!m_PendingFences.IsEmpty() && m_PendingFences[0].m_hFence <= hFence)
  {
    ezTimestamp start = ezTimestamp::CurrentTimestamp();
    ezEnum<ezGALAsyncResult> res = WaitForNextFence(timeout);
    if (res == ezGALAsyncResult::Pending)
      return ezGALAsyncResult::Pending;

    ezTimestamp end = ezTimestamp::CurrentTimestamp();
    timeout -= (end - start);
  }

  return hFence <= m_uiReachedFenceCounter ? ezGALAsyncResult::Ready : ezGALAsyncResult::Pending;
}

ezEnum<ezGALAsyncResult> ezFenceQueueDX11::WaitForNextFence(ezTime timeout /*= ezTime::MakeZero()*/)
{
  ezEnum<ezGALAsyncResult> fenceStatus = ezFencePoolDX11::GetFenceResult(m_PendingFences[0].m_pFence, timeout);
  if (fenceStatus == ezGALAsyncResult::Ready)
  {
    m_uiReachedFenceCounter = m_PendingFences[0].m_hFence;
    ezFencePoolDX11::ReclaimFence(m_PendingFences[0].m_pFence);
    m_PendingFences.PopFront();
    return fenceStatus;
  }

  return fenceStatus;
}
