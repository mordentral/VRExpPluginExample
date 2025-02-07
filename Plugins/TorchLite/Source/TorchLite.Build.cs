using UnrealBuildTool;
using System.IO;
 
public class TorchLite : ModuleRules
{
    private string LibTorchPath
    {
        get { return Path.Combine(ModuleDirectory, "ThirdParty/LibTorch"); }
    }


    public TorchLite(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        //bEnforceIWYU = true;

        // Add any macros that need to be set
        PublicDefinitions.Add("WITH_TLITE=1");


        PublicIncludePaths.Add(Path.Combine(LibTorchPath, "include"));
        PublicIncludePaths.Add(Path.Combine(LibTorchPath, "include/torch/csrc/api/include"));
        PublicAdditionalLibraries.Add(Path.Combine(LibTorchPath, "lib", "torch_cpu.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibTorchPath, "lib", "c10.lib"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
          //  PublicDelayLoadDLLs.Add("torch_cpu.dll");
           // PublicDelayLoadDLLs.Add("c10.dll");
           RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "torch_cpu.dll"));
           RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "c10.dll"));
        }
		/*else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicAdditionalLibraries.Add(Path.Combine(LibTorchPath, "lib", "libtorch.so"));
			PublicAdditionalLibraries.Add(Path.Combine(LibTorchPath, "lib", "libc10.so"));
		}*/
		
		////bUseRTTI = true;
       //// bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "CoreUObject"});
    }
}