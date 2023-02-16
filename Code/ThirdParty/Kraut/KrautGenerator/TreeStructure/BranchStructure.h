#pragma once

#include <KrautFoundation/Streams/Streams.h>
#include <KrautGenerator/Description/DescriptionEnums.h>
#include <KrautGenerator/TreeStructure/BranchNode.h>
#include <KrautGenerator/TreeStructure/BranchRandomData.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct TreeStructure;
  struct TreeStructureDesc;

  struct KRAUT_DLL BranchStructure
  {
    BranchStructure();
    ~BranchStructure();

    void Clear();

    aeVec3 GetDirectionAtNode(aeUInt32 uiNode) const;
    const aeMatrix GetNodeTransformation(aeUInt32 uiNode);

    /// \brief Computes m_fThickness for this branch and propagates thickness values for all nodes along the branch.
    ///
    /// Requires the TreeStructure to look up thickness of the parent branch.
    /// Returns false, if the thickness at the parent branch's node is too low, and this branch should be culled.
    bool UpdateThickness(const TreeStructureDesc& structureDesc, const TreeStructure& treeStructure);

    /// \brief Computes the V texture coordinate for all branch nodes
    ///
    /// This is needed at full resolution for the later LOD generation to be consistent.
    /// Also sets m_fLastDiameter and m_uiLastRingVertices, which are needed for LOD branch tip generation and W texture coordinate calculation.
    void GenerateTexCoordV(const Kraut::TreeStructureDesc& treeStructureDesc);

    aeInt32 m_iParentBranchID = -1;
    aeUInt32 m_uiParentBranchNodeID = 0;

    aeInt32 m_iUmbrellaBuddyID = -1;
    float m_fUmbrellaBranchRotation = 0;

    aeDeque<Kraut::BranchNode> m_Nodes;

    //bool m_bManuallyCreated = false;

    float m_fBranchLength = 0.0f;
    float m_fThickness = 0.0f;
    float m_fLastDiameter = 0.0f; ///< Set by GenerateTexCoordV(), needed to compute the V coords for the LOD branch tips
    float m_fFrondColorVariation = 0.0f;   ///< factor from 0 to 1, how much to use the texture color or the variation color
    aeUInt8 m_uiFrondTextureVariation = 0; ///< if a texture contains more than one frond texture (atlas), this value describes which one to use

    aeVec3 m_vLeafUpDirection;

    Kraut::BranchType::Enum m_Type = Kraut::BranchType::None;

    Kraut::BranchRandomData m_RandomData;
  };

} // namespace Kraut
