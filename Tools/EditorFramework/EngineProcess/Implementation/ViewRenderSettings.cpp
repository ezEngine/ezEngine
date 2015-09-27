#include <PCH.h>
#include <EditorFramework/Plugin.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezViewRenderMode, 1)
EZ_BITFLAGS_CONSTANTS(ezViewRenderMode::None, ezViewRenderMode::WireframeColor, ezViewRenderMode::WireframeMonochrome, ezViewRenderMode::TexCoordsUV0, ezViewRenderMode::VertexNormals, ezViewRenderMode::PixelDepth)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSceneViewPerspective, 1)
EZ_BITFLAGS_CONSTANTS(ezSceneViewPerspective::Orhogonal_Front, ezSceneViewPerspective::Orhogonal_Right, ezSceneViewPerspective::Orhogonal_Top, ezSceneViewPerspective::Perspective)
EZ_END_STATIC_REFLECTED_ENUM();





