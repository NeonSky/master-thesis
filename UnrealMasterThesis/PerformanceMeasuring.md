# Peformance Measuring

As a prerequisite, go to `Project Settings -> Engine -> General Settings -> Framerate`.

Then, set
- `Use Fixed Frame Rate` to `true`;
- and `Fixed Frame Rate` to something very high that the idle editor will not even reach (e.g. `200`).

## Measurement Procedure

For each setup,
1. resize the viewport to `1920x1080`.
2. enter `Play` mode with `Alt+P`.
3. enter `Posses` mode with `F8`.
4. screenshot with relevant `stat` information open.
5. use `stat fps` and `stat unit`, ~~`stat rhicmdlist`, `stat unit`, and `stat gpu`~~
6. run `viewmode unlit` (alt. press `F2`)

If the display is `1920x1080`, then simply hitting `F11` should be enough. The resolution can be verified by hitting the `V` key in `Play` mode.

For the play moade, use `Selected Viewport`.

To take a screenshot, either press `P` or configure `InputPawn -> Take Screenshot Of Frame` to a fixed value.

### Setup #1 (empty scene)

* Ensure `OceanSurfaceSimulation -> Should Simulate` is `false`.

### Setup #2 (non-interactive ocean)

* Ensure `OceanSurfaceSimulation -> Should Simulate` is `true`.
* Ensure `OceanSurfaceSimulation -> Boats` is an empty list. 

### Setup #3 (1 GPU boat)

* Ensure `OceanSurfaceSimulation -> Boats` has one GPU boat (ensure `Camera Follow` is `false`)
* Ensure `OceanSurfaceSimulation -> Should Update Wakes` is `false`

### Setup #4 (2 GPU boats)

* Ensure `OceanSurfaceSimulation -> Boats` has two GPU boats (ensure `Camera Follow` is `false`)
* Ensure `OceanSurfaceSimulation -> Should Update Wakes` is `false`

### Setup #5 (1 GPU boat with wakes)

* Ensure `OceanSurfaceSimulation -> Boats` has one GPU boat (ensure `Camera Follow` is `false`)
* Ensure `OceanSurfaceSimulation -> Should Update Wakes` is `true`

### Setup #6 (2 GPU boats with wakes)

* Ensure `OceanSurfaceSimulation -> Boats` has two GPU boats (ensure `Camera Follow` is `false`)
* Ensure `OceanSurfaceSimulation -> Should Update Wakes` is `true`

### Setup #7 (1 CPU boat with wakes)

* Ensure `OceanSurfaceSimulation -> Boats` has one CPU boat
* Ensure `OceanSurfaceSimulation -> Should Update Wakes` is `true`

### Setup #8 (2 CPU boats with wakes)

* Ensure `OceanSurfaceSimulation -> Boats` has two CPU boats
* Ensure `OceanSurfaceSimulation -> Should Update Wakes` is `true`

## References
- https://docs.unrealengine.com/4.26/en-US/TestingAndOptimization/PerformanceAndProfiling/StatCommands/