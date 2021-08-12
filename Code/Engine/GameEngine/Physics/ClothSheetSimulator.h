#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezClothSimulator
{
public:
  struct Node
  {
    bool m_bFixed = false;
    ezVec3 m_vPosition = ezVec3::ZeroVector();
    ezVec3 m_vPreviousPosition = ezVec3::ZeroVector();
  };

  ezUInt8 m_uiWidth = 32;
  ezUInt8 m_uiHeight = 32;

  ezVec3 m_vAcceleration;

  float m_fDampingFactor = 0.995f;

  ezVec2 m_vSegmentLength = ezVec2(0.1f);

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
