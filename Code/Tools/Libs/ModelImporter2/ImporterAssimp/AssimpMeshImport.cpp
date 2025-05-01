#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <assimp/scene.h>
#include <meshoptimizer/meshoptimizer.h>
#include <mikktspace/mikktspace.h>

namespace ezModelImporter2
{
  struct StreamIndices
  {
    ezUInt32 uiPositions = ezInvalidIndex;
    ezUInt32 uiNormals = ezInvalidIndex;
    ezUInt32 uiUV0 = ezInvalidIndex;
    ezUInt32 uiUV1 = ezInvalidIndex;
    ezUInt32 uiTangents = ezInvalidIndex;
    ezUInt32 uiBoneIdx = ezInvalidIndex;
    ezUInt32 uiBoneWgt = ezInvalidIndex;
    ezUInt32 uiColor0 = ezInvalidIndex;
    ezUInt32 uiColor1 = ezInvalidIndex;
  };

  void ImporterAssimp::SimplifyAiMesh(aiMesh* pMesh)
  {
    if (m_Options.m_uiMeshSimplification == 0)
      return;

    // already processed
    if (m_OptimizedMeshes.Contains(pMesh))
      return;

    m_OptimizedMeshes.Insert(pMesh);

    ezUInt32 numIndices = pMesh->mNumFaces * 3;

    ezDynamicArray<ezUInt32> indices;
    indices.Reserve(numIndices);

    ezDynamicArray<ezUInt32> simplifiedIndices;
    simplifiedIndices.SetCountUninitialized(numIndices);

    for (ezUInt32 face = 0; face < pMesh->mNumFaces; ++face)
    {
      indices.PushBack(pMesh->mFaces[face].mIndices[0]);
      indices.PushBack(pMesh->mFaces[face].mIndices[1]);
      indices.PushBack(pMesh->mFaces[face].mIndices[2]);
    }

    const float fTargetError = ezMath::Clamp<ezUInt32>(m_Options.m_uiMaxSimplificationError, 1, 99) / 100.0f;
    const size_t numTargetIndices = static_cast<size_t>((numIndices * (100 - ezMath::Min<ezUInt32>(m_Options.m_uiMeshSimplification, 99))) / 100.0f);

    float err;
    size_t numNewIndices = 0;

    if (m_Options.m_bAggressiveSimplification)
    {
      numNewIndices = meshopt_simplifySloppy(simplifiedIndices.GetData(), indices.GetData(), numIndices, &pMesh->mVertices[0].x, pMesh->mNumVertices, sizeof(aiVector3D), numTargetIndices, fTargetError, &err);
    }
    else
    {
      numNewIndices = meshopt_simplify(simplifiedIndices.GetData(), indices.GetData(), numIndices, &pMesh->mVertices[0].x, pMesh->mNumVertices, sizeof(aiVector3D), numTargetIndices, fTargetError, 0, &err);
      simplifiedIndices.SetCount(static_cast<ezUInt32>(numNewIndices));
    }

    ezDynamicArray<ezUInt32> remapTable;
    remapTable.SetCountUninitialized(pMesh->mNumVertices);
    const size_t numUniqueVerts = meshopt_optimizeVertexFetchRemap(remapTable.GetData(), simplifiedIndices.GetData(), numNewIndices, pMesh->mNumVertices);

    meshopt_remapVertexBuffer(pMesh->mVertices, pMesh->mVertices, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());

    if (pMesh->HasNormals())
    {
      meshopt_remapVertexBuffer(pMesh->mNormals, pMesh->mNormals, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasTextureCoords(0))
    {
      meshopt_remapVertexBuffer(pMesh->mTextureCoords[0], pMesh->mTextureCoords[0], pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasTextureCoords(1))
    {
      meshopt_remapVertexBuffer(pMesh->mTextureCoords[1], pMesh->mTextureCoords[1], pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasVertexColors(0))
    {
      meshopt_remapVertexBuffer(pMesh->mColors[0], pMesh->mColors[0], pMesh->mNumVertices, sizeof(aiColor4D), remapTable.GetData());
    }
    if (pMesh->HasVertexColors(1))
    {
      meshopt_remapVertexBuffer(pMesh->mColors[1], pMesh->mColors[1], pMesh->mNumVertices, sizeof(aiColor4D), remapTable.GetData());
    }
    if (pMesh->HasTangentsAndBitangents())
    {
      meshopt_remapVertexBuffer(pMesh->mTangents, pMesh->mTangents, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
      meshopt_remapVertexBuffer(pMesh->mBitangents, pMesh->mBitangents, pMesh->mNumVertices, sizeof(aiVector3D), remapTable.GetData());
    }
    if (pMesh->HasBones() && m_Options.m_bImportSkinningData)
    {
      for (ezUInt32 b = 0; b < pMesh->mNumBones; ++b)
      {
        auto& bone = pMesh->mBones[b];

        for (ezUInt32 w = 0; w < bone->mNumWeights;)
        {
          auto& weight = bone->mWeights[w];
          const ezUInt32 uiNewIdx = remapTable[weight.mVertexId];

          if (uiNewIdx == ~0u)
          {
            // this vertex got removed -> swap it with the last weight
            bone->mWeights[w] = bone->mWeights[bone->mNumWeights - 1];
            --bone->mNumWeights;
          }
          else
          {
            bone->mWeights[w].mVertexId = uiNewIdx;
            ++w;
          }
        }
      }
    }

    pMesh->mNumVertices = static_cast<ezUInt32>(numUniqueVerts);

    ezDynamicArray<ezUInt32> newIndices;
    newIndices.SetCountUninitialized(static_cast<ezUInt32>(numNewIndices));

    meshopt_remapIndexBuffer(newIndices.GetData(), simplifiedIndices.GetData(), numNewIndices, remapTable.GetData());

    pMesh->mNumFaces = static_cast<ezUInt32>(numNewIndices / 3);

    ezUInt32 nextIdx = 0;
    for (ezUInt32 face = 0; face < pMesh->mNumFaces; ++face)
    {
      pMesh->mFaces[face].mIndices[0] = newIndices[nextIdx++];
      pMesh->mFaces[face].mIndices[1] = newIndices[nextIdx++];
      pMesh->mFaces[face].mIndices[2] = newIndices[nextIdx++];
    }
  }

  ezResult ImporterAssimp::ProcessAiMesh(aiMesh* pMesh, const ezMat4& transform)
  {
    if ((pMesh->mPrimitiveTypes & aiPrimitiveType::aiPrimitiveType_TRIANGLE) == 0) // no triangles in there ?
      return EZ_SUCCESS;

    m_OutputMeshNames.PushBack(pMesh->mName.C_Str());

    if (!m_Options.m_MeshIncludeTags.IsEmpty() || !m_Options.m_MeshExcludeTags.IsEmpty())
    {
      ezLog::Dev("Found mesh with name: '{}'", pMesh->mName.C_Str());
    }

    if (!m_Options.m_MeshIncludeTags.IsEmpty())
    {
      for (const auto& str : m_Options.m_MeshIncludeTags)
      {
        if (ezStringUtils::StartsWith_NoCase(pMesh->mName.C_Str(), str) || ezStringUtils::EndsWith_NoCase(pMesh->mName.C_Str(), str))
        {
          ezLog::Dev("Including mesh '{}' because of include-tag '{}'", pMesh->mName.C_Str(), str);
          goto do_import;
        }
      }

      ezLog::Dev("Skipping mesh '{}', because it doesn't match any include-tag.", pMesh->mName.C_Str());
      return EZ_SUCCESS; // not a failure case
    }

    for (const auto& str : m_Options.m_MeshExcludeTags)
    {
      if (ezStringUtils::StartsWith_NoCase(pMesh->mName.C_Str(), str) || ezStringUtils::EndsWith_NoCase(pMesh->mName.C_Str(), str))
      {
        ezLog::Dev("Skipping mesh '{}' because of exclude-tag '{}'", pMesh->mName.C_Str(), str);
        return EZ_SUCCESS; // not a failure case
      }
    }


  do_import:

    if (m_Options.m_bImportSkinningData && !pMesh->HasBones())
    {
      ezLog::Warning("Mesh contains an unskinned part ('{}' - {} triangles)", pMesh->mName.C_Str(), pMesh->mNumFaces);
      return EZ_SUCCESS;
    }

    // if enabled, the aiMesh is modified in-place to have less detail
    SimplifyAiMesh(pMesh);

    {
      auto& mi = m_MeshInstances[pMesh->mMaterialIndex].ExpandAndGetRef();
      mi.m_GlobalTransform = transform;
      mi.m_pMesh = pMesh;

      m_uiTotalMeshVertices += pMesh->mNumVertices;
      m_uiTotalMeshTriangles += pMesh->mNumFaces;
    }

    return EZ_SUCCESS;
  }

  static void SetMeshTriangleIndices(ezMeshBufferResourceDescriptor& ref_mb, const aiMesh* pMesh, ezUInt32 uiTriangleIndexOffset, ezUInt32 uiVertexIndexOffset, bool bFlipTriangles)
  {
    if (bFlipTriangles)
    {
      for (ezUInt32 triIdx = 0; triIdx < pMesh->mNumFaces; ++triIdx)
      {
        const ezUInt32 finalTriIdx = uiTriangleIndexOffset + triIdx;

        const ezUInt32 f0 = pMesh->mFaces[triIdx].mIndices[0];
        const ezUInt32 f1 = pMesh->mFaces[triIdx].mIndices[1];
        const ezUInt32 f2 = pMesh->mFaces[triIdx].mIndices[2];

        ref_mb.SetTriangleIndices(finalTriIdx, uiVertexIndexOffset + f0, uiVertexIndexOffset + f2, uiVertexIndexOffset + f1);
      }
    }
    else
    {
      for (ezUInt32 triIdx = 0; triIdx < pMesh->mNumFaces; ++triIdx)
      {
        const ezUInt32 finalTriIdx = uiTriangleIndexOffset + triIdx;

        const ezUInt32 f0 = pMesh->mFaces[triIdx].mIndices[0];
        const ezUInt32 f1 = pMesh->mFaces[triIdx].mIndices[1];
        const ezUInt32 f2 = pMesh->mFaces[triIdx].mIndices[2];

        ref_mb.SetTriangleIndices(finalTriIdx, uiVertexIndexOffset + f0, uiVertexIndexOffset + f1, uiVertexIndexOffset + f2);
      }
    }
  }

  static void SetMeshBoneData(ezMeshBufferResourceDescriptor& ref_mb, ezMeshResourceDescriptor& ref_mrd, float& inout_fMaxBoneOffset, const aiMesh* pMesh, ezUInt32 uiVertexIndexOffset, const StreamIndices& streams, bool b8BitBoneIndices, ezMeshBoneWeigthPrecision::Enum weightsPrecision, bool bNormalizeWeights)
  {
    if (!pMesh->HasBones())
      return;

    ezHashedString hs;

    for (ezUInt32 b = 0; b < pMesh->mNumBones; ++b)
    {
      const aiBone* pBone = pMesh->mBones[b];

      hs.Assign(pBone->mName.C_Str());
      const ezUInt32 uiBoneIndex = ref_mrd.m_Bones[hs].m_uiBoneIndex;

      for (ezUInt32 w = 0; w < pBone->mNumWeights; ++w)
      {
        const auto& weight = pBone->mWeights[w];

        const ezUInt32 finalVertIdx = uiVertexIndexOffset + weight.mVertexId;

        ezUInt8* pBoneIndices8 = reinterpret_cast<ezUInt8*>(ref_mb.GetVertexData(streams.uiBoneIdx, finalVertIdx).GetPtr());
        ezUInt16* pBoneIndices16 = reinterpret_cast<ezUInt16*>(pBoneIndices8);
        ezByteArrayPtr pBoneWeights = ref_mb.GetVertexData(streams.uiBoneWgt, finalVertIdx);

        ezVec4 wgt;
        ezMeshBufferUtils::DecodeToVec4(pBoneWeights, ezMeshBoneWeigthPrecision::ToResourceFormat(weightsPrecision), wgt).AssertSuccess();

        // pBoneWeights are initialized with 0
        // so for the first 4 bones we always assign to one slot that is 0 (least weight)
        // if we have 5 bones or more, we then replace the currently lowest value each time

        ezUInt32 uiLeastWeightIdx = 0;

        for (int i = 1; i < 4; ++i)
        {
          if (wgt.GetData()[i] < wgt.GetData()[uiLeastWeightIdx])
          {
            uiLeastWeightIdx = i;
          }
        }

        const float fEncodedWeight = weight.mWeight;

        if (wgt.GetData()[uiLeastWeightIdx] < fEncodedWeight)
        {
          wgt.GetData()[uiLeastWeightIdx] = fEncodedWeight;

          ezMeshBufferUtils::EncodeBoneWeights(wgt, pBoneWeights, ezMeshBoneWeigthPrecision::ToResourceFormat(weightsPrecision)).AssertSuccess();

          if (b8BitBoneIndices)
            pBoneIndices8[uiLeastWeightIdx] = static_cast<ezUInt8>(uiBoneIndex);
          else
            pBoneIndices16[uiLeastWeightIdx] = static_cast<ezUInt16>(uiBoneIndex);
        }
      }
    }
  }

  static void CheckBoneWeights(ezMeshBufferResourceDescriptor& ref_mb, ezMeshResourceDescriptor& ref_mrd, float& inout_fMaxBoneOffset, const StreamIndices& streams, bool b8BitBoneIndices, ezMeshBoneWeigthPrecision::Enum weightsPrecision, bool bNormalizeWeights)
  {
    ezUInt32 uiZeroWeights = 0;

    for (ezUInt32 vtx = 0; vtx < ref_mb.GetVertexCount(); ++vtx)
    {
      ezByteArrayPtr pBoneWeights = ref_mb.GetVertexData(streams.uiBoneWgt, vtx);

      ezVec4 wgt;
      ezMeshBufferUtils::DecodeToVec4(pBoneWeights, ezMeshBoneWeigthPrecision::ToResourceFormat(weightsPrecision), wgt).AssertSuccess();

      if (wgt.IsZero(0.001f))
      {
        ++uiZeroWeights;
      }
      else if (bNormalizeWeights)
      {
        // NOTE: This is absolutely crucial for some meshes to work right
        // On the other hand, it is also possible that some meshes don't like this

        const float summedWeights = wgt.x + wgt.y + wgt.z + wgt.w;

        wgt /= summedWeights;
        ezMeshBufferUtils::EncodeBoneWeights(wgt, pBoneWeights, weightsPrecision).AssertSuccess();
      }

      const ezVec3 vVertexPos = *reinterpret_cast<const ezVec3*>(ref_mb.GetVertexData(streams.uiPositions, vtx).GetPtr());
      const ezUInt8* pBoneIndices8 = reinterpret_cast<const ezUInt8*>(ref_mb.GetVertexData(streams.uiBoneIdx, vtx).GetPtr());
      const ezUInt16* pBoneIndices16 = reinterpret_cast<const ezUInt16*>(ref_mb.GetVertexData(streams.uiBoneIdx, vtx).GetPtr());

      // also find the maximum distance of any vertex to its influencing bones
      // this is used to adjust the bounding box for culling at runtime
      // ie. we can compute the bounding box from a pose, but that only covers the skeleton, not the full mesh
      // so we then grow the bbox by this maximum distance
      // that usually creates a far larger bbox than necessary, but means there are no culling artifacts

      ezUInt16 uiBoneId[4] = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF};

      if (b8BitBoneIndices)
      {
        uiBoneId[0] = pBoneIndices8[0];
        uiBoneId[1] = pBoneIndices8[1];
        uiBoneId[2] = pBoneIndices8[2];
        uiBoneId[3] = pBoneIndices8[3];
      }
      else
      {
        uiBoneId[0] = pBoneIndices16[0];
        uiBoneId[1] = pBoneIndices16[1];
        uiBoneId[2] = pBoneIndices16[2];
        uiBoneId[3] = pBoneIndices16[3];
      }

      for (const auto& bone : ref_mrd.m_Bones)
      {
        for (int b = 0; b < 4; ++b)
        {
          if (wgt.GetData()[b] < 0.2f) // only look at bones that have a proper weight
            continue;

          if (bone.Value().m_uiBoneIndex == uiBoneId[b])
          {
            // move the vertex into local space of the bone, then determine how far it is away from the bone
            const ezVec3 vOffPos = bone.Value().m_GlobalInverseRestPoseMatrix * vVertexPos;
            const float length = vOffPos.GetLength();

            if (length > inout_fMaxBoneOffset)
            {
              inout_fMaxBoneOffset = length;
            }
          }
        }
      }
    }


    if (uiZeroWeights > 0)
    {
      ezLog::Error("Mesh has {} vertices with bone weights that are zero. These will not show up!", uiZeroWeights);
    }
  }

  static void SetMeshVertexData(ezMeshBufferResourceDescriptor& ref_mb, const aiMesh* pMesh, const ezMat4& mGlobalTransform, ezUInt32 uiVertexIndexOffset, const StreamIndices& streams, ezEnum<ezMeshNormalPrecision> meshNormalsPrecision, ezEnum<ezMeshTexCoordPrecision> meshTexCoordsPrecision, ezEnum<ezMeshVertexColorConversion> meshVertexColorConversion)
  {
    ezMat3 normalsTransform = mGlobalTransform.GetRotationalPart();
    if (normalsTransform.Invert(0.0f).Failed())
    {
      ezLog::Warning("Couldn't invert a mesh's transform matrix.");
      normalsTransform.SetIdentity();
    }

    normalsTransform.Transpose();

    for (ezUInt32 vertIdx = 0; vertIdx < pMesh->mNumVertices; ++vertIdx)
    {
      const ezUInt32 finalVertIdx = uiVertexIndexOffset + vertIdx;

      const ezVec3 position = mGlobalTransform * ConvertAssimpType(pMesh->mVertices[vertIdx]);
      ref_mb.SetVertexData(streams.uiPositions, finalVertIdx, position);

      if (streams.uiNormals != ezInvalidIndex && pMesh->HasNormals())
      {
        ezVec3 normal = normalsTransform * ConvertAssimpType(pMesh->mNormals[vertIdx]);
        normal.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();

        ezMeshBufferUtils::EncodeNormal(normal, ref_mb.GetVertexData(streams.uiNormals, finalVertIdx), meshNormalsPrecision).IgnoreResult();
      }

      if (streams.uiUV0 != ezInvalidIndex && pMesh->HasTextureCoords(0))
      {
        const ezVec2 texcoord = ConvertAssimpType(pMesh->mTextureCoords[0][vertIdx]).GetAsVec2();

        ezMeshBufferUtils::EncodeTexCoord(texcoord, ref_mb.GetVertexData(streams.uiUV0, finalVertIdx), meshTexCoordsPrecision).IgnoreResult();
      }

      if (streams.uiUV1 != ezInvalidIndex && pMesh->HasTextureCoords(1))
      {
        const ezVec2 texcoord = ConvertAssimpType(pMesh->mTextureCoords[1][vertIdx]).GetAsVec2();

        ezMeshBufferUtils::EncodeTexCoord(texcoord, ref_mb.GetVertexData(streams.uiUV1, finalVertIdx), meshTexCoordsPrecision).IgnoreResult();
      }

      if (streams.uiColor0 != ezInvalidIndex && pMesh->HasVertexColors(0))
      {
        const ezColor color = ConvertAssimpType(pMesh->mColors[0][vertIdx]);

        ezMeshBufferUtils::EncodeColor(color.GetAsVec4(), ref_mb.GetVertexData(streams.uiColor0, finalVertIdx), meshVertexColorConversion).IgnoreResult();
      }

      if (streams.uiColor1 != ezInvalidIndex && pMesh->HasVertexColors(1))
      {
        const ezColor color = ConvertAssimpType(pMesh->mColors[1][vertIdx]);

        ezMeshBufferUtils::EncodeColor(color.GetAsVec4(), ref_mb.GetVertexData(streams.uiColor1, finalVertIdx), meshVertexColorConversion).IgnoreResult();
      }

      if (streams.uiTangents != ezInvalidIndex && pMesh->HasTangentsAndBitangents())
      {
        ezVec3 normal = normalsTransform * ConvertAssimpType(pMesh->mNormals[vertIdx]);
        ezVec3 tangent = normalsTransform * ConvertAssimpType(pMesh->mTangents[vertIdx]);
        ezVec3 bitangent = normalsTransform * ConvertAssimpType(pMesh->mBitangents[vertIdx]);

        normal.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();
        tangent.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();
        bitangent.NormalizeIfNotZero(ezVec3::MakeZero()).IgnoreResult();

        const float fBitangentSign = ezMath::Abs(tangent.CrossRH(bitangent).Dot(normal));

        ezMeshBufferUtils::EncodeTangent(tangent, fBitangentSign, ref_mb.GetVertexData(streams.uiTangents, finalVertIdx), meshNormalsPrecision).IgnoreResult();
      }
    }
  }

  static void AllocateMeshStreams(ezMeshBufferResourceDescriptor& ref_mb, ezArrayPtr<aiMesh*> referenceMeshes, StreamIndices& inout_streams, ezUInt32 uiTotalMeshVertices, ezUInt32 uiTotalMeshTriangles, ezEnum<ezMeshNormalPrecision> meshNormalsPrecision, ezEnum<ezMeshTexCoordPrecision> meshTexCoordsPrecision,
    bool bImportSkinningData, bool b8BitBoneIndices, ezEnum<ezMeshBoneWeigthPrecision> meshWeightsPrecision)
  {
    inout_streams.uiPositions = ref_mb.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    inout_streams.uiNormals = ref_mb.AddStream(ezGALVertexAttributeSemantic::Normal, ezMeshNormalPrecision::ToResourceFormatNormal(meshNormalsPrecision));
    inout_streams.uiUV0 = ref_mb.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezMeshTexCoordPrecision::ToResourceFormat(meshTexCoordsPrecision));
    inout_streams.uiTangents = ref_mb.AddStream(ezGALVertexAttributeSemantic::Tangent, ezMeshNormalPrecision::ToResourceFormatTangent(meshNormalsPrecision));

    if (bImportSkinningData)
    {
      if (b8BitBoneIndices)
        inout_streams.uiBoneIdx = ref_mb.AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUByte);
      else
        inout_streams.uiBoneIdx = ref_mb.AddStream(ezGALVertexAttributeSemantic::BoneIndices0, ezGALResourceFormat::RGBAUShort);

      inout_streams.uiBoneWgt = ref_mb.AddStream(ezGALVertexAttributeSemantic::BoneWeights0, ezMeshBoneWeigthPrecision::ToResourceFormat(meshWeightsPrecision));
    }

    bool bTexCoords1 = false;
    bool bVertexColors0 = false;
    bool bVertexColors1 = false;

    for (auto pMesh : referenceMeshes)
    {
      if (pMesh->HasTextureCoords(1))
        bTexCoords1 = true;
      if (pMesh->HasVertexColors(0))
        bVertexColors0 = true;
      if (pMesh->HasVertexColors(1))
        bVertexColors1 = true;
    }

    if (bTexCoords1)
    {
      inout_streams.uiUV1 = ref_mb.AddStream(ezGALVertexAttributeSemantic::TexCoord1, ezMeshTexCoordPrecision::ToResourceFormat(meshTexCoordsPrecision));
    }

    if (bVertexColors0)
    {
      inout_streams.uiColor0 = ref_mb.AddStream(ezGALVertexAttributeSemantic::Color0, ezGALResourceFormat::RGBAUByteNormalized);
    }
    if (bVertexColors1)
    {
      inout_streams.uiColor1 = ref_mb.AddStream(ezGALVertexAttributeSemantic::Color1, ezGALResourceFormat::RGBAUByteNormalized);
    }

    ref_mb.AllocateStreams(uiTotalMeshVertices, ezGALPrimitiveTopology::Triangles, uiTotalMeshTriangles, true);
  }

  static void SetMeshBindPoseData(ezMeshResourceDescriptor& ref_mrd, const aiMesh* pMesh, const ezMat4& mGlobalTransform)
  {
    if (!pMesh->HasBones())
      return;

    ezHashedString hs;

    for (ezUInt32 b = 0; b < pMesh->mNumBones; ++b)
    {
      auto pBone = pMesh->mBones[b];

      auto invPose = ConvertAssimpType(pBone->mOffsetMatrix);
      EZ_VERIFY(invPose.Invert(0.0f).Succeeded(), "Inverting the bind pose matrix failed");
      invPose = mGlobalTransform * invPose;
      EZ_VERIFY(invPose.Invert(0.0f).Succeeded(), "Inverting the bind pose matrix failed");

      hs.Assign(pBone->mName.C_Str());
      ref_mrd.m_Bones[hs].m_GlobalInverseRestPoseMatrix = invPose;
    }
  }

  struct MikkData
  {
    ezMeshBufferResourceDescriptor* m_pMeshBuffer = nullptr;
    ezUInt32 m_uiVertexSize = 0;
    const ezUInt16* m_pIndices16 = nullptr;
    const ezUInt32* m_pIndices32 = nullptr;
    const ezUInt8* m_pPositions = nullptr;
    const ezUInt8* m_pNormals = nullptr;
    const ezUInt8* m_pTexCoords = nullptr;
    ezUInt8* m_pTangents = nullptr;
    ezGALResourceFormat::Enum m_NormalsFormat;
    ezGALResourceFormat::Enum m_TexCoordsFormat;
    ezGALResourceFormat::Enum m_TangentsFormat;
  };

  static int MikkGetNumFaces(const SMikkTSpaceContext* pContext)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    return pMikkData->m_pMeshBuffer->GetPrimitiveCount();
  }

  static int MikkGetNumVerticesOfFace(const SMikkTSpaceContext* pContext, int iFace)
  { //
    return 3;
  }

  static void MikkGetPosition16(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    const ezVec3* pSrcData = reinterpret_cast<const ezVec3*>(pMikkData->m_pPositions + (uiVertexIdx * pMikkData->m_uiVertexSize));
    pData[0] = pSrcData->x;
    pData[1] = pSrcData->y;
    pData[2] = pSrcData->z;
  }

  static void MikkGetPosition32(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);

    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    const ezVec3* pSrcData = reinterpret_cast<const ezVec3*>(pMikkData->m_pPositions + (uiVertexIdx * pMikkData->m_uiVertexSize));
    pData[0] = pSrcData->x;
    pData[1] = pSrcData->y;
    pData[2] = pSrcData->z;
  }

  static void MikkGetNormal16(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    ezVec3* pDest = reinterpret_cast<ezVec3*>(pData);
    ezMeshBufferUtils::DecodeNormal(ezConstByteArrayPtr(pMikkData->m_pNormals + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_NormalsFormat, *pDest).IgnoreResult();
  }

  static void MikkGetNormal32(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    ezVec3* pDest = reinterpret_cast<ezVec3*>(pData);
    ezMeshBufferUtils::DecodeNormal(ezConstByteArrayPtr(pMikkData->m_pNormals + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_NormalsFormat, *pDest).IgnoreResult();
  }

  static void MikkGetTexCoord16(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    ezVec2* pDest = reinterpret_cast<ezVec2*>(pData);
    ezMeshBufferUtils::DecodeTexCoord(ezConstByteArrayPtr(pMikkData->m_pTexCoords + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TexCoordsFormat, *pDest).IgnoreResult();
  }

  static void MikkGetTexCoord32(const SMikkTSpaceContext* pContext, float pData[], int iFace, int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    ezVec2* pDest = reinterpret_cast<ezVec2*>(pData);
    ezMeshBufferUtils::DecodeTexCoord(ezConstByteArrayPtr(pMikkData->m_pTexCoords + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TexCoordsFormat, *pDest).IgnoreResult();
  }

  static void MikkSetTangents16(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices16[iFace * 3 + iVert];

    const ezVec3 tangent = *reinterpret_cast<const ezVec3*>(pTangent);

    ezMeshBufferUtils::EncodeTangent(tangent, fSign, ezByteArrayPtr(pMikkData->m_pTangents + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TangentsFormat).IgnoreResult();
  }

  static void MikkSetTangents32(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    MikkData* pMikkData = static_cast<MikkData*>(pContext->m_pUserData);
    const ezUInt32 uiVertexIdx = pMikkData->m_pIndices32[iFace * 3 + iVert];

    const ezVec3 tangent = *reinterpret_cast<const ezVec3*>(pTangent);

    ezMeshBufferUtils::EncodeTangent(tangent, fSign, ezByteArrayPtr(pMikkData->m_pTangents + (uiVertexIdx * pMikkData->m_uiVertexSize), 32), pMikkData->m_TangentsFormat).IgnoreResult();
  }

  ezResult ImporterAssimp::RecomputeTangents()
  {
    auto& md = m_Options.m_pMeshOutput->MeshBufferDesc();

    if (!md.HasIndexBuffer())
      return EZ_FAILURE;

    MikkData mikkd;
    mikkd.m_pMeshBuffer = &md;
    mikkd.m_uiVertexSize = md.GetVertexDataSize();
    mikkd.m_pIndices16 = reinterpret_cast<const ezUInt16*>(md.GetIndexBufferData().GetData());
    mikkd.m_pIndices32 = reinterpret_cast<const ezUInt32*>(md.GetIndexBufferData().GetData());

    for (ezUInt32 i = 0; i < md.GetVertexDeclaration().m_VertexStreams.GetCount(); ++i)
    {
      const auto& stream = md.GetVertexDeclaration().m_VertexStreams[i];

      if (stream.m_Semantic == ezGALVertexAttributeSemantic::Position)
      {
        mikkd.m_pPositions = md.GetVertexData(i, 0).GetPtr();
      }
      else if (stream.m_Semantic == ezGALVertexAttributeSemantic::Normal)
      {
        mikkd.m_pNormals = md.GetVertexData(i, 0).GetPtr();
        mikkd.m_NormalsFormat = stream.m_Format;
      }
      else if (stream.m_Semantic == ezGALVertexAttributeSemantic::TexCoord0)
      {
        mikkd.m_pTexCoords = md.GetVertexData(i, 0).GetPtr();
        mikkd.m_TexCoordsFormat = stream.m_Format;
      }
      else if (stream.m_Semantic == ezGALVertexAttributeSemantic::Tangent)
      {
        mikkd.m_pTangents = md.GetVertexData(i, 0).GetPtr();
        mikkd.m_TangentsFormat = stream.m_Format;
      }
    }

    if (mikkd.m_pPositions == nullptr || mikkd.m_pTexCoords == nullptr || mikkd.m_pNormals == nullptr || mikkd.m_pTangents == nullptr)
      return EZ_FAILURE;

    // Use Morton S. Mikkelsen's tangent calculation.
    SMikkTSpaceContext context;
    SMikkTSpaceInterface functions;
    context.m_pUserData = &mikkd;
    context.m_pInterface = &functions;

    functions.m_setTSpace = nullptr;
    functions.m_getNumFaces = MikkGetNumFaces;
    functions.m_getNumVerticesOfFace = MikkGetNumVerticesOfFace;

    if (md.Uses32BitIndices())
    {
      functions.m_getPosition = MikkGetPosition32;
      functions.m_getNormal = MikkGetNormal32;
      functions.m_getTexCoord = MikkGetTexCoord32;
      functions.m_setTSpaceBasic = MikkSetTangents32;
    }
    else
    {
      functions.m_getPosition = MikkGetPosition16;
      functions.m_getNormal = MikkGetNormal16;
      functions.m_getTexCoord = MikkGetTexCoord16;
      functions.m_setTSpaceBasic = MikkSetTangents16;
    }

    if (!genTangSpaceDefault(&context))
      return EZ_FAILURE;

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::PrepareOutputMesh()
  {
    if (m_Options.m_pMeshOutput == nullptr)
      return EZ_SUCCESS;

    auto& mb = m_Options.m_pMeshOutput->MeshBufferDesc();

    if (m_Options.m_bImportSkinningData)
    {
      for (auto itMesh : m_MeshInstances)
      {
        for (const auto& mi : itMesh.Value())
        {
          SetMeshBindPoseData(*m_Options.m_pMeshOutput, mi.m_pMesh, mi.m_GlobalTransform);
        }
      }

      ezUInt16 uiBoneCounter = 0;
      for (auto itBone : m_Options.m_pMeshOutput->m_Bones)
      {
        itBone.Value().m_uiBoneIndex = uiBoneCounter;
        ++uiBoneCounter;
      }
    }

    const bool b8BitBoneIndices = m_Options.m_pMeshOutput->m_Bones.GetCount() <= 255;

    StreamIndices streams;
    AllocateMeshStreams(mb, ezArrayPtr<aiMesh*>(m_pScene->mMeshes, m_pScene->mNumMeshes), streams, m_uiTotalMeshVertices, m_uiTotalMeshTriangles, m_Options.m_MeshNormalsPrecision, m_Options.m_MeshTexCoordsPrecision, m_Options.m_bImportSkinningData, b8BitBoneIndices, m_Options.m_MeshBoneWeightPrecision);

    ezUInt32 uiMeshPrevTriangleIdx = 0;
    ezUInt32 uiMeshCurVertexIdx = 0;
    ezUInt32 uiMeshCurTriangleIdx = 0;
    ezUInt32 uiMeshCurSubmeshIdx = 0;

    const bool bFlipTriangles = ezGraphicsUtils::IsTriangleFlipRequired(m_Options.m_RootTransform);

    for (auto itMesh : m_MeshInstances)
    {
      const ezUInt32 uiMaterialIdx = itMesh.Key();

      for (const auto& mi : itMesh.Value())
      {
        if (m_Options.m_bImportSkinningData && !mi.m_pMesh->HasBones())
        {
          // skip meshes that have no bones
          continue;
        }

        SetMeshVertexData(mb, mi.m_pMesh, mi.m_GlobalTransform, uiMeshCurVertexIdx, streams, m_Options.m_MeshNormalsPrecision, m_Options.m_MeshTexCoordsPrecision, m_Options.m_MeshVertexColorConversion);

        if (m_Options.m_bImportSkinningData)
        {
          SetMeshBoneData(mb, *m_Options.m_pMeshOutput, m_Options.m_pMeshOutput->m_fMaxBoneVertexOffset, mi.m_pMesh, uiMeshCurVertexIdx, streams, b8BitBoneIndices, m_Options.m_MeshBoneWeightPrecision, m_Options.m_bNormalizeWeights);
        }

        SetMeshTriangleIndices(mb, mi.m_pMesh, uiMeshCurTriangleIdx, uiMeshCurVertexIdx, bFlipTriangles);

        uiMeshCurTriangleIdx += mi.m_pMesh->mNumFaces;
        uiMeshCurVertexIdx += mi.m_pMesh->mNumVertices;
      }

      if (uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx == 0)
      {
        // skip empty submeshes
        continue;
      }

      if (uiMaterialIdx >= m_OutputMaterials.GetCount())
      {
        m_Options.m_pMeshOutput->SetMaterial(uiMeshCurSubmeshIdx, "");
      }
      else
      {
        m_OutputMaterials[uiMaterialIdx].m_iReferencedByMesh = static_cast<ezInt32>(uiMeshCurSubmeshIdx);
        m_Options.m_pMeshOutput->SetMaterial(uiMeshCurSubmeshIdx, m_OutputMaterials[uiMaterialIdx].m_sName);
      }

      m_Options.m_pMeshOutput->AddSubMesh(uiMeshCurTriangleIdx - uiMeshPrevTriangleIdx, uiMeshPrevTriangleIdx, uiMeshCurSubmeshIdx);

      uiMeshPrevTriangleIdx = uiMeshCurTriangleIdx;
      ++uiMeshCurSubmeshIdx;
    }

    if (m_Options.m_bImportSkinningData)
    {
      CheckBoneWeights(mb, *m_Options.m_pMeshOutput, m_Options.m_pMeshOutput->m_fMaxBoneVertexOffset, streams, b8BitBoneIndices, m_Options.m_MeshBoneWeightPrecision, m_Options.m_bNormalizeWeights);
    }

    m_Options.m_pMeshOutput->ComputeBounds();

    return EZ_SUCCESS;
  }
} // namespace ezModelImporter2
