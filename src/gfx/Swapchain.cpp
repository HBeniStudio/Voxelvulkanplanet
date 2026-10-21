#include "gfx/Swapchain.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>

namespace voxel::gfx {

namespace {

VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats.front();
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes) {
    for (const auto mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

} // namespace

Swapchain::Swapchain(VkContext& ctx, GLFWwindow* window) : ctx_(ctx), window_(window) {
    if (glfwCreateWindowSurface(ctx.instance(), window_, nullptr, &surface_) != VK_SUCCESS) {
        throw std::runtime_error("glfwCreateWindowSurface failed");
    }
    if (!ctx.initializeForSurface(true, surface_)) {
        throw std::runtime_error("VkContext surface init failed");
    }
    createSwapchain();
}

Swapchain::~Swapchain() {
    VkDevice device = ctx_.device();
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
    cleanupSwapchain();
    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(ctx_.device(), renderPass_, nullptr);
    }
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx_.instance(), surface_, nullptr);
    }
}

void Swapchain::cleanupSwapchain() {
    VkDevice device = ctx_.device();
    VmaAllocator allocator = ctx_.allocator();
    for (VkFramebuffer fb : framebuffers_) {
        vkDestroyFramebuffer(device, fb, nullptr);
    }
    framebuffers_.clear();
    for (VkImageView view : imageViews_) {
        vkDestroyImageView(device, view, nullptr);
    }
    imageViews_.clear();
    if (depthView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device, depthView_, nullptr);
        depthView_ = VK_NULL_HANDLE;
    }
    if (depthImage_ != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, depthImage_, depthAllocation_);
        depthImage_ = VK_NULL_HANDLE;
    }
    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void Swapchain::createRenderPass() {
    if (renderPass_ != VK_NULL_HANDLE) {
        return;
    }
    VkAttachmentDescription color{};
    color.format = colorFormat_;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = VK_FORMAT_D32_SFLOAT;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    std::array<VkAttachmentDescription, 2> attachments = {color, depth};
    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpInfo.pAttachments = attachments.data();
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    if (vkCreateRenderPass(ctx_.device(), &rpInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        throw std::runtime_error("Swapchain render pass failed");
    }
}

void Swapchain::createDepthResources() {
    VkDevice device = ctx_.device();
    VmaAllocator allocator = ctx_.allocator();
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.extent = {extent_.width, extent_.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthImage_, &depthAllocation_, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImage_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &depthView_);
}

void Swapchain::createFramebuffers() {
    VkDevice device = ctx_.device();
    framebuffers_.resize(imageViews_.size());
    for (size_t i = 0; i < imageViews_.size(); ++i) {
        std::array<VkImageView, 2> attachments = {imageViews_[i], depthView_};
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = renderPass_;
        fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbInfo.pAttachments = attachments.data();
        fbInfo.width = extent_.width;
        fbInfo.height = extent_.height;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &framebuffers_[i]);
    }
}

void Swapchain::createSwapchain() {
    VkPhysicalDevice phys = ctx_.physicalDevice();
    VkSurfaceKHR surface = surface_;

    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, formats.data());

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &presentCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &presentCount, presentModes.data());

    const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);
    colorFormat_ = surfaceFormat.format;
    const VkPresentModeKHR presentMode = choosePresentMode(presentModes);

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    extent_.width = std::clamp(static_cast<uint32_t>(width), caps.minImageExtent.width,
                               caps.maxImageExtent.width);
    extent_.height = std::clamp(static_cast<uint32_t>(height), caps.minImageExtent.height,
                                caps.maxImageExtent.height);

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    const uint32_t queueFamilies[] = {ctx_.graphicsQueueFamily(), ctx_.presentQueueFamily()};
    if (ctx_.graphicsQueueFamily() != ctx_.presentQueueFamily()) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilies;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(ctx_.device(), &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateSwapchainKHR failed");
    }

    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(ctx_.device(), swapchain_, &swapImageCount, nullptr);
    images_.resize(swapImageCount);
    vkGetSwapchainImagesKHR(ctx_.device(), swapchain_, &swapImageCount, images_.data());

    imageViews_.resize(images_.size());
    for (size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = colorFormat_;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(ctx_.device(), &viewInfo, nullptr, &imageViews_[i]);
    }

    createRenderPass();
    createDepthResources();
    createFramebuffers();
}

void Swapchain::recreateIfNeeded() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    if (width == 0 || height == 0) {
        return;
    }
    if (static_cast<uint32_t>(width) == extent_.width && static_cast<uint32_t>(height) == extent_.height) {
        return;
    }
    vkDeviceWaitIdle(ctx_.device());
    cleanupSwapchain();
    createSwapchain();
}

VkResult Swapchain::acquireNextImage(VkSemaphore signalSemaphore, uint32_t* imageIndex) {
    const VkResult result =
        vkAcquireNextImageKHR(ctx_.device(), swapchain_, UINT64_MAX, signalSemaphore, VK_NULL_HANDLE, imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateIfNeeded();
        return result;
    }
    return result;
}

void Swapchain::present(uint32_t imageIndex, VkSemaphore waitSemaphore) {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;
    const VkResult result = vkQueuePresentKHR(ctx_.presentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateIfNeeded();
    }
}

} // namespace voxel::gfx
