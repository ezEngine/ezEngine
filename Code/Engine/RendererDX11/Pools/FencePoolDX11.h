#pragma once

#include <RendererDX11/RendererDX11DLL.h>

#include <Foundation/Time/Timestamp.h>

struct ID3D11Query;
class ezGALDeviceDX11;


class EZ_RENDERERDX11_DLL ezFencePoolDX11
{
public:
  static void Initialize(ezGALDeviceDX11* pDevice);
  static void DeInitialize();

  static ID3D11Query* RequestFence();
  static void ReclaimFence(ID3D11Query*& ref_pFence);

  static void InsertFence(ID3D11Query* pFence);
  static ezEnum<ezGALAsyncResult> GetFenceResult(ID3D11Query* pFence, ezTime timeout = ezTime::MakeZero());

private:
  static ezHybridArray<ID3D11Query*, 4> s_Fences;
  static ezGALDeviceDX11* s_pDevice;
};


class EZ_RENDERERDX11_DLL ezFenceQueueDX11
{
public:
  ezFenceQueueDX11();
  ~ezFenceQueueDX11();

  ezGALFenceHandle GetCurrentFenceHandle();
  ezGALFenceHandle SubmitCurrentFence();
  ezEnum<ezGALAsyncResult> GetFenceResult(ezGALFenceHandle hFence, ezTime timeout = ezTime::MakeZero());

private:
  void FlushReadyFences();
  ezEnum<ezGALAsyncResult> WaitForNextFence(ezTime timeout = ezTime::MakeZero());

private:
  struct PendingFence
  {
    ID3D11Query* m_pFence = nullptr;
    ezGALFenceHandle m_hFence = {};
  };
  ezDeque<PendingFence> m_PendingFences;
  ezUInt64 m_uiCurrentFenceCounter = 1;
  ezUInt64 m_uiReachedFenceCounter = 0;
};
