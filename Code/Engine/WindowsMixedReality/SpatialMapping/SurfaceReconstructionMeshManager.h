#pragma once

#ifdef BUILDSYSTEM_ENABLE_MIXEDREALITY_SUPPORT

#include <WindowsMixedReality/Basics.h>
#include <wrl/client.h>
#include <Windows.perception.spatial.surfaces.h>
#include <Core/World/Declarations.h>
#include <Foundation/Configuration/Singleton.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

struct ezSrmManagerEvent
{
  enum class Type
  {
    AvailabilityChanged, ///< SRM data is now known to be available or not
    MeshRemoved,
    MeshUpdated,
  };

  class ezSurfaceReconstructionMeshManager* m_pManager = nullptr;
  Type m_Type;
  ezUuid m_MeshGuid;
};

struct ezSurfaceMeshInfo
{
  ezUuid m_MeshGuid;
  ezInt64 m_iLastUpdate = 0; ///< If this is set to zero, the mesh was deleted
  ezTransform m_Transform;
  ezMeshBufferResourceDescriptor m_MeshData;
  bool m_bIsObserved = true;

  //////////////////////////////////////////////////////////////////////////
  //ezGameObjectHandle m_hGameObject;
  //ezComponentHandle m_hMeshComponent;
};

class EZ_WINDOWSMIXEDREALITY_DLL ezSurfaceReconstructionMeshManager
{
  EZ_DECLARE_SINGLETON(ezSurfaceReconstructionMeshManager);

public:
  ezSurfaceReconstructionMeshManager();
  ~ezSurfaceReconstructionMeshManager();

  /// \brief Helper function to write all supported vertex/normal/index formats to the log.
  static void LogSupportedFormats();

  enum class SrmAvailability { Pending, Available, NotAvailable };
  /// \brief Returns whether access to the surface reconstruction mesh is available.
  SrmAvailability GetSrmAvailability() const { return m_SrmAvailability; }

  void PullCurrentSurfaces();

  void ClearUnobservedSurfaces();

  ezEvent<const ezSrmManagerEvent&> m_Events;


  /// \brief Returns all surfaces that are known at the current time.
  const ezMap<ezUuid, ezSurfaceMeshInfo>& BeginAccessingSurfaces();
  void EndAccessingSurfaces();

private:
  void RequestSpatialMappingAccess();
  void CreateSurfaceObserver();
  void UpdateSurface(const ezUuid& guid, ABI::Windows::Perception::Spatial::Surfaces::ISpatialSurfaceInfo* pSurfaceInfo);
  void UpdateSurfaceMesh(const ezUuid& guid, ABI::Windows::Perception::Spatial::Surfaces::ISpatialSurfaceMesh* pMesh);
  void UpdateMeshData(ezMeshBufferResourceDescriptor& mb, ABI::Windows::Perception::Spatial::Surfaces::ISpatialSurfaceMesh* pMesh);

  void ClearSurfaceMesh(const ezUuid& guid);

  mutable ezMutex m_Mutex;
  ComPtr<ABI::Windows::Perception::Spatial::Surfaces::ISpatialSurfaceObserver> m_pSurfaceObserver;
  SrmAvailability m_SrmAvailability = SrmAvailability::Pending;
  ezMap<ezUuid, ezSurfaceMeshInfo> m_Surfaces;
};

#endif