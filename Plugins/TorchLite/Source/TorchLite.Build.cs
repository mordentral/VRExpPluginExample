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
        PublicDefinitions.Add("TORCH_STATIC");

        PublicIncludePaths.Add(Path.Combine(LibTorchPath, "include"));
        PublicIncludePaths.Add(Path.Combine(LibTorchPath, "include/torch/csrc/api/include"));
        PublicAdditionalLibraries.Add(Path.Combine(LibTorchPath, "lib", "torch_cpu.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(LibTorchPath, "lib", "c10.lib"));

        RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "torch_cpu.dll"));
        RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "c10.dll"));
        RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "asmjit.dll"));
        RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "fbgemm.dll"));
        RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "libiomp5md.dll"));
        RuntimeDependencies.Add(Path.Combine(LibTorchPath, "lib", "uv.dll"));


        string BinariesPath = Path.Combine(ModuleDirectory, "../.. /../Binaries/Win64/");
        RuntimeDependencies.Add(Path.Combine(BinariesPath, "torch_cpu.dll"), Path.Combine(LibTorchPath, "lib", "torch_cpu.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(BinariesPath, "c10.dll"), Path.Combine(LibTorchPath, "lib", "c10.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(BinariesPath, "asmjit.dll"), Path.Combine(LibTorchPath, "lib", "asmjit.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(BinariesPath, "fbgemm.dll"), Path.Combine(LibTorchPath, "lib", "fbgemm.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(BinariesPath, "libiomp5md.dll"), Path.Combine(LibTorchPath, "lib", "libiomp5md.dll"), StagedFileType.NonUFS);
        RuntimeDependencies.Add(Path.Combine(BinariesPath, "uv.dll"), Path.Combine(LibTorchPath, "lib", "uv.dll"), StagedFileType.NonUFS);


        if (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            PublicDefinitions.Add("_ITERATOR_DEBUG_LEVEL=0");  // Avoid Debug/Release mismatch issues
        }

        bUseRTTI = true;
        bEnableExceptions = true;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "CoreUObject"});
    }
}