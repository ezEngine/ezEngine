#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <d3d11.h>

struct ID3D11Query;
class ezGALDeviceDX11;

/// \brief Pool for GPU queries.
class EZ_RENDERERDX11_DLL ezQueryPoolDX11
{
public:
  ezQueryPoolDX11(ezGALDeviceDX11* pDevice);

  /// \brief Initializes the pool.
  ezResult Initialize();
  void DeInitialize();

  void BeginFrame();
  void EndFrame();

  ezGALTimestampHandle InsertTimestamp();

  /// \brief Retrieves the timestamp value if it is available.
  /// \param hTimestamp The target timestamp to resolve.
  /// \param result The time of the timestamp. If this is empty on success the timestamp has expired.
  /// \return Returns false if the result is not available yet.
  ezEnum<ezGALAsyncResult> GetTimestampResult(ezGALTimestampHandle hTimestamp, ezTime& out_result);

  ezGALPoolHandle BeginOcclusionQuery(ezEnum<ezGALQueryType> type);
  void EndOcclusionQuery(ezGALPoolHandle hPool);
  ezEnum<ezGALAsyncResult> GetOcclusionQueryResult(ezGALPoolHandle hPool, ezUInt64& out_uiQueryResult);

private:
  static constexpr ezUInt32 s_uiRetainFrames = 4;
  static constexpr ezUInt64 s_uiPredicateFlag = EZ_BIT(19);
  static constexpr double s_fInvalid = -1.0;

  struct PerFrameData
  {
    ezGALFenceHandle m_hFence;
    ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = s_fInvalid;
    ezUInt32 m_uiReadyFrames = 0; ///< How many frames ago the m_pDisjointTimerQuery result was ready. Used to retain data for s_uiRetainFrames.
    ezUInt64 m_uiFrameCounter = ezUInt64(-1);
  };

private:
  PerFrameData GetFreeFrame();

private:
  ezGALDeviceDX11* m_pDevice = nullptr;

  struct Pool
  {
    Pool(ezAllocator* pAllocator);

    ezResult Initialize(ezGALDeviceDX11* pDevice, D3D11_QUERY queryType, ezUInt32 uiCount);
    void DeInitialize();

    ezGALPoolHandle CreateQuery();
    ID3D11Query* GetQuery(ezGALPoolHandle hPool);
    template <typename T>
    ezEnum<ezGALAsyncResult> GetResult(ezGALPoolHandle hPool, T& out_uiResult)
    {
      ID3D11Query* pQuery = GetQuery(hPool);
      HRESULT res = m_pDevice->GetDXImmediateContext()->GetData(pQuery, &out_uiResult, sizeof(out_uiResult), D3D11_ASYNC_GETDATA_DONOTFLUSH);
      if (res == S_FALSE)
      {
        return ezGALAsyncResult::Pending;
      }
      else if (res == S_OK)
      {
        return ezGALAsyncResult::Ready;
      }
      else
      {
        return ezGALAsyncResult::Expired;
      }
    }

    // #TODO_DX11 Replace ring buffer with proper pool like in Vulkan to prevent buffer overrun.
    ezDynamicArray<ID3D11Query*, ezLocalAllocatorWrapper> m_Queries;
    ezUInt32 m_uiNextTimestamp = 0;
    ezGALDeviceDX11* m_pDevice = nullptr;
  };

  // Pools
  Pool m_TimestampPool;
  Pool m_OcclusionPool;
  Pool m_OcclusionPredicatePool;

  // Disjoint timer and frame meta data needed for timestamps
  ezDeque<PerFrameData> m_PendingFrames;
  ezDeque<PerFrameData> m_FreeFrames;
  ezUInt64 m_uiFirstFrameIndex = 0;

  ezTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};
