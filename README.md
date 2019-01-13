# vsync_test

OK I have spent a completely stupid amount of man hours making an extremely simple program -- drawing a single triangle that follows the mouse cursor with minimal input lag, and with vsync, perfectly smooth.

I am confident I have found at least one pretty serious (at least in my opinion) bug in DWM (or two bugs that are somewhat connected -- I will be describing only one of them here).

When you alt+tab out of a windowed application (there are possibly other triggers too), there's a chance that DWM (or something related to DWM) falls out of sync with either the presentation of the application's frames, or with the underlying hardware layer.

It seems to be connected to the number of other windows that are open (speculation: it has to do with the time it takes to compose that alt+tab screen). On one of 3 machines I needed a substantial amount of other windows (>30), on another just having visual studio open was enough.

When The Bug happens, the application will have 1 additional frame of input lag (regardless of OpenGL/D3D, how many buffers are used, whether the flip presentation model is used, whether DISCARD or SEQUENTIAL is used).

This is because DWM will first drop a frame, and then queue one frame too much.

It is the same symptom as if your application has at least 2 back buffers, and doesn't wait after presenting each frame and before processing input for the next frame using DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT (or an equivalent approach using DwmFlush), however this time it is not your application's swap chain that is clogged.

Frame timings in the application may be completely normal -- the problem is in the underlying DWM.

The application can sometimes resynchronize (and thereby fix The Bug until the next time it chooses to occur) by intentionally dropping a single frame, so that DWM cannot queue another frame exactly once, which will cause the additional frame it had queued up to be flushed. If the attempt was successful, input lag will be normal again. It will look like simulation ran twice as fast for exactly one frame, but no frame is actually dropped as far as visible frame rate is concerned.

It is also possible to drop more than one frame, which will increase the chance of success (somehow it doesn't always work), however that will cause visible stutter.

Additionally, it is possible that this method is unable to fix The Bug, although I was only able to reproduce this on one of the 3 machines (I wasn't trying very hard though) -- it just so happened that a youtube video was playing in the background. If The Bug occured while it was playing, it was impossible to undo The Bug even with more extreme methods such as reinitializing D3D, recreating the window, or restarting the entire process. Only when the video was paused, or the containing window minimized, it was possible to undo The Bug using the method described above.

If the method failed to undo The Bug (either because it just randomly didn't work, or because of the video-in-background thing), the intentionally dropped frame will be visibly dropped as well, and the additionally queued frame in DWM is still stuck there.

One big obvious problem is that you cannot just intentionally drop frames every now and then, because dropping a frame while DWM does *not* have one frame too much in its queue will actually cause visible stutter.

It is possible to detect the moment when The Bug occurs by querying IDXGISwapChain::GetFrameStatistics every frame. Normally, PresentCount will increase by 1 every time Present is called on the swap chain (regardless of whether vblanks were missed or not), and PresentRefreshCount will increase by 1 every frame as well, unless a physical vblank was actually missed, in which case in increases by a bigger number. However when The Bug occurs, both numbers are the same as they were during the last frame.

This however does not tell you whether The Bug is currently in effect or not, it will only tell you whether it just occurred. Most importantly, it cannot be used to detect whether your attempt to fix it by intentionally dropping a frame has worked or not. Interestingly, it seems it's best to wait around 20 or so frames until attempting to drop a frame for good chances to undo The Bug. Trying to fix it directly the next frame after it occurred never seems to work.

Also, unforutnately, IDXGISwapChain::GetFrameStatistics is only available when using one of the flip presentation models (which are only available in relatively recent version of DirectX and Windows), however The Bug affects all presentation models.

A way to detect whether The Bug is currently in effect and needs some serious frame dropping is calling DwmGetCompositionTimingInfo. Normally, the difference between DWM_TIMING_INFO.(either qpcCompose or qpcVBlank) and DXGI_FRAME_STATISTICS.SyncQPCTime is roughly (but not exactly) equal to one period as reported DWM_TIMING_INFO.qpcRefreshPeriod, indicating that DWM is one frame closer to the real VBlank than DirectX. During The Bug, the difference between DWM and DirectX is roughly twice as much. If The Bug was successfully resolved by dropping a frame, the difference is once again only roughly one period.

A slight problem with this approach is that these numbers will also be incorrect if the PC is too slow to present at the screen refresh rate. Other than that, it seemed viable to use the composition timing difference to determine whether The Bug might be present and therefore whether a frame drop may be necessary, however after detecting The Bug, the application should wait a bit before attempting to drop a frame, and it should not continuously try to drop frames if it seems The Bug cannot be resolved.

I only have nVidia systems, it is unclear whether AMD is affected or whether this is at all related to the graphics card manufacturer. I would guess Microsoft is at fault, and this is another piece of evidence that DWM was a summer intern job.

As DWM is not in effect in exclusive fullscreen, and is also not in effect in fullscreen windowed mode with a flip presentation model being active, it seems The Bug cannot occur in these modes.
