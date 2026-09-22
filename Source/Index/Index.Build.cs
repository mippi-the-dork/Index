using UnrealBuildTool;

public class Index : ModuleRules
{
    public Index(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "Slate",
            "SlateCore",
            "InputCore",
            "SceneOutliner",
            "LevelEditor",
            "UnrealEd"
        });
    }
}
