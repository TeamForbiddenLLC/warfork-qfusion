#ifndef RI_SWAPCHAIN_H
#define RI_SWAPCHAIN_H

#include "ri_types.h"

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
	uint16_t imageCount;
	struct RIWindowHandle_s* windowHandle; 
	struct RIQueue_s* queue;
	uint16_t width, height;
};

int InitRISwapchain(struct RIDevice_s* dev, struct RISwapchainDesc_s* init, struct RISwapchain_s* swapchain);
uint32_t RISwapchainAcquireNextTexture(struct RIDevice_s* dev, struct RISwapchain_s* swapchain);
void RISwapchainPresent(struct RIDevice_s* dev, struct RISwapchain_s* swapchain);
void FreeRISwapchain(struct RIDevice_s* dev, struct RISwapchain_s* swapchain);

static inline bool IsRISwapchainValid( struct RISwapchain_s *swapchain )
{
	return swapchain->imageCount > 0 && swapchain->width > 0 && swapchain->height > 0;
}

#endif
