#pragma once

#define VK_NO_PROTOTYPES
#include <VkBootstrap.h>
#include <volk.h>
#include <vk_mem_alloc.h>

namespace voxel::gfx {

class VkContext {
public:
    VkContext() = default;
    ~VkContext();

    VkContext(const VkContext&) = delete;
    VkContext& operator=(const VkContext&) = delete;

    bool initialize(bool enableValidation = true);
    void shutdown();

    [[nodiscard]] VkInstance instance() const { return instance_; }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const { return physicalDevice_; }
    [[nodiscard]] VkDevice device() const { return device_; }
    [[nodiscard]] VkQueue graphicsQueue() const { return graphicsQueue_; }
    [[nodiscard]] uint32_t graphicsQueueFamily() const { return graphicsQueueFamily_; }
    [[nodiscard]] VmaAllocator allocator() const { return allocator_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = 0;
    VmaAllocator allocator_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;
    bool validationEnabled_ = false;
    vkb::Instance vkbInstance_{};
    vkb::Device vkbDevice_{};
};

} // namespace voxel::gfx
