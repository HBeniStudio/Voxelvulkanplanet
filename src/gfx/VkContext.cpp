#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "gfx/VkContext.h"

#include "core/Log.h"

#include <VkBootstrap.h>
#include <spdlog/spdlog.h>

namespace voxel::gfx {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                             const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                             void*) {
    const auto log = voxel::core::Log::logger();
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        log->error("[Vulkan] {}", callbackData->pMessage);
    } else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        log->warn("[Vulkan] {}", callbackData->pMessage);
    } else if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        log->debug("[Vulkan] {}", callbackData->pMessage);
    }
    return VK_FALSE;
}

} // namespace

VkContext::~VkContext() {
    shutdown();
}

bool VkContext::initialize(bool enableValidation) {
    return initializeForSurface(enableValidation, VK_NULL_HANDLE);
}

bool VkContext::initializeForSurface(bool enableValidation, VkSurfaceKHR surface) {
    if (device_ != VK_NULL_HANDLE) {
        return true;
    }

    if (volkInitialize() != VK_SUCCESS) {
        spdlog::error("volkInitialize failed");
        return false;
    }

    const bool headless = surface == VK_NULL_HANDLE;

    vkb::InstanceBuilder builder;
    builder.set_app_name("voxel")
        .require_api_version(1, 2, 0)
        .set_minimum_instance_version(1, 2, 0)
        .set_headless(headless);

    validationEnabled_ = enableValidation;
    if (enableValidation) {
        builder.request_validation_layers().set_debug_callback(debugCallback);
    }

    auto instResult = builder.build();
    if (!instResult) {
        spdlog::error("Vulkan instance: {}", instResult.error().message());
        return false;
    }
    vkbInstance_ = instResult.value();
    instance_ = vkbInstance_.instance;
    debugMessenger_ = vkbInstance_.debug_messenger;
    volkLoadInstance(instance_);

    vkb::PhysicalDeviceSelector selector{vkbInstance_};
    selector.set_minimum_version(1, 2).prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);
    if (headless) {
        selector.require_present(false);
    } else {
        selector.set_surface(surface).require_present(true);
    }

    auto physResult = selector.select();
    if (!physResult) {
        selector.allow_any_gpu_device_type(true);
        physResult = selector.select();
    }
    if (!physResult) {
        spdlog::error("Physical device: {}", physResult.error().message());
        return false;
    }

    vkb::DeviceBuilder deviceBuilder{physResult.value()};
    auto devResult = deviceBuilder.build();
    if (!devResult) {
        spdlog::error("Logical device: {}", devResult.error().message());
        return false;
    }
    vkbDevice_ = devResult.value();
    device_ = vkbDevice_.device;
    physicalDevice_ = vkbDevice_.physical_device.physical_device;

    const auto queuePair = vkbDevice_.get_queue_and_index(vkb::QueueType::graphics);
    if (!queuePair) {
        spdlog::error("Graphics queue: {}", queuePair.error().message());
        return false;
    }
    graphicsQueue_ = queuePair->first;
    graphicsQueueFamily_ = queuePair->second;

    if (!headless) {
        const auto presentPair = vkbDevice_.get_queue_and_index(vkb::QueueType::present);
        if (!presentPair) {
            spdlog::error("Present queue: {}", presentPair.error().message());
            return false;
        }
        presentQueue_ = presentPair->first;
        presentQueueFamily_ = presentPair->second;
    } else {
        presentQueue_ = graphicsQueue_;
        presentQueueFamily_ = graphicsQueueFamily_;
    }

    volkLoadDevice(device_);

    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.physicalDevice = physicalDevice_;
    allocInfo.device = device_;
    allocInfo.instance = instance_;
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_2;

    VmaVulkanFunctions vulkanFunctions{};
    if (vmaImportVulkanFunctionsFromVolk(&allocInfo, &vulkanFunctions) != VK_SUCCESS) {
        spdlog::error("vmaImportVulkanFunctionsFromVolk failed");
        return false;
    }
    allocInfo.pVulkanFunctions = &vulkanFunctions;

    if (vmaCreateAllocator(&allocInfo, &allocator_) != VK_SUCCESS) {
        spdlog::error("vmaCreateAllocator failed");
        return false;
    }

    spdlog::info("Vulkan 1.2 device ready (graphics family {}, present family {})",
                 graphicsQueueFamily_, presentQueueFamily_);
    return true;
}

void VkContext::shutdown() {
    if (allocator_ != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator_);
        allocator_ = VK_NULL_HANDLE;
    }
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
        vkb::destroy_device(vkbDevice_);
        device_ = VK_NULL_HANDLE;
        vkbDevice_ = {};
    }
    if (instance_ != VK_NULL_HANDLE) {
        vkb::destroy_instance(vkbInstance_);
        instance_ = VK_NULL_HANDLE;
        vkbInstance_ = {};
        debugMessenger_ = VK_NULL_HANDLE;
    }
}

} // namespace voxel::gfx
