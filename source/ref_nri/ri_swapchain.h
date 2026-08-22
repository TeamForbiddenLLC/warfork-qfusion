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

			// Deliberately full-width and not bitfields. These are written from two threads -- the present
			// thread raises them from the acquire/present results, the main thread raises outOfDate when
			// r_swapinterval changes -- and two 1-bit fields sharing a storage unit are one memory
			// location, so each write would be a read-modify-write that can clobber the other flag.
			// Separate words make the only remaining race "both threads store 1", which is benign; a
			// stale read on the main thread just defers the rebuild by a frame.
			volatile uint32_t outOfDate;      // acquire/present reported OUT_OF_DATE/SUBOPTIMAL; swapchain needs rebuild
			volatile uint32_t acquireFailed;  // last acquire failed (OUT_OF_DATE); skip acquire-wait/present this frame
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

// Present-mode preference. The concrete VkPresentModeKHR is picked from the surface's supported
// list; FIFO is guaranteed by spec and is the fallback for both.
enum RISwapchainPresentPreference_e {
	RI_PRESENT_PREFER_LOW_LATENCY = 0, // IMMEDIATE, then FIFO_RELAXED, then FIFO
	RI_PRESENT_PREFER_VSYNC = 1,       // FIFO_RELAXED, then FIFO
};

struct RISwapchainDesc_s {
	uint8_t format; // RISwapchainFormat_e
	uint8_t presentPreference; // RISwapchainPresentPreference_e
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
// Re-pick the present mode for a live swapchain from a RISwapchainPresentPreference_e. Returns true
// when the mode actually changed, in which case the swapchain is flagged out-of-date and the caller
// must run its rebuild path (shutdown attachments / RISwapchainResize / recreate attachments).
bool RISwapchainSetPresentPreference(struct RIDevice_s* dev, struct RISwapchain_s* swapchain, uint8_t preference);
VkResult RISwapchainPresent_vk(struct RIDevice_s* dev, struct RISwapchain_s* swapchain, uint32_t index, size_t num_wait_semaphores, VkSemaphore* wait_semaphores );

// Frame submit: submits the primary command buffer with the caller's waits and signals, fenced on the
// ring element. It does not touch the swapchain -- the frame renders into an offscreen backbuffer and
// the acquire, the blit into the acquired image and the present all happen on the present thread (see
// r_present_thread.h), which waits on one of the signal semaphores handed in here.
struct RIQueueFrameSubmitDesc_s {
	struct RICmd_s* cmd;                        // primary cmd; caller has already called EndRICmd
	struct RICommandRingElement_s* ringElement; // provides the pacing fence (its semaphore is unused here)
	struct RITimeline_s* timeline;              // optional (may be NULL); additionally signalled on submit
#if ( DEVICE_IMPL_VULKAN )
	struct {
		size_t numWaitSemaphores;                 // e.g. resource-upload flush + secondary cmd semaphores
		VkSemaphoreSubmitInfo* waitSemaphores;
		size_t numSignalSemaphores;               // e.g. the present thread's per-slot frame-done semaphore
		VkSemaphoreSubmitInfo* signalSemaphores;
	} vk;
#endif
};
int RIQueueFrameSubmit(struct RIDevice_s* dev, struct RIQueue_s* queue, struct RIQueueFrameSubmitDesc_s* desc);


static inline bool IsRISwapchainValid( struct RISwapchain_s *swapchain )
{
	return RISwapchainGetImageCount(swapchain) > 0 && swapchain->width > 0 && swapchain->height > 0;
}

#endif
