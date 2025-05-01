#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/Logging/Log.h>
#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>

#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace ezModelImporter2
{
  ImporterAssimp::ImporterAssimp() = default;
  ImporterAssimp::~ImporterAssimp() = default;

  class aiLogStreamError : public Assimp::LogStream
  {
  public:
    void write(const char* szMessage) { ezLog::Warning("AssImp: {0}", szMessage); }
  };

  class aiLogStreamWarning : public Assimp::LogStream
  {
  public:
    void write(const char* szMessage) { ezLog::Warning("AssImp: {0}", szMessage); }
  };

  class aiLogStreamInfo : public Assimp::LogStream
  {
  public:
    void write(const char* szMessage) { ezLog::Debug("AssImp: {0}", szMessage); }
  };

  ezResult ImporterAssimp::DoImport()
  {
    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL);

    Assimp::DefaultLogger::get()->attachStream(new aiLogStreamError(), Assimp::Logger::Err);
    Assimp::DefaultLogger::get()->attachStream(new aiLogStreamWarning(), Assimp::Logger::Warn);
    Assimp::DefaultLogger::get()->attachStream(new aiLogStreamInfo(), Assimp::Logger::Info);

    // Note: ReadFileFromMemory is not able to read dependent files even if we use our own Assimp::IOSystem.
    // It is possible to use ReadFile instead but this leads to a lot of code...
    // Triangulate:           Our mesh format cannot handle anything else.
    // JoinIdenticalVertices: Assimp doesn't use index buffer at all if this is not specified.
    // TransformUVCoords:     As of now we do not have a concept for uv transforms.
    // Process_FlipUVs:       Assimp assumes OpenGl style UV coordinate system otherwise.
    // ImproveCacheLocality:  Reorders triangles for better vertex cache locality.

    ezUInt32 uiAssimpFlags = 0;
    if (m_Options.m_pMeshOutput != nullptr)
    {
      uiAssimpFlags |= aiProcess_Triangulate | aiProcess_TransformUVCoords | aiProcess_FlipUVs | aiProcess_ImproveCacheLocality;

      if (!m_Options.m_bImportSkinningData)
      {
        // joining vertices doesn't take into account that two vertices might have different bone assignments
        // so in case of a mesh that is cut into multiple pieces (breakable object),
        // it will re-join vertices that are supposed to stay separate
        // therefore don't do this for skinned meshes
        uiAssimpFlags |= aiProcess_JoinIdenticalVertices;
      }
    }



    m_pScene = m_Importer.ReadFile(m_Options.m_sSourceFile, uiAssimpFlags);
    if (m_pScene == nullptr)
    {
      ezLog::Error("Assimp failed to import '{}'", m_Options.m_sSourceFile);
      return EZ_FAILURE;
    }

    if (m_pScene->mMetaData != nullptr)
    {
      float fUnitScale = 1.0f;

      if (m_pScene->mMetaData->Get("UnitScaleFactor", fUnitScale))
      {
        // Only FBX files have this unit scale factor and the default unit for FBX is cm. We want meters.
        fUnitScale /= 100.0f;

        ezMat3 s = ezMat3::MakeScaling(ezVec3(fUnitScale));

        m_Options.m_RootTransform = s * m_Options.m_RootTransform;
      }
    }

    if (aiNode* node = m_pScene->mRootNode)
    {
      ezMat4 tmp;
      tmp.SetIdentity();
      tmp.SetRotationalPart(m_Options.m_RootTransform);
      tmp.Transpose(); // aiMatrix4x4 is row-major

      const aiMatrix4x4 transform = reinterpret_cast<aiMatrix4x4&>(tmp);

      node->mTransformation = transform * node->mTransformation;
    }

    EZ_SUCCEED_OR_RETURN(ImportMaterials());

    EZ_SUCCEED_OR_RETURN(TraverseAiScene());

    EZ_SUCCEED_OR_RETURN(PrepareOutputMesh());

    EZ_SUCCEED_OR_RETURN(ImportAnimations());

    EZ_SUCCEED_OR_RETURN(ImportBoneColliders(nullptr));

    if (m_Options.m_pMeshOutput)
    {
      if (m_Options.m_bRecomputeNormals)
      {
        if (m_Options.m_pMeshOutput->MeshBufferDesc().RecomputeNormals().Failed())
        {
          ezLog::Error("Recomputing the mesh normals failed.");
          // do not return failure here, because we can still continue
        }
      }

      if (m_Options.m_bRecomputeTangents)
      {
        if (RecomputeTangents().Failed())
        {
          ezLog::Error("Recomputing the mesh tangents failed.");
          // do not return failure here, because we can still continue
        }
      }
    }

    if (m_pScene->mNumTextures > 0 && m_pScene->mTextures)
    {
      ezStringBuilder refName;

      for (ezUInt32 i = 0; i < m_pScene->mNumTextures; ++i)
      {
        const auto& st = *m_pScene->mTextures[i];

        refName.SetFormat("*{}", i);

        auto& tex = m_OutputTextures[refName];
        tex.m_sFilename = st.mFilename.C_Str();
        if (tex.m_sFilename.IsEmpty())
        {
          tex.m_sFilename = refName;
        }

        if (st.mHeight == 0 && st.mWidth > 0)
        {
          tex.m_sFileFormatExtension = st.achFormatHint;
          tex.m_RawData = ezMakeArrayPtr((const ezUInt8*)st.pcData, st.mWidth);
        }
      }
    }

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::TraverseAiScene()
  {
    if (m_Options.m_pSkeletonOutput != nullptr)
    {
      m_Options.m_pSkeletonOutput->m_Children.PushBack(EZ_DEFAULT_NEW(ezEditableSkeletonJoint));
      EZ_SUCCEED_OR_RETURN(TraverseAiNode(m_pScene->mRootNode, ezMat4::MakeIdentity(), m_Options.m_pSkeletonOutput->m_Children.PeekBack()));
    }
    else
    {
      EZ_SUCCEED_OR_RETURN(TraverseAiNode(m_pScene->mRootNode, ezMat4::MakeIdentity(), nullptr));
    }

    return EZ_SUCCESS;
  }

  ezResult ImporterAssimp::TraverseAiNode(aiNode* pNode, const ezMat4& parentTransform, ezEditableSkeletonJoint* pCurJoint)
  {
    ezMat4 invTrans = parentTransform;
    EZ_ASSERT_DEBUG(invTrans.Invert(0.0f).Succeeded(), "inversion failed");

    const ezMat4 localTransform = ConvertAssimpType(pNode->mTransformation);
    const ezMat4 globalTransform = parentTransform * localTransform;

    if (pCurJoint)
    {
      pCurJoint->m_sName.Assign(pNode->mName.C_Str());
      pCurJoint->m_LocalTransform = ezTransform::MakeFromMat4(localTransform);
    }

    if (pNode->mNumMeshes > 0)
    {
      for (ezUInt32 meshIdx = 0; meshIdx < pNode->mNumMeshes; ++meshIdx)
      {
        EZ_SUCCEED_OR_RETURN(ProcessAiMesh(m_pScene->mMeshes[pNode->mMeshes[meshIdx]], globalTransform));
      }
    }

    for (ezUInt32 childIdx = 0; childIdx < pNode->mNumChildren; ++childIdx)
    {
      if (pCurJoint)
      {
        pCurJoint->m_Children.PushBack(EZ_DEFAULT_NEW(ezEditableSkeletonJoint));

        EZ_SUCCEED_OR_RETURN(TraverseAiNode(pNode->mChildren[childIdx], globalTransform, pCurJoint->m_Children.PeekBack()));
      }
      else
      {
        EZ_SUCCEED_OR_RETURN(TraverseAiNode(pNode->mChildren[childIdx], globalTransform, nullptr));
      }
    }

    return EZ_SUCCESS;
  }


  ezResult ImporterAssimp::ImportBoneColliders(ezEditableSkeletonJoint* pJoint)
  {
    if (m_Options.m_pSkeletonOutput == nullptr)
      return EZ_SUCCESS;

    if (pJoint == nullptr)
    {
      for (ezEditableSkeletonJoint* pJoint : m_Options.m_pSkeletonOutput->m_Children)
      {
        EZ_SUCCEED_OR_RETURN(ImportBoneColliders(pJoint));
      }

      return EZ_SUCCESS;
    }
    else
    {
      for (ezEditableSkeletonJoint* pChild : pJoint->m_Children)
      {
        EZ_SUCCEED_OR_RETURN(ImportBoneColliders(pChild));
      }
    }

    ezStringBuilder sTmp;

    const ezString& sName = pJoint->m_sName.GetString();

    for (auto meshIt : m_MeshInstances)
    {
      for (const MeshInstance& meshInst : meshIt.Value())
      {
        auto pMesh = meshInst.m_pMesh;

        if (ezStringUtils::FindSubString(pMesh->mName.C_Str(), sName) != nullptr)
        {
          sTmp = pMesh->mName.C_Str();

          if (sTmp.TrimWordStart("UCX_") && sTmp.TrimWordStart(sName) && (sTmp.IsEmpty() || sTmp.TrimWordStart("_")))
          {
            // mesh is named "UCX_BoneName_xyz" or "UCX_BoneName" -> use mesh as convex collider for this bone

            EZ_ASSERT_DEV(pMesh->HasPositions(), "TODO: early out");
            EZ_ASSERT_DEV(pMesh->HasFaces(), "TODO: early out");

            ezEditableSkeletonBoneCollider& col = pJoint->m_BoneColliders.ExpandAndGetRef();
            col.m_sIdentifier = pMesh->mName.C_Str();
            col.m_TriangleIndices.Reserve(pMesh->mNumFaces * 3);
            col.m_VertexPositions.Reserve(pMesh->mNumVertices);

            for (ezUInt32 v = 0; v < pMesh->mNumVertices; ++v)
            {
              col.m_VertexPositions.PushBack(meshInst.m_GlobalTransform * ConvertAssimpType(pMesh->mVertices[v]));
            }

            for (ezUInt32 f = 0; f < pMesh->mNumFaces; ++f)
            {
              col.m_TriangleIndices.PushBack(pMesh->mFaces[f].mIndices[0]);
              col.m_TriangleIndices.PushBack(pMesh->mFaces[f].mIndices[1]);
              col.m_TriangleIndices.PushBack(pMesh->mFaces[f].mIndices[2]);
            }
          }
          else
          {
            // ezLog::Error("TODO: error message");
          }
        }
      }
    }

    return EZ_SUCCESS;
  }

} // namespace ezModelImporter2
