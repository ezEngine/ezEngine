#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Graphics/Spline.h>
#include <Core/Messages/EventMessage.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/Bitflags.h>

struct ezMsgTransformChanged;
struct ezMsgParentChanged;
struct ezMsgExtractRenderData;
class ezSplineComponent;
class ezSplineNodeComponent;

//////////////////////////////////////////////////////////////////////////

EZ_DECLARE_FLAGS(ezUInt8, ezSplineComponentFlags, VisualizeSpline, VisualizeUpDir, VisualizeTangents);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSplineComponentFlags);

//////////////////////////////////////////////////////////////////////////

struct EZ_RENDERERCORE_DLL ezMsgSplineChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgSplineChanged, ezEventMessage);
};

//////////////////////////////////////////////////////////////////////////

class ezSplineComponentManager : public ezComponentManager<class ezSplineComponent, ezBlockStorageType::FreeList>
{
public:
  ezSplineComponentManager(ezWorld* pWorld);

  void SetEnableUpdate(ezSplineComponent* pThis, bool bEnable);

protected:
  void Initialize() override;
  void Update(const ezWorldModule::UpdateContext& context);

  ezHybridArray<ezSplineComponent*, 32> m_NeedUpdate;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Describes a Spline shape.
///
/// This can be used for moving things along the spline (see ezFollowSplineComponent) or to describe the (complex) shape of an object, for example a rope.
///
/// The ezSplineComponent stores the shape as nodes with positions and tangents.
/// Remapping XXXX
///
/// To set up the shape, attach child objects and attach an ezSplineNodeComponent to each. Also give each child object a distinct name.
/// Then reference these child objects by name through the "Nodes" property on the spline shape.
///
/// During scene export, typically the child objects are automatically deleted (if they have no children and no other components).
/// Instead, the ezSplineComponent stores all necessary information in a more compact representation.
class EZ_RENDERERCORE_DLL ezSplineComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSplineComponent, ezComponent, ezSplineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezSplineComponent

public:
  ezSplineComponent();
  ~ezSplineComponent();

  /// \brief Informs the spline component, that its shape has changed. Sent by spline nodes when they are modified.
  void OnMsgSplineChanged(ezMsgSplineChanged& ref_msg); // [ message handler ]

  /// \brief Whether the spline end connects to the beginning.
  void SetClosed(bool bClosed);                                                       // [ property ]
  bool GetClosed() const { return m_Spline.m_bClosed; }                               // [ property ]

  void SetSplineFlags(ezBitflags<ezSplineComponentFlags> flags);                      // [ property ]
  ezBitflags<ezSplineComponentFlags> GetSplineFlags() const { return m_SplineFlags; } // [ property ]

  /// \brief Returns the position of the spline at the given key (segment index + t).
  ezVec3 GetPositionAtKey(float fKey) const; // [ scriptable ]

  /// \brief Returns the forward direction of the spline at the given key (segment index + t).
  ezVec3 GetForwardDirAtKey(float fKey) const; // [ scriptable ]

  /// \brief Returns the up direction of the spline at the given key (segment index + t).
  ezVec3 GetUpDirAtKey(float fKey) const; // [ scriptable ]

  /// \brief Returns the scale of the spline at the given key (segment index + t).
  ezVec3 GetScaleAtKey(float fKey) const; // [ scriptable ]

  /// \brief Returns the transform of the spline at the given key (segment index + t).
  ezTransform GetTransformAtKey(float fKey) const; // [ scriptable ]


  /// \brief Returns the total length of the spline (in local space)
  float GetTotalLength() const { return m_fTotalLength; } // [ scriptable ]

  /// \brief Returns the spline key (segment index + t) for the given distance along the spline
  float GetKeyAtDistance(float fDistance) const; // [ scriptable ]

  /// \brief Returns the position of the spline at the given distance along the spline.
  ezVec3 GetPositionAtDistance(float fDistance) const; // [ scriptable ]

  /// \brief Returns the forward direction of the spline at the given distance along the spline.
  ezVec3 GetForwardDirAtDistance(float fDistance) const; // [ scriptable ]

  /// \brief Returns the up direction of the spline at the given distance along the spline.
  ezVec3 GetUpDirAtDistance(float fDistance) const; // [ scriptable ]

  /// \brief Returns the scale of the spline at the given distance along the spline.
  ezVec3 GetScaleAtDistance(float fDistance) const; // [ scriptable ]

  /// \brief Returns the full transform of the spline at the given distance along the spline.
  ezTransform GetTransformAtDistance(float fDistance) const; // [ scriptable ]


  /// \brief Access to the underlying spline object
  const ezSpline& GetSpline() const { return m_Spline; }
  void SetSpline(ezSpline&& spline);

  ezUInt32 GetChangeCounter() const { return m_Spline.m_uiChangeCounter; } // [ scriptable ]

protected:
  friend class ezSplineNodeComponent;

  ezUInt32 Nodes_GetCount() const { return m_Nodes.GetCount(); }                           // [ property ]
  const ezHashedString& Nodes_GetNode(ezUInt32 uiIndex) const { return m_Nodes[uiIndex]; } // [ property ]
  void Nodes_SetNode(ezUInt32 uiIndex, const ezHashedString& sNodeName);                   // [ property ]
  void Nodes_Insert(ezUInt32 uiIndex, const ezHashedString& sNodeName);                    // [ property ]
  void Nodes_Remove(ezUInt32 uiIndex);                                                     // [ property ]

  void UpdateSpline();

  ezSplineNodeComponent* FindNodeComponent(const ezHashedString& sNodeName);
  void UpdateFromNodeObjects();

  void InsertHalfPoint(ezDynamicArray<float>& ref_Ts, ezUInt32 uiCp0, float fLowerT, float fUpperT, const ezSimdVec4f& vLowerPos, const ezSimdVec4f& vUpperPos, float fDistSqr, ezInt32 iMinSteps, ezInt32 iMaxSteps) const;

  void CreateDistanceToKeyRemapping();

  void DrawDebugVisualizations(ezBitflags<ezSplineComponentFlags> flags) const;
  void DrawDebugTangents(ezUInt32 uiPointIndex, ezSplineTangentMode::Enum tangentModeIn = ezSplineTangentMode::Default, ezSplineTangentMode::Enum tangentModeOut = ezSplineTangentMode::Default) const;
  bool DrawSplineOnSelection() const;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezSpline m_Spline;

  ezSmallArray<ezHashedString, 1> m_Nodes; // [ property ]

  ezArrayMap<float, float> m_DistanceToKey;
  float m_fTotalLength = 0.0f;

  ezBitflags<ezSplineComponentFlags> m_SplineFlags; // [ property ]
  ezUInt8 m_uiDummy = 0;
  mutable ezUInt16 m_uiExtractedFrame = 0;
};

//////////////////////////////////////////////////////////////////////////

using ezSplineNodeComponentManager = ezComponentManager<class ezSplineNodeComponent, ezBlockStorageType::Compact>;

/// \brief Attach this to child object of an ezSplineComponent to turn them into viable spline nodes.
///
/// See ezSplineComponent for details on how to create a spline.
///
/// This component allows to specify the mode of the tangents (linear, auto),
/// and also to adjust the 'roll' that the spline will have at this location (rotation around the forward axis).
class EZ_RENDERERCORE_DLL ezSplineNodeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSplineNodeComponent, ezComponent, ezSplineNodeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezSplineNodeComponent

public:
  ezSplineNodeComponent();
  ~ezSplineNodeComponent();

  /// \brief Sets the rotation along the forward axis, that the spline shall have at this location.
  void SetRoll(ezAngle roll);                                                        // [ property ]
  ezAngle GetRoll() const { return m_Roll; }                                         // [ property ]
                                                                                     //
  void SetTangentModeIn(ezEnum<ezSplineTangentMode> mode);                           // [ property ]
  ezEnum<ezSplineTangentMode> GetTangentModeIn() const { return m_TangentModeIn; }   // [ property ]
                                                                                     //
  void SetTangentModeOut(ezEnum<ezSplineTangentMode> mode);                          // [ property ]
  ezEnum<ezSplineTangentMode> GetTangentModeOut() const { return m_TangentModeOut; } // [ property ]

  void SetCustomTangentIn(const ezVec3& vTangent);                                   // [ property ]
  const ezVec3& GetCustomTangentIn() const { return m_vCustomTangentIn; }            // [ property ]

  void SetCustomTangentOut(const ezVec3& vTangent);                                  // [ property ]
  const ezVec3& GetCustomTangentOut() const { return m_vCustomTangentOut; }          // [ property ]

  void SetLinkCustomTangents(bool bLink);                                            // [ property ]
  bool GetLinkCustomTangents() const;                                                // [ property ]

protected:
  friend class ezSplineComponent;

  void OnMsgTransformChanged(ezMsgTransformChanged& msg);
  void OnMsgParentChanged(ezMsgParentChanged& msg);
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void OnActivated() override;

  void SplineChanged();

  ezAngle m_Roll;
  ezEnum<ezSplineTangentMode> m_TangentModeIn;
  ezEnum<ezSplineTangentMode> m_TangentModeOut;

  ezUInt16 m_uiNodeIndex = ezSmallInvalidIndex; // Internal, set by ezSplineComponent

  ezVec3 m_vCustomTangentIn = ezVec3::MakeZero();
  ezVec3 m_vCustomTangentOut = ezVec3::MakeZero();
};
