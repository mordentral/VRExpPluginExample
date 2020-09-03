// Some copyright should be here...
using System.IO;
using UnrealBuildTool;

public class OpenXRExpansionPlugin : ModuleRules
{
    public OpenXRExpansionPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //bEnforceIWYU = true;

       PublicDefinitions.Add("WITH_OPEN_XR_EXPANSION=1");

		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
		PrivateIncludePaths.AddRange(
			new string[] {
				//"OpenXRHMD/Private",
                ///"OpenXRHMD/Private",
                EngineDir + "/Source/ThirdParty/OpenXR/include"
				//EngineDir + "/Source/Runtime/Renderer/Private",
				//EngineDir + "/Source/Runtime/OpenGLDrv/Private",
				//EngineDir + "/Source/Runtime/VulkanRHI/Private",
				// ... add other private include paths required here ...
			}
			);

        PublicDependencyModuleNames.AddRange(
            new string[] 
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "HeadMountedDisplay",
                "RHI",
                "RenderCore",
				"OpenXR",
                "OpenXRHMD"
               // "ShaderCore",
               // "ProceduralMeshComponent",
               // "VRExpansionPlugin"
               // "EngineSettings"
            });
			
			AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");
    }
}
