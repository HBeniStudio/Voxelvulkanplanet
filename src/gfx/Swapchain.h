#pragma once

#include "gfx/VkContext.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <vector>

namespace voxel::gfx {

class Swapchain {
public:
    Swapchain(VkContext& ctx, GLFWwindow* window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    void recreateIfNeeded();
    [[nodiscard]] VkRenderPass renderPass() const { return renderPass_; }
    [[nodiscard]] VkFramebuffer framebuffer(uint32_t imageIndex) const { return framebuffers_[imageIndex]; }
    [[nodiscard]] VkExtent2D extent() const { return extent_; }
    [[nodiscard]] VkResult acquireNextImage(VkSemaphore signalSemaphore, uint32_t* imageIndex);
    void present(uint32_t imageIndex, VkSemaphore waitSemaphore);

private:
    VkContext& ctx_;
    GLFWwindow* window_ = nullptr;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat colorFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    std::vector<VkFramebuffer> framebuffers_;
    VkImage depthImage_ = VK_NULL_HANDLE;
    VmaAllocation depthAllocation_ = VK_NULL_HANDLE;
    VkImageView depthView_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;

    void cleanupSwapchain();
    void createSwapchain();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
};

} // namespace voxel::gfx
