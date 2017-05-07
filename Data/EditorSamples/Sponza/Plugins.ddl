Plugin
{
	string %Path{"ezEnginePluginAssets"}
	bool %LoadCopy{false}
	string %DependencyOf{"EditorPluginAssets"}
}
Plugin
{
	string %Path{"ezEnginePluginParticle"}
	bool %LoadCopy{false}
	string %DependencyOf{"EditorPluginParticle"}
}
Plugin
{
	string %Path{"ezEnginePluginScene"}
	bool %LoadCopy{false}
	string %DependencyOf{"EditorPluginScene"}
}
Plugin
{
	string %Path{"ezFmodPlugin"}
	bool %LoadCopy{false}
	string %DependencyOf{"EditorPluginFmod"}
}
Plugin
{
	string %Path{"ezParticlePlugin"}
	bool %LoadCopy{false}
	string %DependencyOf{"<manual>","EditorPluginParticle"}
}
Plugin
{
	string %Path{"ezPhysXPlugin"}
	bool %LoadCopy{false}
	string %DependencyOf{"<manual>","EditorPluginPhysX"}
}
