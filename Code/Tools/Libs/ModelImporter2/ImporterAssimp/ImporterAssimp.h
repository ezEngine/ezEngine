#pragma once

#include <ModelImporter2/Importer/Importer.h>

#include <assimp/Importer.hpp>

class ezEditableSkeletonJoint;
struct aiNode;
struct aiMesh;

namespace ezModelImporter2
{
  class ImporterAssimp : public Importer
  {
  public:
    ImporterAssimp();
    ~ImporterAssimp();

  protected:
    virtual ezResult DoImport() override;

  private:
    ezResult TraverseAiScene();

    ezResult PrepareOutputMesh();
    ezResult RecomputeTangents();

    ezResult TraverseAiNode(aiNode* pNode, const ezMat4& parentTransform, ezEditableSkeletonJoint* pCurJoint);
    ezResult ProcessAiMesh(aiMesh* pMesh, const ezMat4& transform);

    ezResult ImportMaterials();
    ezResult ImportAnimations();

    ezResult ImportBoneColliders(ezEditableSkeletonJoint* pJoint);

    void SimplifyAiMesh(aiMesh* pMesh);

    Assimp::Importer m_Importer;
    const aiScene* m_pScene = nullptr;
    ezUInt32 m_uiTotalMeshVertices = 0;
    ezUInt32 m_uiTotalMeshTriangles = 0;

    struct MeshInstance
    {
      ezMat4 m_GlobalTransform;
      aiMesh* m_pMesh;
    };

    ezMap<ezUInt32, ezHybridArray<MeshInstance, 4>> m_MeshInstances;

    ezSet<aiMesh*> m_OptimizedMeshes;
  };

  extern ezColor ConvertAssimpType(const aiColor3D& value, bool bInvert = false);
  extern ezColor ConvertAssimpType(const aiColor4D& value, bool bInvert = false);
  extern ezMat4 ConvertAssimpType(const aiMatrix4x4& value, bool bDummy = false);
  extern ezVec3 ConvertAssimpType(const aiVector3D& value, bool bDummy = false);
  extern ezQuat ConvertAssimpType(const aiQuaternion& value, bool bDummy = false);
  extern float ConvertAssimpType(float value, bool bDummy = false);
  extern int ConvertAssimpType(int value, bool bDummy = false);

} // namespace ezModelImporter2
