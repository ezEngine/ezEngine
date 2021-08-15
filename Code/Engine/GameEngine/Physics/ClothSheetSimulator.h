#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A simple simulator for swinging and hanging cloth.
///
/// Uses Verlet Integration to update the cloth positions from velocities, and the "Jakobsen method" to enforce distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class EZ_GAMEENGINE_DLL ezClothSimulator
{
public:
  struct Node
  {
    /// Whether this node can swing freely or will remain fixed in place.
    bool m_bFixed = false;
    ezVec3 m_vPosition = ezVec3::ZeroVector();
    ezVec3 m_vPreviousPosition = ezVec3::ZeroVector();
  };

  /// Resolution of the cloth along X
  ezUInt8 m_uiWidth = 32;

  /// Resolution of the cloth along Y
  ezUInt8 m_uiHeight = 32;

  /// Overall force acting equally upon all cloth nodes.
  ezVec3 m_vAcceleration;

  /// Factor with which all node velocities are damped to reduce swinging.
  float m_fDampingFactor = 0.995f;

  /// The distance along x and y between each neighboring node.
  ezVec2 m_vSegmentLength = ezVec2(0.1f);

  /// All cloth nodes.
  ezDynamicArray<Node> m_Nodes;

  void SimulateCloth(const ezTime& tDiff);
  void SimulateStep(const float tDiffSqr, ezUInt32 uiMaxIterations, double fAllowedError);
  bool HasEquilibrium(float fAllowedMovement) const;

private:
  double EnforceDistanceConstraint();
  void UpdateNodePositions(const float tDiffSqr);
  ezVec3 MoveTowards(const ezVec3& posThis, const ezVec3& posNext, float factor, const ezVec3& fallbackDir, double& inout_fError, float fSegLen);

  ezTime m_leftOverTimeStep;
};
