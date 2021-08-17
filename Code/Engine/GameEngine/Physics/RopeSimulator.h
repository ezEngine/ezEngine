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
    ezSimdVec4f m_vPosition = ezSimdVec4f::ZeroVector();
    ezSimdVec4f m_vPreviousPosition = ezSimdVec4f::ZeroVector();

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
  void SimulateStep(const ezSimdFloat tDiffSqr, ezUInt32 uiMaxIterations, ezSimdFloat fAllowedError);
  void SimulateTillEquilibrium(ezSimdFloat fAllowedMovement = 0.005f, ezUInt32 uiMaxIterations = 1000);
  bool HasEquilibrium(ezSimdFloat fAllowedMovement) const;

private:
  ezSimdFloat EnforceDistanceConstraint();
  void UpdateNodePositions(const ezSimdFloat tDiffSqr);
  ezSimdVec4f MoveTowards(const ezSimdVec4f posThis, const ezSimdVec4f posNext, ezSimdFloat factor, const ezSimdVec4f fallbackDir, ezSimdFloat& inout_fError);

  ezTime m_leftOverTimeStep;
};
