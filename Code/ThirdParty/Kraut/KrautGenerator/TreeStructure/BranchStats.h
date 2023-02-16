#pragma once

#include <KrautFoundation/Math/Vec3.h>
#include <KrautGenerator/Description/DescriptionEnums.h>
#include <KrautGenerator/TreeStructure/BranchRandomData.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct KRAUT_DLL BranchStats
  {
    Kraut::BranchRandomData m_RandomData;

    Kraut::BranchType::Enum m_Type = BranchType::None;

    aeInt32 m_iParentBranchID = -1;
    aeInt32 m_iParentBranchNodeID = 0;

    float m_fBranchLength = 0.0f;    //!< The length of the branch
    float m_fBranchThickness = 0.0f; //!< The (relative) thickness of the branch to its parent branch

    float m_fBranchAngle = 0.0f;
    float m_fRotationalDeviation = 0.0f;
    float m_fFrondColorVariation = 0.0f;
    aeUInt8 m_uiFrondTextureVariation = 0;

    aeVec3 m_vStartPosition = aeVec3::ZeroVector();  //!< At which point in space the branch starts
    aeVec3 m_vStartDirection = aeVec3::ZeroVector(); //!< At which direction the branch starts growing
    aeVec3 m_vGrowDirection = aeVec3::ZeroVector();  //!< Which general grow direction the branch has
    aeVec3 m_vGrowDirection2 = aeVec3::ZeroVector(); //!< Which general grow direction the branch has
    float m_fGrowDir2UUsageDistance = 0.0f;          //!< At which absolute distance to use the second grow direction
  };

} // namespace Kraut
