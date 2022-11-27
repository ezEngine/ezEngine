if(TARGET glfw)
    ez_set_build_flags(glfw)
    ez_set_default_target_output_dirs(glfw)

    mark_as_advanced(FORCE GLFW_BUILD_SHARED_LIBS)
    mark_as_advanced(FORCE GLFW_BUILD_EXAMPLES)
    mark_as_advanced(FORCE GLFW_BUILD_TESTS)
    mark_as_advanced(FORCE GLFW_BUILD_DOCS)
    mark_as_advanced(FORCE GLFW_INSTALL)
    mark_as_advanced(FORCE GLFW_VULKAN_STATIC)


    set_property(GLOBAL APPEND PROPERTY "EXPORT_PROJECTS" glfw)
    
    ez_set_project_ide_folder(glfw "Code/ThirdParty/GLFW")
    ez_set_project_ide_folder(update_mappings "Code/ThirdParty/GLFW")
    
    mark_as_advanced(FORCE GLFW_USE_HYBRID_HPG)
    mark_as_advanced(FORCE USE_MSVC_RUNTIME_LIBRARY_DLL)
endif()