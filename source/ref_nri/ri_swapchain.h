#ifndef RI_SWAPCHAIN_H
#define RI_SWAPCHAIN_H

#include "ri_prelude.h"
#include "ri_resource.h"
#include "ri_command.h"

struct RIDevice_s;

enum RISwapchainFormat_e {
	RI_SWAPCHAIN_BT709_G10_16BIT,
	RI_SWAPCHAIN_BT709_G22_8BIT,
	RI_SWAPCHAIN_BT709_G22_10BIT,
	RI_SWAPCHAIN_BT2020_G2084_10BIT
};

enum RIWindowType_e { RI_WINDOW_X11, RI_WINDOW_WIN32, RI_WINDOW_METAL, RI_WINDOW_WAYLAND };

struct RISwapchain_s {
	struct RIQueue_s *presentQueue;
	uint16_t width;
	uint16_t height;
	uint32_t format; // RI_Format_e

	union {
#if ( DEVICE_IMPL_VULKAN )
		struct {
			VkSwapchainKHR swapchain;
			VkSurfaceKHR surface;
			uint32_t imageCount;
			VkImage images[RI_MAX_SWAPCHAIN_IMAGES];
			VkImageView views[RI_MAX_SWAPCHAIN_IMAGES];

			// Acquire semaphores are round-robin: the image index isn't known until the acquire
			// returns, so they can't be indexed by it. Reuse of element k is safe because the submit
			// that waits on it is fenced, and the ring is at least as long as the frames in flight.
			uint32_t acquireIdx;
			VkSemaphore acquireSemaphores[RI_MAX_SWAPCHAIN_IMAGES];

			// Present semaphores MUST be indexed by acquired image index, not by frame-in-flight slot.
			// A binary semaphore signalled by the submit and waited on by vkQueuePresentKHR cannot be
			// re-signalled until the presentation engine's wait has completed, and the only way to
			// observe that is to re-acquire the image it presented -- no fence covers it. Keying them
			// to the image makes reuse safe by construction: present[i] is only signalled again after
			// image i comes back from an acquire.
			VkSemaphore presentSemaphores[RI_MAX_SWAPCHAIN_IMAGES];

			VkColorSpaceKHR imageColorSpace;
			VkPresentModeKHR presentMode;

			uint32_t outOfDate : 1;      // acquire/present reported OUT_OF_DATE/SUBOPTIMAL; swapchain needs rebuild
			uint32_t acquireFailed : 1;  // last acquire failed (OUT_OF_DATE); skip acquire-wait/present this frame
		} vk;
#endif
	};
};

struct RIWindowHandle_s {
	uint8_t type; // RIWindowType_e
	union {
		struct {
    	void* hwnd; // HWND
		} windows;
		struct {
    	void* dpy; // Display*
    	uint64_t window; // Window
		} x11;
		struct {
    	void* display; // wl_display*
    	void* surface; // wl_surface*
		} wayland;
		struct {
    	void* caMetalLayer; // CAMetalLayer*
		} metal;
	};
};

struct RISwapchainDesc_s {
	uint8_t format; // RISwapchainFormat_e
	uint16_t requestImageCount;
	struct RIWindowHandle_s* windowHandle; 
	struct RIQueue_s* queue;
	uint16_t width, height;
};

int InitRISwapchain(struct RIDevice_s* dev, struct RISwapchainDesc_s* init, struct RISwapchain_s* swapchain);
uint32_t RISwapchainAcquireNextTexture(struct RIDevice_s* dev, struct RISwapchain_s* swapchain);
void FreeRISwapchain(struct RIDevice_s* dev, struct RISwapchain_s* swapchain);
uint32_t RISwapchainGetImageCount(struct RISwapchain_s *swapchain);
struct RITextureView_s RISwapchainGetTextureView(struct RISwapchain_s* swapchain, uint32_t index);
// Non-owning view of an acquired swapchain image as an RITexture_s, so it can be passed to the barrier
// and copy commands. cookie stays 0: the swapchain owns these images and has no stable identity to hand
// out for them.
struct RITexture_s RISwapchainGetTexture(struct RISwapchain_s* swapchain, uint32_t index);

int RISwapchainResize(struct RIDevice_s* dev, struct RISwapchain_s* swapchain, uint16_t width, uint16_t height);
VkResult RISwapchainPresent_vk(struct RIDevice_s* dev, struct RISwapchain_s* swapchain, uint32_t index, size_t num_wait_semaphores, VkSemaphore* wait_semaphores );

// Swapchain-owned frame submit: submits the primary command buffer waiting on the swapchain's
// current acquire semaphore (plus any caller-supplied waits), signals the present semaphore for
// imageIndex (and optional timeline), fences on the ring element, then presents. Replaces the
// hand-built submit/present block that used to live in the frontend.
struct RISwapchainFrameSubmitDesc_s {
	uint32_t imageIndex;                        // acquired swapchain image index to present; also selects the present semaphore
	struct RICmd_s* cmd;                        // primary cmd; caller has already called EndRICmd
	struct RICommandRingElement_s* ringElement; // provides the pacing fence (its semaphore is unused here)
	struct RITimeline_s* timeline;              // optional (may be NULL); additionally signalled on submit
#if ( DEVICE_IMPL_VULKAN )
	struct {
		size_t numWaitSemaphores;               // extra waits beyond the acquire semaphore
		VkSemaphoreSubmitInfo* waitSemaphores;  // e.g. resource-upload flush + secondary cmd semaphores
	} vk;
#endif
};
int RISwapchainFrameSubmit(struct RIDevice_s* dev, struct RISwapchain_s* swapchain, struct RIQueue_s* queue, struct RISwapchainFrameSubmitDesc_s* desc);


static inline bool IsRISwapchainValid( struct RISwapchain_s *swapchain )
{
	return RISwapchainGetImageCount(swapchain) > 0 && swapchain->width > 0 && swapchain->height > 0;
}

#endif
