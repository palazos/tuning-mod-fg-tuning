#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>

#if defined(FG_TUNING_BUILD)
#define FG_TUNING_EXPORT __attribute__((visibility("default")))
#else
#define FG_TUNING_EXPORT
#endif

// Frame generation layer for the Vulkan renderer.
//
// The renderer composes the X11 scene into an offscreen image instead of the
// swapchain, estimates motion between the last two real frames on the GPU and
// presents interpolated frames in between. Everything lives in this folder;
// the renderer only calls the hooks below.

class VulkanRendererContext;
struct FrameGenState;

// Redirects VulkanRendererContext::recordCmdBuf. A default-constructed value
// (or a null pointer) leaves the normal swapchain path untouched.
struct FrameGenPass {
    VkRenderPass  renderPass  = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    // Draw the pre-composed scene 1:1 (no filters) and add the cursor on top.
    bool          compose     = false;
};

struct FrameGenStats {
    float srcFps = 0.f;
    float outFps = 0.f;
    float gpuMs = 0.f;
    float addedLatencyMs = 0.f;
    int   generated = 0;  // generated frames per real frame
    int   state = 0;      // 0 off, 1 active, 2 unavailable, 3 GPU overload (normal renderer),
                          // 4 active with motion quality lowered for the GPU
    int   quality = 0;    // motion quality in use
};

// Called from renderFrame. Returns true when the frame was fully handled.
//
// Ludashi 5 X++ (LUDASHI_NATIVE_X11) calls renderFrame from its present thread
// and drops the frame unless two things happen, in this order:
//   1. The swapchain image is acquired, then nativeFrameReady latches the scene.
//      The latch is what uploads the X11 snapshot. It has to stay after acquire:
//      that is the point where the previous slot is finished and this image is ours.
//   2. slotSerial of the slot Draw captured (currentFrame on entry) changes.
//      Draw registers one fence, the one for that slot. Tuning therefore reuses
//      the slot for every present inside the call and advances currentFrame once
//      afterwards. The Java server leaves nativeFrameReady null, so none of this
//      runs there and the old present order stays.
#ifdef __cplusplus
extern "C" {
#endif

// Plugin build exports FgTuning_* so they never collide with libfg_loader stubs
// that keep the VulkanRendererFrameGen* names the renderer already calls.
#if defined(FG_TUNING_BUILD)
FG_TUNING_EXPORT bool FgTuning_Render(VulkanRendererContext* ctx);
FG_TUNING_EXPORT void FgTuning_NoteContent(VulkanRendererContext* ctx, int width, int height);
FG_TUNING_EXPORT void FgTuning_Configure(VulkanRendererContext* ctx, bool enabled, int mode,
                                         int quality, int multiplier, float targetHz);
FG_TUNING_EXPORT FrameGenStats FgTuning_GetStats(VulkanRendererContext* ctx);
FG_TUNING_EXPORT void FgTuning_NoteVsync(int64_t frameTimeNs);
FG_TUNING_EXPORT bool FgTuning_SourceUncapped();
FG_TUNING_EXPORT int FgTuning_SourceCap();
FG_TUNING_EXPORT void FgTuning_Destroy(VulkanRendererContext* ctx);
// Dynamic output rate in fps, 0 for the panel refresh. Optional: older plugins lack it.
FG_TUNING_EXPORT void FgTuning_SetDynamicTarget(float fps);
#else
bool VulkanRendererFrameGenRender(VulkanRendererContext* ctx);
void VulkanRendererFrameGenNoteContent(VulkanRendererContext* ctx, int width, int height);
void VulkanRendererFrameGenConfigure(VulkanRendererContext* ctx, bool enabled, int mode,
                                     int quality, int multiplier, float targetHz);
FrameGenStats VulkanRendererFrameGenGetStats(VulkanRendererContext* ctx);
void VulkanRendererFrameGenNoteVsync(int64_t frameTimeNs);
bool VulkanRendererFrameGenSourceUncapped();
int VulkanRendererFrameGenSourceCap();
void VulkanRendererFrameGenDestroy(VulkanRendererContext* ctx);
void VulkanRendererFrameGenSetDynamicTarget(float fps);
#endif

#ifdef __cplusplus
}
#endif
