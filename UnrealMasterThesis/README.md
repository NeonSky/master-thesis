# UnrealMasterThesis

## Structure

The code is split into 2 modules:
- `UnrealMasterThesis` - the primary game module of the project.
- `ShaderModels` - contains a C++ model/interface for each global `.usf` shader in `/Shaders`.

The `UnrealMasterThesis` module depends on `ShaderModels`, while `ShaderModels` only depends on engine features and the shaders in `/Shaders`.

## Development

Initial setup steps:
1. Run `/opt/unreal-engine/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh /home/neonsky/main/local/projects/UnrealMasterThesis/UnrealMasterThesis.uproject -game` to generate the `Intermediate/ProjectFiles` directory.
2. Open project with the UE4 editor.

- To recompile a change made in `ShaderModels`, you must (re)start the UE4 editor.
- To recompile non-breaking changes to `.usf` shaders, run `recompileshaders changed` from the UE4 console.
- To recompile changes to `UnrealMasterThesis`, just hit Compile from within the editor like normal.
- The fastest way to check for compile errors in any module is by running `make UnrealMasterThesisEditor`.

The `ShaderModels` module cannot be recompiled from within the editor because all shaders must be loaded before the engine starts. Consequently, this module has loading phase `PostConfigInit` in the `.uproject` file.

