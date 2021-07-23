#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A simple simulator for swinging and hanging ropes.
///
/// Can be used both for interactive rope simulation, as well as to just pre-compute the shape of hanging wires, cables, etc.
/// Uses Verlet Integration to update the rope positions from velocities, and the "Jakobsen method" to enforce
/// rope distance constraints.
///
/// Based on https://owlree.blog/posts/simulating-a-rope.html
class EZ_GAMEENGINE_DLL ezRopeSimulator
{
public:
  struct Node
  {
    ezVec3 m_vPosition = ezVec3::ZeroVector();
    ezVec3 m_vPreviousPosition = ezVec3::ZeroVector();

    // could add per node acceleration
    // could add per node mass
  };

public:
  ezRopeSimulator();
  ~ezRopeSimulator();

  /// \brief External acceleration, typically gravity or a combination of gravity and wind.
  /// Applied to all rope nodes equally.
  ezVec3 m_vAcceleration = ezVec3(0, 0, -10);

  /// \brief All the nodes in the rope
  ezDynamicArray<Node> m_Nodes;

  /// \brief A factor to dampen velocities to make the rope stop swinging.
  /// Should be between 0.97 (strong damping) and 1.0 (no damping).
  float m_fDampingFactor = 0.995f;

  /// \brief How long each rope segment (between two nodes) should be.
  float m_fSegmentLength = 0.1f;

  bool m_bFirstNodeIsFixed = true;
  bool m_bLastNodeIsFixed = true;

  void SimulateRope(const ezTime& tDiff);
  void SimulateStep(const float tDiffSqr, ezUInt32 uiMaxIterations, double fAllowedError);
  void SimulateTillEquilibrium(float fAllowedMovement = 0.005f, ezUInt32 uiMaxIterations = 1000);

private:
  double EnforceDistanceConstraint();
  void UpdateNodePositions(const float tDiffSqr);
  ezVec3 MoveTowards(const ezVec3& posThis, const ezVec3& posNext, float factor, const ezVec3& fallbackDir, double& inout_fError);

  ezTime m_leftOverTimeStep;
};
