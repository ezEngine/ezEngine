#pragma once

#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using ezCpuMeshResourceHandle = ezTypedResourceHandle<class ezCpuMeshResource>;

using ezRmlUiCanvas3DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas3DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostTransform>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas3DComponent : public ezRmlUiCanvasComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, ezRmlUiCanvasComponentBase, ezRmlUiCanvas3DComponentManager);

public:
  ezRmlUiCanvas3DComponent();
  ~ezRmlUiCanvas3DComponent();

  ezRmlUiCanvas3DComponent& operator=(ezRmlUiCanvas3DComponent&& rhs);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void Update() final override;

  bool ReceiveInput(const ezVec2& vMousePosInsideCanvas, ezRmlUiInputSnapshot input) override;
  bool RaycastInput(const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezRmlUiInputSnapshot input);

  /// \brief Changes which mesh will be used for hit testing.
  void SetProxyMesh(const ezMeshResourceHandle& hMesh) { m_hProxyMesh = hMesh; } // [ property ]
  const ezMeshResourceHandle& GetProxyMesh() const { return m_hProxyMesh; }      // [ property ]

  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(ProxyMesh, m_hProxyMesh, SetProxyMesh);

  void SetBaseMaterial(const ezMaterialResourceHandle& hMaterial);                    // [ property ]
  const ezMaterialResourceHandle& GetBaseMaterial() const { return m_hBaseMaterial; } // [ property ]

  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(BaseMaterial, m_hBaseMaterial, SetBaseMaterial);

  void SetMaterialIndex(ezUInt32 uiMaterialIndex);                       // [ property ]
  ezUInt32 GetMaterialIndex() const { return m_uiMaterialIndex; }        // [ property ]

  void SetTextureSlotName(ezStringView sName);                           // [ property ]
  ezStringView GetTextureSlotName() const { return m_sTextureSlotName; } // [ property ]

  void SetTextureSize(const ezVec2U32& vSize);                           // [ property ]
  const ezVec2U32& GetTextureSize() const { return m_vSize; }            // [ property ]

  void SetDpiScale(float fDpiScale);                                     // [ property ]
  float GetDpiScale() const { return m_fDpiScale; }                      // [ property ]

  void SetClearStaleInput(bool bClearStaleInput);                        // [ property ]
  bool GetClearStaleInput() const { return m_bClearStaleInput; }         // [ property ]

  void SetInteractive(bool bIsInteractive);                              // [ property ]
  bool IsInteractive() const { return m_bIsInteractive; }                // [ property ]

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const override; // [ msg handler ]
  virtual void OnMsgReload(ezMsgRmlUiReload& msg) override;                        // [ msg handler ]

  bool UpdateTextureAndMaterial();

  static bool RaycastMeshTexCoords(const class ezCpuMeshResource* pMesh, ezUInt32 uiSubMeshIndex, const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezVec2& out_vTexCoords, float fEpsilon = 0.00001f);

  // properties
  ezMeshResourceHandle m_hProxyMesh;
  ezMaterialResourceHandle m_hBaseMaterial;
  ezUInt32 m_uiMaterialIndex = 0;
  ezHashedString m_sTextureSlotName;
  float m_fDpiScale = 1.0f;
  bool m_bClearStaleInput = true;
  bool m_bIsInteractive = true;

  // runtime data
  ezInt8 m_iInputAge = -1;
  ezCpuMeshResourceHandle m_hCachedCpuMesh;
  ezMaterialResourceHandle m_hMaterial;
  ezTexture2DResourceHandle m_hTexture;
};
