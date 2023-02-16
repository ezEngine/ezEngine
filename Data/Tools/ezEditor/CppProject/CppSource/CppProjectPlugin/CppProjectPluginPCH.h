#pragma once

#include <Foundation/Basics.h>

#include <CppProjectPlugin/CppProjectPluginDLL.h>

#include <Core/Collection/CollectionResource.h>
#include <Core/Input/InputManager.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Uuid.h>
#include <GameEngine/DearImgui/DearImgui.h>
#include <Imgui/imgui.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <Utilities/DataStructures/GameGrid.h>
