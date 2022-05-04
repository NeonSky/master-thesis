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
- Select the fixed ocean seed `7`.
- Ensure the GPU boats have `Camera Follow` disabled.
- Ensure the OceanSurfaceSimulation has `Should Update Wakes` enabled.

## Overview of Measurements

The following presents the measurements we will take.

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

Finally, the cost of our `FFT` shader, for a `256x256` texture.

## Taking A Measurment

For each measurement,
1. resize the viewport to `1920x1080`.
2. enter `Play` mode with `Alt+P`.
3. enter `Posses` mode with `F8`.
4. enable `stat fps`.
5. run `viewmode unlit` (alt. press `F2`)
6. press `P` for frame cost (stored as `scr.png`).
7. ensure all data collector stuff is disabled.

If your monitor's display is `1920x1080`, then simply hitting `F11` should be enough. The resolution can be verified by hitting the `V` key in `Play` mode.

For the play mode, use `Selected Viewport`.

`P` gives an immediate frame cost. Take the average of 10 such values.
Only start using this after you have have first measured the average CPU cost.

For all settings, the GPU cost will overwhelmingly dominate the CPU cost (even for an empty scene). Thus, based on the approximation that: `frame cost = max(CPU cost, GPU cost)`, we approximate frame cost as GPU cost. In reality, the true GPU costs may be smaller, but they can at least no be larger than what we estimate using this strategy.

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

### Row 1

Let
```
a = GPU cost of "Only ocean" in table 1
b = GPU cost of "Empty scene" in table 1
```

Then, the GPU cost is `a - b`

### Row 2

As in table 1, measure "Ocean + 1 GPU-based boat", but with `Should Update Wakes` disabled. Let us denote this GPU cost as `a`.

Let
```
b = GPU cost of "Only ocean" in table 1
```

Then, the GPU cost is `a - b`

### Row 3

Let
```
a = GPU cost of "Ocean + 1 GPU-based boat" in table 1
b = GPU cost of "Only ocean" in table 1
c = GPU cost of "Update GPU boat" in table 3
```

Then, the GPU cost is `a - (b + c)`

## Measuring FFT

Use the same setup as row 1 of table 1.

First, measure frame cost with `FFTs Per Frame = 11` (see FFTPerformer) and then with `FFTs Per Frame = 10`. The difference yields the FFT cost.

