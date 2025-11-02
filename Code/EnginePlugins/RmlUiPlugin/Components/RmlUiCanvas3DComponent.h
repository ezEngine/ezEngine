#pragma once

#include <RmlUiPlugin/Components/RmlUiCanvasComponentBase.h>

using ezRmlUiCanvas3DComponentManager = ezComponentManagerSimple<class ezRmlUiCanvas3DComponent, ezComponentUpdateType::Always, ezBlockStorageType::Compact, ezWorldUpdatePhase::PostTransform>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiCanvas3DComponent : public ezRmlUiCanvasComponentBase
{
  EZ_DECLARE_COMPONENT_TYPE(ezRmlUiCanvas3DComponent, ezRmlUiCanvasComponentBase, ezRmlUiCanvas3DComponentManager);

public:
  ezRmlUiCanvas3DComponent();
  ~ezRmlUiCanvas3DComponent();

  ezRmlUiCanvas3DComponent& operator=(ezRmlUiCanvas3DComponent&& rhs);

  void Update() final override;

  bool ReceiveInput(const ezVec2& vMousePosInsideCanvas, ezRmlUiInputSnapshot input) final override;
  bool RaycastInput(const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezRmlUiInputSnapshot input);

  void SetTextureSlotName(const char* szName) { m_sTextureSlotName.Assign(szName); }
  const char* GetTextureSlotName() const { return m_sTextureSlotName; }

  void SetTextureSize(const ezVec2U32& vSize);                                     // [ property ]
  const ezVec2U32& GetTextureSize() const { return m_vSize; }                      // [ property ]

  void SetDpiScale(float fDpiScale);                                               // [ property ]
  float GetDpiScale() const { return m_fDpiScale; }                                // [ property ]

  void SetClearStaleInput(bool bClearStaleInput);                                  // [ property ]
  bool GetClearStaleInput() const { return m_bClearStaleInput; }                   // [ property ]

  void SetInteractive(bool bIsInteractive);                                        // [ property ]
  bool IsInteractive() const { return m_bIsInteractive; }                          // [ property ]

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  // adds SetMaterialFile() and GetMaterialFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS(BaseMaterial, m_hBaseMaterial);

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const final override;   // [ msg handler ]

  bool UpdateTextureAndMaterial(bool bForceUpdateMaterial = false);

  static bool RaycastMeshTexCoords(const class ezCpuMeshResource* pMesh, ezUInt32 uiSubMeshIndex, const ezVec3& vRayOrigin, const ezVec3& vRayDir, ezVec2& out_vTexCoords, float fEpsilon = 0.00001f);

  ezMaterialResourceHandle m_hBaseMaterial;
  ezUInt32 m_uiMaterialIndex = 0;
  ezHashedString m_sTextureSlotName;
  float m_fDpiScale = 1.0f;
  bool m_bClearStaleInput = true;
  bool m_bIsInteractive = true;
  ezInt8 m_iInputAge = -1;
  
  ezMaterialResourceHandle m_hMaterial;
  ezTexture2DResourceHandle m_hTexture;
};
