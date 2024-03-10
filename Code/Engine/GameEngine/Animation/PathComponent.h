#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Types/Bitflags.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererCore/RendererCoreDLL.h>

struct ezMsgTransformChanged;
struct ezMsgParentChanged;

//////////////////////////////////////////////////////////////////////////

EZ_DECLARE_FLAGS(ezUInt32, ezPathComponentFlags, VisualizePath, VisualizeUpDir);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPathComponentFlags);

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezMsgPathChanged : public ezEventMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgPathChanged, ezEventMessage);
};

//////////////////////////////////////////////////////////////////////////

class ezPathComponentManager : public ezComponentManager<class ezPathComponent, ezBlockStorageType::FreeList>
{
public:
  ezPathComponentManager(ezWorld* pWorld);

  void SetEnableUpdate(ezPathComponent* pThis, bool bEnable);

protected:
  void Initialize() override;
  void Update(const ezWorldModule::UpdateContext& context);

  ezHybridArray<ezPathComponent*, 32> m_NeedUpdate;
};

/// \brief Describes a path shape.
///
/// This can be used for moving things along the path (see ezFollowPathComponent) or to describe the (complex) shape of an object, for example a rope.
///
/// The ezPathComponent stores the shape as nodes with positions and tangents.
/// It can be asked to provide a 'linearized' representation, e.g. one that is made up of many short segments whose linear interpolation
/// is still reasonably close to the curved shape.
///
/// To set up the shape, attach child objects and attach an ezPathNodeComponent to each. Also give each child object a distinct name.
/// Then reference these child objects by name through the "Nodes" property on the path shape.
///
/// During scene export, typically the child objects are automatically deleted (if they have no children and no other components).
/// Instead, the ezPathComponent stores all necessary information in a more compact representation.
class EZ_GAMEENGINE_DLL ezPathComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPathComponent, ezComponent, ezPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezPathComponent

public:
  ezPathComponent();
  ~ezPathComponent();

  /// \brief Informs the path component, that its shape has changed. Sent by path nodes when they are modified.
  void OnMsgPathChanged(ezMsgPathChanged& ref_msg); // [ message handler ]

  /// \brief Whether the path end connects to the beginning.
  void SetClosed(bool bClosed);                                                 // [ property ]
  bool GetClosed() const { return m_bClosed; }                                  // [ property ]

  void SetPathFlags(ezBitflags<ezPathComponentFlags> flags);                    // [ property ]
  ezBitflags<ezPathComponentFlags> GetPathFlags() const { return m_PathFlags; } // [ property ]


  /// \brief The 'raw' data for a single path control point
  struct ControlPoint
  {
    ezVec3 m_vPosition = ezVec3::MakeZero();
    ezVec3 m_vTangentIn = ezVec3::MakeZero();
    ezVec3 m_vTangentOut = ezVec3::MakeZero();
    ezAngle m_Roll;

    ezResult Serialize(ezStreamWriter& ref_writer) const;
    ezResult Deserialize(ezStreamReader& ref_reader);
  };

  /// \brief If the control points changed recently, this makes sure the local representation is synchronized. Call this before GetControlPointRepresentation(), if necessary.
  void EnsureControlPointRepresentationIsUpToDate();

  /// \brief Grants access to the control points that define the path's shape.
  const ezArrayPtr<const ezPathComponent::ControlPoint> GetControlPointRepresentation() const { return m_ControlPointRepresentation; }


  /// \brief If the path is linearized, this represents a single sample point
  struct LinearizedElement
  {
    ezVec3 m_vPosition = ezVec3::MakeZero();
    ezVec3 m_vUpDirection = ezVec3::MakeAxisZ();
  };

  /// \brief If the control points changed recently, this makes sure the linearized representation gets recreated. Call this before GetLinearizedRepresentation(), if necessary.
  void EnsureLinearizedRepresentationIsUpToDate();

  /// \brief Grants access to the linearized representation that define the path's shape.
  const ezArrayPtr<const ezPathComponent::LinearizedElement> GetLinearizedRepresentation() const { return m_LinearizedRepresentation; }

  /// \brief Returns the total length of the linearized path representation.
  float GetLinearizedRepresentationLength() const { return m_fLinearizedLength; }

  /// \brief Forces that the current control point state is never updated in the future. Used as a work-around during serialization.
  void SetDisableControlPointUpdates(bool bDisable) { m_bDisableControlPointUpdates = bDisable; }

  /// \brief An object that keeps track of where one is sampling the path component.
  ///
  /// If you want to follow a path, keep this object alive and only advance its position,
  /// to not waste performance.
  struct LinearSampler
  {
    /// \brief Resets the sampler to point to the beginning of the path.
    void SetToStart();

  private:
    friend class ezPathComponent;

    float m_fSegmentFraction = 0.0f;
    ezUInt32 m_uiSegmentNode = 0;
  };

  /// \brief Sets the sampler to the desired distance along the path.
  ///
  /// For a long distance, this is a slow operation, because it has to follow the path
  /// from the beginning.
  void SetLinearSamplerTo(LinearSampler& ref_sampler, float fDistance) const;

  /// \brief Moves the sampler along the path by the desired distance.
  ///
  /// Prefer this over SetLinearSamplerTo().
  bool AdvanceLinearSamplerBy(LinearSampler& ref_sampler, float& inout_fAddDistance) const;

  /// \brief Samples the linearized path representation at the desired location and returns the interpolated values.
  ezPathComponent::LinearizedElement SampleLinearizedRepresentation(const LinearSampler& sampler) const;

  /// \brief Specifies how large the error of the linearized path representation is allowed to be.
  ///
  /// The lower the allowed error, the more detailed the linearized path will be, to match the
  /// Bezier representation as closely as possible.
  ///
  /// The error is a distance measure. Thus a value of 0.01 means that the linearized representation
  /// may at most deviate a centimeter from the real curve.
  void SetLinearizationError(float fError);                              // [ property ]
  float GetLinearizationError() const { return m_fLinearizationError; }  // [ property ]

protected:
  ezUInt32 Nodes_GetCount() const { return m_Nodes.GetCount(); }         // [ property ]
  const ezString& Nodes_GetNode(ezUInt32 i) const { return m_Nodes[i]; } // [ property ]
  void Nodes_SetNode(ezUInt32 i, const ezString& node);                  // [ property ]
  void Nodes_Insert(ezUInt32 uiIndex, const ezString& node);             // [ property ]
  void Nodes_Remove(ezUInt32 uiIndex);                                   // [ property ]

  void FindControlPoints(ezDynamicArray<ControlPoint>& out_ControlPoints) const;
  void CreateLinearizedPathRepresentation(const ezDynamicArray<ControlPoint>& points);

  void DrawDebugVisualizations();

  ezBitflags<ezPathComponentFlags> m_PathFlags;                 // [ property ]
  float m_fLinearizationError = 0.05f;                          // [ property ]
  float m_fLinearizedLength = 0.0f;                             //
  bool m_bDisableControlPointUpdates = false;                   //
  bool m_bControlPointsChanged = true;                          //
  bool m_bLinearizedRepresentationChanged = true;               //
  bool m_bClosed = false;                                       // [ property ]
  ezDynamicArray<ezString> m_Nodes;                             // [ property ]
  ezDynamicArray<LinearizedElement> m_LinearizedRepresentation; //
  ezDynamicArray<ControlPoint> m_ControlPointRepresentation;    //
};

//////////////////////////////////////////////////////////////////////////

using ezPathNodeComponentManager = ezComponentManager<class ezPathNodeComponent, ezBlockStorageType::Compact>;

/// \brief The different modes that tangents may use in a path node.
struct ezPathNodeTangentMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Auto,   ///< The curvature through the node is automatically computed to be smooth.
    Linear, ///< There is no curvature through this node/tangent. Creates sharp corners.

    Default = Auto
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezPathNodeTangentMode);


/// \brief Attach this to child object of an ezPathComponent to turn them into viable path nodes.
///
/// See ezPathComponent for details on how to create a path.
///
/// This component allows to specify the mode of the tangents (linear, curved),
/// and also to adjust the 'roll' that the path will have at this location (rotation around the forward axis).
class EZ_GAMEENGINE_DLL ezPathNodeComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPathNodeComponent, ezComponent, ezPathNodeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezPathNodeComponent

public:
  ezPathNodeComponent();
  ~ezPathNodeComponent();

  /// \brief Sets the rotation along the forward axis, that the path shall have at this location.
  void SetRoll(ezAngle roll);                                                      // [ property ]
  ezAngle GetRoll() const { return m_Roll; }                                       // [ property ]
                                                                                   //
  void SetTangentMode1(ezEnum<ezPathNodeTangentMode> mode);                        // [ property ]
  ezEnum<ezPathNodeTangentMode> GetTangentMode1() const { return m_TangentMode1; } // [ property ]
                                                                                   //
  void SetTangentMode2(ezEnum<ezPathNodeTangentMode> mode);                        // [ property ]
  ezEnum<ezPathNodeTangentMode> GetTangentMode2() const { return m_TangentMode2; } // [ property ]

protected:
  void OnMsgTransformChanged(ezMsgTransformChanged& msg);
  void OnMsgParentChanged(ezMsgParentChanged& msg);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void PathChanged();

  ezAngle m_Roll;
  ezEnum<ezPathNodeTangentMode> m_TangentMode1;
  ezEnum<ezPathNodeTangentMode> m_TangentMode2;
};
