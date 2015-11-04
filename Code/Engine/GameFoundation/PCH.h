#pragma once

// <StaticLinkUtil::StartHere>
// all include's before this will be left alone and not replaced by the StaticLinkUtil
// all include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library



#include <Core/Application/Application.h>
#include <Core/Application/Config/FileSystemConfig.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <Foundation/Basics.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <System/Window/Window.h>

