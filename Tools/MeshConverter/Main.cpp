#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Core/Application/Application.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <../ThirdParty/AssImp/include/scene.h>
#include <../ThirdParty/AssImp/include/Importer.hpp>
#include <../ThirdParty/AssImp/include/postprocess.h>
#include <../ThirdParty/AssImp/include/Logger.hpp>
#include <../ThirdParty/AssImp/include/LogStream.hpp>
#include <../ThirdParty/AssImp/include/DefaultLogger.hpp>

using namespace Assimp;

class aiLogStream : public LogStream
{
public:
  void write(const char* message)
  {
    ezLog::Dev("AI: %s", message);
  }
};

class ezMeshConverterApp : public ezApplication
{
private:
  ezString m_sInputFile;
  ezString m_sOutputFile;

public:

  virtual void AfterEngineInit() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    if (GetArgumentCount() != 3)
      ezLog::Error("This tool requires exactly two command-line argument: An absolute path to the input and the output mesh.");

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    {
      ezStringBuilder sFile = GetArgument(1);
      sFile.MakeCleanPath();

      if (!ezPathUtils::IsAbsolutePath(sFile))
        ezLog::Error("The given path is not absolute: '%s'", sFile.GetData());

      m_sInputFile = sFile;
    }

    {
      ezStringBuilder sFile = GetArgument(2);
      sFile.MakeCleanPath();

      if (!ezPathUtils::IsAbsolutePath(sFile))
        ezLog::Error("The given path is not absolute: '%s'", sFile.GetData());

      m_sOutputFile = sFile;
    }

    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("");
  }

  virtual void BeforeEngineShutdown()
  {
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual ezApplication::ApplicationExecution Run() override
  {
    ezLog::Info("Input: '%s'", m_sInputFile.GetData());

    Importer importer;

    DefaultLogger::create("", Logger::NORMAL);

    const unsigned int severity = Logger::Debugging | Logger::Info | Logger::Err | Logger::Warn;
    DefaultLogger::get()->attachStream(new aiLogStream(), severity);

    const aiScene* scene = nullptr;

    {
      EZ_LOG_BLOCK("Importing Mesh", m_sInputFile.GetData());

      scene = importer.ReadFile(m_sInputFile.GetData(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_PreTransformVertices | aiProcess_RemoveRedundantMaterials);

      if (!scene)
      {
        ezLog::Error("Could not import file '%s'", m_sInputFile.GetData());
        SetReturnCode(1);
        return ezApplication::Quit;
      }
    }

    ezLog::Success("Mesh has been imported", m_sInputFile.GetData());

    ezLog::Info("Number of unique Meshes: %u", scene->mNumMeshes);

    ezMeshResourceDescriptor desc;

    desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::UVFloat);
    desc.MeshBufferDesc().AddStream(ezGALVertexAttributeSemantic::Normal, ezGALResourceFormat::XYZFloat);

    ezUInt32 uiVertices = 0;
    ezUInt32 uiTriangles = 0;

    for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
    {
      uiVertices += scene->mMeshes[i]->mNumVertices;
      uiTriangles += scene->mMeshes[i]->mNumFaces;
    }

    desc.MeshBufferDesc().AllocateStreams(uiVertices, uiTriangles);

    ezUInt32 uiCurVertex = 0;
    ezUInt32 uiCurTriangle = 0;

    desc.SetMaterial(0, "");

    aiString name;
    ezStringBuilder sMatName;

    for (ezUInt32 i = 0; i < scene->mNumMeshes; ++i)
    {
      desc.AddSubMesh(scene->mMeshes[i]->mNumFaces, uiCurTriangle, scene->mMeshes[i]->mMaterialIndex);

      aiMaterial* mat = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];


      mat->Get(AI_MATKEY_NAME, name);
      sMatName.Format("Materials/%s.material", name.C_Str());
      desc.SetMaterial(scene->mMeshes[i]->mMaterialIndex, sMatName);

      for (ezUInt32 f = 0; f < scene->mMeshes[i]->mNumFaces; ++f, ++uiCurTriangle)
      {
        EZ_ASSERT_DEV(scene->mMeshes[i]->mFaces[f].mNumIndices == 3, "");

        desc.MeshBufferDesc().SetTriangleIndices(uiCurTriangle, uiCurVertex + scene->mMeshes[i]->mFaces[f].mIndices[0], uiCurVertex + scene->mMeshes[i]->mFaces[f].mIndices[1], uiCurVertex + scene->mMeshes[i]->mFaces[f].mIndices[2]);
      }

      for (ezUInt32 v = 0; v < scene->mMeshes[i]->mNumVertices; ++v, ++uiCurVertex)
      {
        desc.MeshBufferDesc().SetVertexData(0, uiCurVertex, ezVec3(scene->mMeshes[i]->mVertices[v].x, scene->mMeshes[i]->mVertices[v].y, scene->mMeshes[i]->mVertices[v].z));
        desc.MeshBufferDesc().SetVertexData(1, uiCurVertex, ezVec2(scene->mMeshes[i]->mTextureCoords[0][v].x, 1.0f - scene->mMeshes[i]->mTextureCoords[0][v].y));
        desc.MeshBufferDesc().SetVertexData(2, uiCurVertex, ezVec3(scene->mMeshes[i]->mNormals[v].x, scene->mMeshes[i]->mNormals[v].y, scene->mMeshes[i]->mNormals[v].z));
      }
    }

    desc.Save(m_sOutputFile);

    SetReturnCode(0);
    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezMeshConverterApp);
