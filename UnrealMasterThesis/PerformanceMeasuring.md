# Peformance Measuring

This guide shows how to measure performance on `main` without using RDG insights (see report).


## Prerequisites

Go to `Project Settings -> Engine -> General Settings -> Framerate`.

Then,
- set `Use Fixed Frame Rate` to `true`
- set `Fixed Frame Rate` to `500`

Verify by checking the FPS (using `stat fps`) in the editor. It should be significantly above `60` FPS.

Moreover,
- Enable `Measure CPU Cost` on InputPawn.
- Enable `stat fps`.
- Select the fixed ocean seed `7`.
- Ensure the GPU boats have `Camera Follow` disabled.
- Ensure the OceanSurfaceSimulation has `Should Update Wakes` enabled.
- Ensure all data collector stuff is disabled.
- For the play mode, use `Selected Viewport`.

## Overview of Measurements

The following tables presents the measurements we will take.

Full simulation, but not rendered:
|Setting|GPU (ms)| CPU (ms)|
|-|-|-|
|Empty scene|-|-|
|Only ocean|-|-|
|Ocean + 1 CPU-based boat (sync)|-|-|
|Ocean + 1 CPU-based boat (async)|-|-|
|Ocean + 1 GPU-based boat |-|-|
|Ocean + 2 CPU-based boats (sync)|-|-|
|Ocean + 2 CPU-based boats (async)|-|-|
|Ocean + 2 GPU-based boats |-|-|

Full simulation, and rendered (at `1920x1080`):
|Setting|GPU (ms)| CPU (ms)|
|-|-|-|
|Empty scene|-|-|
|Only ocean|-|-|
|Ocean + 1 CPU-based boat (sync)|-|-|
|Ocean + 1 CPU-based boat (async)|-|-|
|Ocean + 1 GPU-based boat |-|-|
|Ocean + 2 CPU-based boats (sync)|-|-|
|Ocean + 2 CPU-based boats (async)|-|-|
|Ocean + 2 GPU-based boats |-|-|

GPU costs of individual features:
|Shader| GPU (ms)|
|-|-|
|Update non-interactive Ocean|-|
|Update GPU boat|-|
|Update wakes|-|
|FFT (256x256) |-|

## Taking A Measurment

For each measurement,
1. resize the viewport to `1920x1080`.
2. enter `Play` mode with `Alt+P`.
3. enter `Posses` mode with `F8`.
4. run `viewmode unlit` (alt. press `F2`)
5. wait until the on-screen debugged CPU cost has stabilized, then write it down.
6. press `P`, then write down the frame cost it captured (stored as `scr.png`).

If your monitor's display is `1920x1080`, then simply hitting `F11` should be enough. The resolution can be verified by hitting the `V` key in `Play` mode.

`P` gives an immediate frame cost. Take the average of 10 such values.
Only start using this after you have have first measured the average CPU cost.

For all settings, the GPU cost will overwhelmingly dominate the CPU cost (even for an empty scene). Thus, based on the approximation that: `frame cost = max(CPU cost, GPU cost)`, we approximate frame cost as GPU cost. In reality, the true GPU costs may be smaller, but they can at least not be larger than what we estimate using this strategy.

As of writing there seems to be a bug (at least on Linux) where the frame cost slowly accumulates when using RDG. If you simply resize the viewport, this accumulation resets. Thus, if you are experiencing issue, then resize once briefly before pressing `P`.

## Measuring Table 1 and 2

For table 1,
- select `Model` of each boat and disable `Visible`.
- enable `Actor Hidden In Game` for OceanSurfaceSimulation.

For table 2,
- select `Model` of each boat and enable `Visible`.
- disable `Actor Hidden In Game` for OceanSurfaceSimulation.

Measure one table at a time.
If you are measuring table 2 immediately after table 1, then you might need to clear the wakes textures.

### Row 1

- Disable `Should Simulate` on OceanSurfaceSimulation.

### Row 2

- Enable `Should Simulate` on OceanSurfaceSimulation.
- Ensure the boat list in OceanSurfaceSimulation is empty.

### Row 3

- Ensure `#define BOAT_COUNT 1`
- Ensure the boat list in OceanSurfaceSimulation only contains one CPU boat.
- Disable `Mock Async Readback` on said boat.

### Row 4

- Ensure `#define BOAT_COUNT 1`
- Ensure the boat list in OceanSurfaceSimulation only contains one CPU boat.
- Enable `Mock Async Readback` on said boat.

### Row 5

- Ensure `#define BOAT_COUNT 1`
- Ensure the boat list in OceanSurfaceSimulation only contains one GPU boat.

### Row 6-8

Same as rows 3-5, but
- Ensure `#define BOAT_COUNT 2`
- Use two of each boat instead of one (ensure that they have identical settings).

## Measuring Table 3

For these, run the Unreal Engine project through RenderDoc.
Then, use the `Ocean + 1 CPU-based boat (async)` (from either table 1 or table 2).
Then, capture a frame of that scene by pressing F12.
Open the captured frame in RenderDoc and click on the clock icon to get timings.

For this table, it is enough to measure each value once, as opposed to taking an average.

If the RenderDoc overlay says "Inactive window", then open RenderDoc and go to the `UE4Editor [PID X]` tab. From there, hit `Cycle Active Window` until the correct window is active.

### Row 1

Go to `WorldTick`, and then sum the timings of the entries between the first `FourierComponents pass` entry (inclusive) and the second `Butterfly Post-process Pass` entry (inclusive) after that.

### Row 2

Go to `WorldTick`, and then sum the timings of the entries between the first `StructuredBufferUpload(SubmergedTriangles)` entry (inclusive) and its subsequent `GPU Boat Pass` entry (inclusive).

### Row 3

Go to `WorldTick`, and then sum the timings of the entries between the first `Obstruction Pass` entry (inclusive) and the third (inclusive), which should be preceeded by a `Scale Pass`.

### Row 4

Go to `WorldTick`, and then sum the timings of the entries between the first `Butterfly FFT Pass` entry (inclusive) and its subsequent `Butterfly Post-process Pass` entry (exclusive).