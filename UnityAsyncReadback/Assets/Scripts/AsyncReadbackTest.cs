using System.Collections.Generic;
using System.Collections;
using Unity.Collections;
using UnityEngine.Rendering;
using UnityEngine;

// Based on: https://github.com/keijiro/AsyncCaptureTest
public class AsyncReadbackTest : MonoBehaviour {

    public int N = 256;

    (RenderTexture grab, RenderTexture flip) _rt;
    NativeArray<byte> _buffer;

    List<float> starts = new List<float>();
    List<float> ends = new List<float>();

    private float avg; // Enable "Debug" mode in the inspector to see.

    System.Collections.IEnumerator Start() {
        var (w, h) = (N, N);

        this.avg = 0.0f;

        _rt.grab = new RenderTexture(w, h, 0);
        _rt.flip = new RenderTexture(w, h, 0);

        _buffer = new NativeArray<byte>(w * h * 4, Allocator.Persistent, NativeArrayOptions.UninitializedMemory);

        var (scale, offs) = (new Vector2(1, -1), new Vector2(0, 1));

        while (true) {
            yield return new WaitForEndOfFrame();

            if (starts.Count == ends.Count) {

                ScreenCapture.CaptureScreenshotIntoRenderTexture(_rt.grab);
                Graphics.Blit(_rt.grab, _rt.flip, scale, offs);

                starts.Add(Time.realtimeSinceStartup);
                AsyncGPUReadback.RequestIntoNativeArray(ref _buffer, _rt.flip, 0, OnCompleteReadback);
            }
        }
    }

    void OnCompleteReadback(AsyncGPUReadbackRequest request) {
        ends.Add(Time.realtimeSinceStartup);
        Debug.Log("Now (ms): " + 1000.0 * (ends[ends.Count-1] - starts[starts.Count-1]));
        this.avg = 0.0f;
        for (int i = 0; i < starts.Count; i++) {
            this.avg += ends[i] - starts[i];
        }
        this.avg = 1000.0f * (this.avg / starts.Count);

        if (request.hasError) {
            Debug.Log("GPU readback error detected.");
            return;
        }

        using var encoded = ImageConversion.EncodeNativeArrayToPNG(_buffer, _rt.flip.graphicsFormat, (uint)_rt.flip.width, (uint)_rt.flip.height);

        System.IO.File.WriteAllBytes("debug.png", encoded.ToArray());
    }

    void OnDestroy() {
        AsyncGPUReadback.WaitAllRequests();

        Destroy(_rt.flip);
        Destroy(_rt.grab);

        _buffer.Dispose();
    }

}
