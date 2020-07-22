﻿#includde < WindowsMixedRealityPCH.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/SpatialMapping/SurfaceReconstructionMeshManager.h>
#include <WindowsMixedReality/SpatialReferenceFrame.h>

// Warning	C4467	usage of ATL attributes is deprecated
#define EZ_MSVC_WARNING_NUMBER 4467
#include <Foundation/Basics/Compiler/DisableWarning.h>
#include <Foundation/Basics/Compiler/RestoreWarning.h>
#include <robuffer.h>

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Perception::Spatial;
using namespace ABI::Windows::Graphics::DirectX;
using namespace ABI::Windows::Storage::Streams;

EZ_IMPLEMENT_SINGLETON(ezSurfaceReconstructionMeshManager);

ezSurfaceReconstructionMeshManager::ezSurfaceReconstructionMeshManager()
  : m_SingletonRegistrar(this)
{
  RequestSpatialMappingAccess();
}

ezSurfaceReconstructionMeshManager::~ezSurfaceReconstructionMeshManager() {}

// static
void ezSurfaceReconstructionMeshManager::LogSupportedFormats()
{
  EZ_LOG_BLOCK("Supported DX formats for Surface Reconstruction Mesh");

  ComPtr<Surfaces::ISpatialSurfaceMeshOptionsStatics> pMeshOptionStatics;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_Surfaces_SpatialSurfaceMeshOptions, pMeshOptionStatics);

  ComPtr<IVectorView<DirectXPixelFormat>> supportedVertexFormats;
  pMeshOptionStatics->get_SupportedVertexPositionFormats(&supportedVertexFormats);

  ComPtr<IVectorView<DirectXPixelFormat>> supportedIndexFormats;
  pMeshOptionStatics->get_SupportedTriangleIndexFormats(&supportedIndexFormats);

  ComPtr<IVectorView<DirectXPixelFormat>> supportedNormalFormats;
  pMeshOptionStatics->get_SupportedVertexNormalFormats(&supportedNormalFormats);

  ezUwpUtils::ezWinRtIterateIVectorView<DirectXPixelFormat>(supportedVertexFormats, [](UINT index, const DirectXPixelFormat& format) {
    ezLog::Info("Vertex Format: {0}", format);
    return true;
  });

  ezUwpUtils::ezWinRtIterateIVectorView<DirectXPixelFormat>(supportedNormalFormats, [](UINT index, const DirectXPixelFormat& format) {
    ezLog::Info("Normal Format: {0}", format);
    return true;
  });

  ezUwpUtils::ezWinRtIterateIVectorView<DirectXPixelFormat>(supportedIndexFormats, [](UINT index, const DirectXPixelFormat& format) {
    ezLog::Info("Index Format: {0}", format);
    return true;
  });
}

void ezSurfaceReconstructionMeshManager::RequestSpatialMappingAccess()
{
  ezLog::Info("Requesting spatial mapping access");

  ComPtr<Surfaces::ISpatialSurfaceObserverStatics> pSurfaceObserverStatics;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_Surfaces_SpatialSurfaceObserver, pSurfaceObserverStatics);

  // TODO: check this first
  // using namespace Windows::Foundation::Metadata;
  // if (ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 4))
  //{
  //  if (!SpatialSurfaceObserver::IsSupported())
  //  {

  //  }
  //}

  ComPtr<__FIAsyncOperation_1_Windows__CPerception__CSpatial__CSpatialPerceptionAccessStatus> status;
  if (SUCCEEDED(pSurfaceObserverStatics->RequestAccessAsync(&status)))
  {

    ezUwpUtils::ezWinRtPutCompleted<SpatialPerceptionAccessStatus, SpatialPerceptionAccessStatus>(
      status, [this](SpatialPerceptionAccessStatus result) {
        switch (result)
        {
          case SpatialPerceptionAccessStatus_Allowed:

            m_SrmAvailability = SrmAvailability::Available;
            ezLog::Info("Spatial Surfaces Access available.");
            break;

          case SpatialPerceptionAccessStatus_DeniedBySystem:
            m_SrmAvailability = SrmAvailability::NotAvailable;
            ezLog::Error(
              "Spatial Surface Access denied by System. This usually means the app does not contain the necessary capability flag in its manifest.");
            break;

          case SpatialPerceptionAccessStatus_DeniedByUser:
            m_SrmAvailability = SrmAvailability::NotAvailable;
            ezLog::Error("Spatial Surface Access denied by User.");
            break;

          case SpatialPerceptionAccessStatus_Unspecified:
            m_SrmAvailability = SrmAvailability::NotAvailable;
            ezLog::Error("Spatial Surface Access unspecified.");
            break;
        }

        ezSrmManagerEvent e;
        e.m_pManager = this;
        e.m_Type = ezSrmManagerEvent::Type::AvailabilityChanged;
        m_Events.Broadcast(e);

        if (m_SrmAvailability == SrmAvailability::Available)
        {
          CreateSurfaceObserver();
        }
      });
  }
}

void ezSurfaceReconstructionMeshManager::CreateSurfaceObserver()
{
  EZ_ASSERT_DEV(m_pSurfaceObserver == nullptr, "Surface Observer is already created");

  auto pHoloSpace = ezWindowsHolographicSpace::GetSingleton();

  ezUwpUtils::CreateInstance(RuntimeClass_Windows_Perception_Spatial_Surfaces_SpatialSurfaceObserver, m_pSurfaceObserver);

  ComPtr<ISpatialBoundingVolumeStatics> volumeStatics;
  ezUwpUtils::RetrieveStatics(RuntimeClass_Windows_Perception_Spatial_SpatialBoundingVolume, volumeStatics);

  /// \todo Make this configurable
  SpatialBoundingBox box;
  ezUwpUtils::ConvertVec3(ezVec3(0), box.Center);
  ezUwpUtils::ConvertVec3(ezVec3(5), box.Extents);

  ComPtr<ISpatialCoordinateSystem> pReferenceCoords;
  pHoloSpace->GetDefaultReferenceFrame()->GetInternalCoordinateSystem(pReferenceCoords);

  ComPtr<ISpatialBoundingVolume> boxVolume;
  volumeStatics->FromBox(pReferenceCoords.Get(), box, &boxVolume);

  m_pSurfaceObserver->SetBoundingVolume(boxVolume.Get());
}

void ezSurfaceReconstructionMeshManager::PullCurrentSurfaces()
{
  if (m_pSurfaceObserver == nullptr)
    return;

  // tag all surfaces as 'unobserved'
  {
    EZ_LOCK(m_Mutex);

    for (auto it = m_Surfaces.GetIterator(); it.IsValid(); ++it)
    {
      // tag all surfaces as 'removed'
      it.Value().m_bIsObserved = false;
    }
  }

  ComPtr<__FIMapView_2_GUID_Windows__CPerception__CSpatial__CSurfaces__CSpatialSurfaceInfo> surfaceMap;
  m_pSurfaceObserver->GetObservedSurfaces(&surfaceMap);

  using ValueInterfaceType = Surfaces::ISpatialSurfaceInfo*;

  ezUwpUtils::ezWinRtIterateIMapView<ValueInterfaceType>(surfaceMap, [this](const GUID& key, const ValueInterfaceType& pSurfaceInfo) -> bool {
    UpdateSurface(ezUwpUtils::ConvertGuid(key), pSurfaceInfo);
    return true;
  });

  ClearUnobservedSurfaces();
}

const ezMap<ezUuid, ezSurfaceMeshInfo>& ezSurfaceReconstructionMeshManager::BeginAccessingSurfaces()
{
  m_Mutex.Acquire();
  return m_Surfaces;
}

void ezSurfaceReconstructionMeshManager::EndAccessingSurfaces()
{
  m_Mutex.Release();
}

void ezSurfaceReconstructionMeshManager::UpdateSurface(const ezUuid& guid, Surfaces::ISpatialSurfaceInfo* pSurfaceInfo)
{
  DateTime lastUpdate;
  pSurfaceInfo->get_UpdateTime(&lastUpdate);

  {
    EZ_LOCK(m_Mutex);

    auto& surface = m_Surfaces[guid];
    surface.m_bIsObserved = true;
    if (surface.m_iLastUpdate == lastUpdate.UniversalTime)
      return;
  }

  ComPtr<Surfaces::ISpatialSurfaceMeshOptions> pMeshOptions;
  ezUwpUtils::CreateInstance(RuntimeClass_Windows_Perception_Spatial_Surfaces_SpatialSurfaceMeshOptions, pMeshOptions);

  /// \todo Support normals
  // pMeshOptions->put_IncludeVertexNormals(TRUE);
  // pMeshOptions->put_VertexNormalFormat();
  pMeshOptions->put_TriangleIndexFormat(DirectXPixelFormat_R16UInt);
  pMeshOptions->put_VertexPositionFormat(DirectXPixelFormat_R32G32B32A32Float);

  /// \todo Make detail setting configurable
  ComPtr<IAsyncOperation<Surfaces::SpatialSurfaceMesh*>> pAsyncComputeMesh;
  pSurfaceInfo->TryComputeLatestMeshWithOptionsAsync(200, pMeshOptions.Get(), &pAsyncComputeMesh);

  ezUwpUtils::ezWinRtPutCompleted<Surfaces::SpatialSurfaceMesh*, ComPtr<Surfaces::ISpatialSurfaceMesh>>(
    pAsyncComputeMesh, [this, guid](const ComPtr<Surfaces::ISpatialSurfaceMesh>& pMesh) { UpdateSurfaceMesh(guid, pMesh.Get()); });
}

static const ezUInt8* GetBytePointer(const ComPtr<Surfaces::ISpatialSurfaceMeshBuffer>& pMeshBuffer, ezUInt32 uiNumElements,
  DirectXPixelFormat expectedFormat, ezUInt32 uiExpectedStride)
{
  DirectXPixelFormat indexFormat;
  pMeshBuffer->get_Format(&indexFormat);

  UINT32 uiStride = 0;
  pMeshBuffer->get_Stride(&uiStride);

  ComPtr<IBuffer> pBuffer;
  pMeshBuffer->get_Data(&pBuffer);

  UINT32 uiDataLength = 0;
  pBuffer->get_Length(&uiDataLength);

  EZ_ASSERT_DEBUG(indexFormat == expectedFormat, "Incorrect format");
  EZ_ASSERT_DEBUG(uiStride == uiExpectedStride, "Incorrect stride");
  EZ_ASSERT_DEBUG(uiDataLength >= uiNumElements * uiExpectedStride, "Unexpected size");

  ComPtr<Windows::Storage::Streams::IBufferByteAccess> pByteBuffer;
  pBuffer->QueryInterface<Windows::Storage::Streams::IBufferByteAccess>(&pByteBuffer);

  byte* pBytes = nullptr;
  pByteBuffer->Buffer(&pBytes);

  return reinterpret_cast<const ezUInt8*>(pBytes);
}

void ezSurfaceReconstructionMeshManager::UpdateSurfaceMesh(const ezUuid& guid, Surfaces::ISpatialSurfaceMesh* pMesh)
{
  EZ_LOCK(m_Mutex);

  if (pMesh == nullptr)
  {
    ClearSurfaceMesh(guid);
    return;
  }

  auto& surface = m_Surfaces[guid];
  surface.m_MeshGuid = guid;
  surface.m_Transform.SetIdentity();

  // set last update time
  {
    ComPtr<Surfaces::ISpatialSurfaceInfo> pSurfaceInfo;
    pMesh->get_SurfaceInfo(&pSurfaceInfo);

    DateTime lastUpdate;
    pSurfaceInfo->get_UpdateTime(&lastUpdate);
    surface.m_iLastUpdate = lastUpdate.UniversalTime;
  }

  ezMeshBufferResourceDescriptor& mb = surface.m_MeshData;
  if (UpdateMeshData(mb, pMesh).Failed())
  {
    ClearSurfaceMesh(guid);
    return;
  }

  {
    ezSrmManagerEvent e;
    e.m_pManager = this;
    e.m_Type = ezSrmManagerEvent::Type::MeshUpdated;
    e.m_MeshGuid = guid;
    m_Events.Broadcast(e);
  }
}

ezResult ezSurfaceReconstructionMeshManager::UpdateMeshData(ezMeshBufferResourceDescriptor& mb, Surfaces::ISpatialSurfaceMesh* pMesh)
{
  mb.Clear();

  mb.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
  mb.AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);

  ComPtr<Surfaces::ISpatialSurfaceMeshBuffer> pVertexData;
  pMesh->get_VertexPositions(&pVertexData);

  ComPtr<Surfaces::ISpatialSurfaceMeshBuffer> pIndexData;
  pMesh->get_TriangleIndices(&pIndexData);

  ComPtr<ISpatialCoordinateSystem> pCoordSys;
  pMesh->get_CoordinateSystem(&pCoordSys);

  ComPtr<ISpatialCoordinateSystem> pRefFrame;
  ezWindowsHolographicSpace::GetSingleton()->GetDefaultReferenceFrame()->GetInternalCoordinateSystem(pRefFrame);

  ComPtr<__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4> pRelMat;
  pCoordSys->TryGetTransformTo(pRefFrame.Get(), &pRelMat);

  if (pRelMat == nullptr)
    return EZ_FAILURE;

  const ezMat4 relMat = ezUwpUtils::ConvertMat4(pRelMat.Get());

  Numerics::Vector3 meshScale;
  pMesh->get_VertexPositionScale(&meshScale);

  ezMat4 scaleMat;
  scaleMat.SetScalingMatrix(ezUwpUtils::ConvertVec3(meshScale));

  ezMat4 coordMat;
  coordMat.SetIdentity();
  coordMat.SetColumn(0, ezVec4(1, 0, 0, 0));
  coordMat.SetColumn(1, ezVec4(0, 0, 1, 0));
  coordMat.SetColumn(2, ezVec4(0, 1, 0, 0));

  const ezMat4 finalMat = coordMat * relMat * scaleMat;

  UINT32 uiNumVertices = 0;
  pVertexData->get_ElementCount(&uiNumVertices);

  UINT32 uiNumIndices = 0;
  pIndexData->get_ElementCount(&uiNumIndices);

  mb.AllocateStreams(uiNumVertices, ezGALPrimitiveTopology::Triangles, uiNumIndices / 3);

  // Vertices
  {
    const ezVec4* pVertices =
      reinterpret_cast<const ezVec4*>(GetBytePointer(pVertexData, uiNumVertices, DirectXPixelFormat_R32G32B32A32Float, sizeof(ezVec4)));

    for (ezUInt32 v = 0; v < uiNumVertices; ++v)
    {
      mb.SetVertexData(0, v, finalMat * pVertices[v].GetAsVec3()); // position
      mb.SetVertexData(1, v, ezVec3(0, 0, 1));                     // normal
    }
  }

  // Indices
  {
    const ezUInt16* pIndices = reinterpret_cast<const ezUInt16*>(GetBytePointer(pIndexData, uiNumIndices, DirectXPixelFormat_R16UInt, 2));

    ezUInt32 uiTriangle = 0;
    for (ezUInt32 v = 0; v < uiNumIndices; v += 3)
    {
      mb.SetTriangleIndices(uiTriangle, pIndices[v], pIndices[v + 1], pIndices[v + 2]);
      ++uiTriangle;
    }
  }

  mb.ComputeBounds();
  return EZ_SUCCESS;
}

void ezSurfaceReconstructionMeshManager::ClearSurfaceMesh(const ezUuid& guid)
{
  auto it = m_Surfaces.Find(guid);
  if (it.IsValid())
  {
    auto& surface = it.Value();
    surface.m_iLastUpdate = 0;

    ezSrmManagerEvent e;
    e.m_pManager = this;
    e.m_Type = ezSrmManagerEvent::Type::MeshRemoved;
    e.m_MeshGuid = guid;
    m_Events.Broadcast(e);
  }
}

void ezSurfaceReconstructionMeshManager::ClearUnobservedSurfaces()
{
  EZ_LOCK(m_Mutex);

  for (auto it = m_Surfaces.GetIterator(); it.IsValid();)
  {
    // remove all surfaces that still have the tag
    if (it.Value().m_bIsObserved == false)
    {
      const ezUuid guid = it.Key();
      ++it;

      ezLog::Debug("Clearing outdated surface");
      ClearSurfaceMesh(guid);
    }
    else
    {
      ++it;
    }
  }
}
