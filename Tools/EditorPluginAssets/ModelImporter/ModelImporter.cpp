#include <PCH.h>
#include <EditorPluginAssets/ModelImporter/ModelImporter.h>
#include <EditorPluginAssets/ModelImporter/ImporterImplementation.h>
#include <EditorPluginAssets/ModelImporter/Node.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Time/Stopwatch.h>

EZ_IMPLEMENT_SINGLETON(ezModelImporter::Importer);

namespace ezModelImporter
{
  Importer::Importer()
    : m_SingletonRegistrar(this)
  {
  }

  Importer::~Importer()
  {
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  void ValidateSceneGraph(Scene* scene)
  {
    if (!scene)
      return;

    ezHashSet<const Node*> visitedNodes;
    ezDynamicArray<const Node*> queue;

    for (const HierarchyObject* object : scene->GetRootObjects())
    {
      //EZ_ASSERT_DEBUG(!object->GetParent().IsValid(), "Root object has a parent!");

      const Node* currentNode = object->Cast<Node>();
      if (currentNode)
      {
        visitedNodes.Clear();
        queue.Clear();
        queue.PushBack(currentNode);

        while (!queue.IsEmpty())
        {
          currentNode = queue.PeekBack();
          queue.PopBack();
          EZ_ASSERT_DEBUG(visitedNodes.Insert(currentNode) == false, "Imported scene contains cycles in its graph.");

          for (ObjectHandle child : currentNode->m_Children)
          {
            //EZ_ASSERT_DEBUG(scene->GetObject(object->GetParent()) == currentNode, "Child's parent pointer is incorrect!");

            const Node* childNode = object->Cast<Node>();
            if (childNode)
              queue.PushBack(childNode);
          }
        }
      }
    }
  }
#endif

  ezSharedPtr<Scene> Importer::ImportScene(const char* szFileName, bool addToCache)
  {
    ezLogBlock logBlock("Scene Import", szFileName);

    ezSharedPtr<Scene> scene;
    if (m_cachedScenes.TryGetValue(szFileName, scene))
      return scene;

    ezString fileExtension = ezPathUtils::GetFileExtension(szFileName);
    if (fileExtension.IsEmpty())
    {
      ezLog::Error("Unable to choose model importer since file '{0}' has no file extension.", szFileName);
      return nullptr;
    }

    // Newer implementations have higher priority.
    for(int i=m_ImporterImplementations.GetCount() - 1; i >= 0; --i)
    {
      auto supportedFormats = m_ImporterImplementations[i]->GetSupportedFileFormats();
      if (std::any_of(cbegin(supportedFormats), cend(supportedFormats), [fileExtension](const char* ext)
          { return ezStringUtils::IsEqual_NoCase(ext, fileExtension); }))
      {
        ezStopwatch timer;
        scene = m_ImporterImplementations[i]->ImportScene(szFileName);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        ValidateSceneGraph(scene);
#endif
        if (scene)
        {
          scene->RefreshRootList();
          scene->CreateUniqueNames();
        }

        ezLog::Success("Scene '{0}' has been imported (time {1}.2s)", szFileName, timer.GetRunningTotal().GetSeconds());
        if (addToCache)
          m_cachedScenes.Insert(szFileName, scene);

        return scene;
      }
    }

    return nullptr;
  }

  void Importer::ClearCachedScenes()
  {
    m_cachedScenes.Clear();
  }

  void Importer::AddImporterImplementation(ezUniquePtr<ImporterImplementation> importerImplementation)
  {
    EZ_ASSERT_DEBUG(importerImplementation != nullptr, "Given importer implementation is null!");
    m_ImporterImplementations.PushBack(std::move(importerImplementation));
  }

  ezHashSet<ezString> Importer::GetSupportedTypes()
  {
    ezHashSet<ezString> supportedFormats;
    for (auto& importer : m_ImporterImplementations)
    {
      ezArrayPtr<const ezString> formats = importer->GetSupportedFileFormats();
      for (const ezString& format : formats)
      {
        supportedFormats.Insert(format);
      }
    }
    return supportedFormats;
  }
}
