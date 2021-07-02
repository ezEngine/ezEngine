if (TARGET PhysXPlugin)

	# Make sure this project is built when the Editor is built
	ez_add_as_runtime_dependency(PhysXPlugin)

	ez_uwp_add_import_to_sources(PhysXPlugin ezPhysX::Foundation)
	ez_uwp_add_import_to_sources(PhysXPlugin ezPhysX::Common)
	ez_uwp_add_import_to_sources(PhysXPlugin ezPhysX::PhysX)
	
endif()