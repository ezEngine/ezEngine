#include <BakingPluginPCH.h>

#include <BakingPlugin/BakingScene.h>
#include <BakingPlugin/Tracer/TracerEmbree.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

#include <embree3/rtcore.h>

namespace
{
  static RTCDevice s_rtcDevice;
  static ezHashTable<ezHashedString, RTCScene, ezHashHelper<ezHashedString>, ezStaticAllocatorWrapper> s_rtcMeshCache;

  const char* rtcErrorCodeToString[] = {
    "RTC_NO_ERROR",
    "RTC_UNKNOWN_ERROR",
    "RTC_INVALID_ARGUMENT",
    "RTC_INVALID_OPERATION",
    "RTC_OUT_OF_MEMORY",
    "RTC_UNSUPPORTED_CPU",
    "RTC_CANCELLED"};

  const char* GetStringFromRTCErrorCode(RTCError code)
  {
    return (code >= 0 && code < EZ_ARRAY_SIZE(rtcErrorCodeToString)) ? rtcErrorCodeToString[code] : "RTC invalid error code";
  }

  static void ErrorCallback(void* userPtr, RTCError code, const char* str)
  {
    ezLog::Error("Embree: {}: {}", GetStringFromRTCErrorCode(code), str);
  }

  static ezResult InitDevice()
  {
    if (s_rtcDevice == nullptr)
    {
      if (s_rtcDevice = rtcNewDevice("threads=1"))
      {
        ezLog::Info("Created new Embree Device (Version {})", RTC_VERSION_STRING);

        rtcSetDeviceErrorFunction(s_rtcDevice, &ErrorCallback, nullptr);

        bool bRay4Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY4_SUPPORTED);
        bool bRay8Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY8_SUPPORTED);
        bool bRay16Supported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_NATIVE_RAY16_SUPPORTED);
        bool bRayStreamSupported = rtcGetDeviceProperty(s_rtcDevice, RTC_DEVICE_PROPERTY_RAY_STREAM_SUPPORTED);

        ezLog::Info("Supported ray packets: Ray4:{}, Ray8:{}, Ray16:{}, RayStream:{}", bRay4Supported, bRay8Supported, bRay16Supported, bRayStreamSupported);
      }
      else
      {
        ezLog::Error("Failed to create Embree Device. Error: {}", GetStringFromRTCErrorCode(rtcGetDeviceError(nullptr)));
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }

  static void DeinitDevice()
  {
    for (auto it : s_rtcMeshCache)
    {
      rtcReleaseScene(it.Value());
    }
    s_rtcMeshCache.Clear();

    rtcReleaseDevice(s_rtcDevice);
    s_rtcDevice = nullptr;
  }

  static RTCScene GetOrCreateMesh(const ezHashedString& sResourceId)
  {
    RTCScene scene = nullptr;
    if (s_rtcMeshCache.TryGetValue(sResourceId, scene))
    {
      return scene;
    }

    ezCpuMeshResourceHandle hCpuMesh = ezResourceManager::LoadResource<ezCpuMeshResource>(sResourceId);
    ezResourceLock<ezCpuMeshResource> pCpuMesh(hCpuMesh, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      ezLog::Warning("Failed to retrieve CPU mesh '{}'", sResourceId);
      return nullptr;
    }

    RTCGeometry triangleMesh = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
    {
      const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

      rtcCommitGeometry(triangleMesh);
    }

    scene = rtcNewScene(s_rtcDevice);
    {
      rtcAttachGeometry(scene, triangleMesh);
      rtcReleaseGeometry(triangleMesh);

      rtcCommitScene(scene);
    }

    s_rtcMeshCache.Insert(sResourceId, scene);
    return scene;
  }

} // namespace

struct ezTracerEmbree::Data
{
  ~Data()
  {
    ClearScene();
  }

  void ClearScene()
  {
    if (m_rtcScene != nullptr)
    {
      rtcReleaseScene(m_rtcScene);
      m_rtcScene = nullptr;
    }
  }

  RTCScene m_rtcScene = nullptr;
};

ezTracerEmbree::ezTracerEmbree()
{
  m_pData = EZ_DEFAULT_NEW(Data);
}

ezTracerEmbree::~ezTracerEmbree() = default;

ezResult ezTracerEmbree::BuildScene(const ezBakingScene& scene)
{
  EZ_SUCCEED_OR_RETURN(InitDevice());

  m_pData->ClearScene();
  m_pData->m_rtcScene = rtcNewScene(s_rtcDevice);

  for (auto& meshObject : scene.GetMeshObjects())
  {
    RTCScene mesh = GetOrCreateMesh(meshObject.m_MeshResourceId);
    if (mesh == nullptr)
    {
      continue;
    }

    RTCGeometry instance = rtcNewGeometry(s_rtcDevice, RTC_GEOMETRY_TYPE_INSTANCE);
    {
      rtcSetGeometryInstancedScene(instance, mesh);

      ezMat4 transform = ezSimdConversion::ToMat4(meshObject.m_GlobalTransform.GetAsMat4());
      rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, transform.m_fElementsCM);

      rtcCommitGeometry(instance);
    }

    rtcAttachGeometry(m_pData->m_rtcScene, instance);
    rtcReleaseGeometry(instance);
  }

  rtcCommitScene(m_pData->m_rtcScene);

  return EZ_SUCCESS;
}

void ezTracerEmbree::TraceRays()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}
