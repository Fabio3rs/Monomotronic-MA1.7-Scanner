// Dear ImGui: standalone example application for Glfw + Vulkan
//
// Application modified to implement a touch-first automotive ECU diagnostic UI
// based on a detailed UX/UI design.
//

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h" // For ImLerp
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Standard library includes
#include <algorithm>
#include <cmath>
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <vector>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1900) &&                                 \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

// #define APP_USE_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
#endif

// Data
static VkAllocationCallbacks *g_Allocator = nullptr;
static VkInstance g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice g_Device = VK_NULL_HANDLE;
static uint32_t g_QueueFamily = (uint32_t)-1;
static VkQueue g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int g_MinImageCount = 2;
static bool g_SwapChainRebuild = false;

// Global window pointer for TopBar access
static GLFWwindow *g_AppWindow = nullptr;

// Accessor function for window pointer
GLFWwindow *GetAppWindow() { return g_AppWindow; }

// --- Application State & UI Management ---
#include "app_data.h" // global app state (currentScreen, pinnedSensorIndices, etc.)

// Core systems
#include "core/AnimationSystem.h"
#include "core/ECUBackend.h"
#include "core/RecordingManager.h"
#include "core/ScreenManager.h"
#include "core/StateManager.h"
#include "core/ThemeManager.h"
#include "core/TouchGestureHandler.h"

// UI components
#include "ui/BottomNav.h"
#include "ui/TopBar.h"

// Font, color, sensor and simulated-data definitions are centralized in
// app_data.*

// --- UI COMPONENTS ---
static TopBar g_top_bar;
static BottomNav g_bottom_nav;

// Delta time tracking for animations
static double g_last_frame_time = 0.0;

static void glfw_error_callback(int error, const char *description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static void check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

#ifdef APP_USE_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
             uint64_t object, size_t location, int32_t messageCode,
             const char *pLayerPrefix, const char *pMessage, void *pUserData) {
    (void)flags;
    (void)object;
    (void)location;
    (void)messageCode;
    (void)pUserData;
    (void)pLayerPrefix; // Unused arguments
    fprintf(stderr,
            "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n",
            objectType, pMessage);
    return VK_FALSE;
}
#endif // APP_USE_VULKAN_DEBUG_REPORT

static bool
IsExtensionAvailable(const ImVector<VkExtensionProperties> &properties,
                     const char *extension) {
    for (const VkExtensionProperties &p : properties)
        if (strcmp(p.extensionName, extension) == 0)
            return true;
    return false;
}

static VkPhysicalDevice SetupVulkan_SelectPhysicalDevice() {
    uint32_t gpu_count;
    VkResult err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
    check_vk_result(err);
    IM_ASSERT(gpu_count > 0);

    ImVector<VkPhysicalDevice> gpus;
    gpus.resize(gpu_count);
    err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus.Data);
    check_vk_result(err);

    for (VkPhysicalDevice &device : gpus) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            return device;
    }

    if (gpu_count > 0)
        return gpus[0];
    return VK_NULL_HANDLE;
}

static void SetupVulkan(ImVector<const char *> instance_extensions) {
    VkResult err;
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
    volkInitialize();
#endif

    // Create Vulkan Instance
    {
        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateInstanceExtensionProperties(nullptr, &properties_count,
                                               nullptr);
        properties.resize(properties_count);
        err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count,
                                                     properties.Data);
        check_vk_result(err);

        if (IsExtensionAvailable(
                properties,
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
            instance_extensions.push_back(
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        if (IsExtensionAvailable(
                properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
            instance_extensions.push_back(
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            create_info.flags |=
                VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }
#endif

#ifdef APP_USE_VULKAN_DEBUG_REPORT
        const char *layers[] = {"VK_LAYER_KHRONOS_validation"};
        create_info.enabledLayerCount = 1;
        create_info.ppEnabledLayerNames = layers;
        instance_extensions.push_back("VK_EXT_debug_report");
#endif

        create_info.enabledExtensionCount = (uint32_t)instance_extensions.Size;
        create_info.ppEnabledExtensionNames = instance_extensions.Data;
        err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
        check_vk_result(err);
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
        volkLoadInstance(g_Instance);
#endif

#ifdef APP_USE_VULKAN_DEBUG_REPORT
        auto f_vkCreateDebugReportCallbackEXT =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                g_Instance, "vkCreateDebugReportCallbackEXT");
        IM_ASSERT(f_vkCreateDebugReportCallbackEXT != nullptr);
        VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
        debug_report_ci.sType =
            VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        debug_report_ci.pfnCallback = debug_report;
        debug_report_ci.pUserData = nullptr;
        err = f_vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci,
                                               g_Allocator, &g_DebugReport);
        check_vk_result(err);
#endif
    }

    g_PhysicalDevice = SetupVulkan_SelectPhysicalDevice();

    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count,
                                                 nullptr);
        VkQueueFamilyProperties *queues = (VkQueueFamilyProperties *)malloc(
            sizeof(VkQueueFamilyProperties) * count);
        vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count,
                                                 queues);
        for (uint32_t i = 0; i < count; i++)
            if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                g_QueueFamily = i;
                break;
            }
        free(queues);
        IM_ASSERT(g_QueueFamily != (uint32_t)-1);
    }

    {
        ImVector<const char *> device_extensions;
        device_extensions.push_back("VK_KHR_swapchain");
        uint32_t properties_count;
        ImVector<VkExtensionProperties> properties;
        vkEnumerateDeviceExtensionProperties(g_PhysicalDevice, nullptr,
                                             &properties_count, nullptr);
        properties.resize(properties_count);
        vkEnumerateDeviceExtensionProperties(
            g_PhysicalDevice, nullptr, &properties_count, properties.Data);
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
        if (IsExtensionAvailable(properties,
                                 VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
            device_extensions.push_back(
                VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

        const float queue_priority[] = {1.0f};
        VkDeviceQueueCreateInfo queue_info[1] = {};
        queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[0].queueFamilyIndex = g_QueueFamily;
        queue_info[0].queueCount = 1;
        queue_info[0].pQueuePriorities = queue_priority;
        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount =
            sizeof(queue_info) / sizeof(queue_info[0]);
        create_info.pQueueCreateInfos = queue_info;
        create_info.enabledExtensionCount = (uint32_t)device_extensions.Size;
        create_info.ppEnabledExtensionNames = device_extensions.Data;
        err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator,
                             &g_Device);
        check_vk_result(err);
        vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
    }

    {
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator,
                                     &g_DescriptorPool);
        check_vk_result(err);
    }
}

static void SetupVulkanWindow(ImGui_ImplVulkanH_Window *wd,
                              VkSurfaceKHR surface, int width, int height) {
    wd->Surface = surface;

    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily,
                                         wd->Surface, &res);
    if (res != VK_TRUE) {
        fprintf(stderr, "Error no WSI support on physical device 0\n");
        exit(-1);
    }

    const VkFormat requestSurfaceImageFormat[] = {
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
    const VkColorSpaceKHR requestSurfaceColorSpace =
        VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat,
        (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
        requestSurfaceColorSpace);

#ifdef APP_USE_UNLIMITED_FRAME_RATE
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR,
                                        VK_PRESENT_MODE_IMMEDIATE_KHR,
                                        VK_PRESENT_MODE_FIFO_KHR};
#else
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
    wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        g_PhysicalDevice, wd->Surface, &present_modes[0],
        IM_ARRAYSIZE(present_modes));

    IM_ASSERT(g_MinImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator,
        width, height, g_MinImageCount);
}

static void CleanupVulkan() {
    vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef APP_USE_VULKAN_DEBUG_REPORT
    auto f_vkDestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
            g_Instance, "vkCreateDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // APP_USE_VULKAN_DEBUG_REPORT

    vkDestroyDevice(g_Device, g_Allocator);
    vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow() {
    ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData,
                                    g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data) {
    VkResult err;

    VkSemaphore image_acquired_semaphore =
        wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore render_complete_semaphore =
        wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX,
                                image_acquired_semaphore, VK_NULL_HANDLE,
                                &wd->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);

    ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
    {
        err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);
        check_vk_result(err);

        err = vkResetFences(g_Device, 1, &fd->Fence);
        check_vk_result(err);
    }
    {
        err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
        check_vk_result(err);
    }
    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = wd->RenderPass;
        info.framebuffer = fd->Framebuffer;
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        info.clearValueCount = 1;
        info.pClearValues = &wd->ClearValue;
        vkCmdBeginRenderPass(fd->CommandBuffer, &info,
                             VK_SUBPASS_CONTENTS_INLINE);
    }

    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

    vkCmdEndRenderPass(fd->CommandBuffer);
    {
        VkPipelineStageFlags wait_stage =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &image_acquired_semaphore;
        info.pWaitDstStageMask = &wait_stage;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &fd->CommandBuffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &render_complete_semaphore;

        err = vkEndCommandBuffer(fd->CommandBuffer);
        check_vk_result(err);
        err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
        check_vk_result(err);
    }
}

static void FramePresent(ImGui_ImplVulkanH_Window *wd) {
    if (g_SwapChainRebuild)
        return;
    VkSemaphore render_complete_semaphore =
        wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &render_complete_semaphore;
    info.swapchainCount = 1;
    info.pSwapchains = &wd->Swapchain;
    info.pImageIndices = &wd->FrameIndex;
    VkResult err = vkQueuePresentKHR(g_Queue, &info);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        g_SwapChainRebuild = true;
        return;
    }
    check_vk_result(err);
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount;
}

// Main code
int main(int, char **) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    g_AppWindow =
        glfwCreateWindow(1024, 600, "ECU Sensor Monitor", nullptr, nullptr);
    GLFWwindow *window = g_AppWindow; // Keep local reference for main loop
    if (!glfwVulkanSupported()) {
        printf("GLFW: Vulkan Not Supported\n");
        return 1;
    }

    ImVector<const char *> extensions;
    uint32_t extensions_count = 0;
    const char **glfw_extensions =
        glfwGetRequiredInstanceExtensions(&extensions_count);
    for (uint32_t i = 0; i < extensions_count; i++)
        extensions.push_back(glfw_extensions[i]);
    SetupVulkan(extensions);

    VkSurfaceKHR surface;
    VkResult err =
        glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
    check_vk_result(err);

    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    ImGui_ImplVulkanH_Window *wd = &g_MainWindowData;
    SetupVulkanWindow(wd, surface, w, h);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    initSimulatedSensors();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // --- KEY UI/UX STYLE SETUP ---
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(16, 12);
    style.ItemSpacing = ImVec2(12, 12);
    style.ItemInnerSpacing = ImVec2(10, 10);
    style.TouchExtraPadding = ImVec2(8, 8);
    style.GrabMinSize = 50;
    style.WindowRounding = 0.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarSize = 18.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;

    ImGui::StyleColorsDark();

    // Enhanced hover states for touchscreen feedback
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily;
    init_info.Queue = g_Queue;
    init_info.PipelineCache = g_PipelineCache;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.RenderPass = wd->RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_MinImageCount;
    init_info.ImageCount = wd->ImageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = g_Allocator;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    // --- FONT LOADING ---
    // Configure font with oversample for better quality
    ImFontConfig font_config;
    font_config.OversampleH = 2;
    font_config.OversampleV = 2;

    // Include Latin + Font Awesome icon ranges
    static const ImWchar glyph_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2260, 0x2265, // Mathematical Operators (≡ U+2261, ≤, ≥)
        0x25A0, 0x25FF, // Geometric Shapes (●, ▶, ■, etc)
        0xF000, 0xF2FF, // Font Awesome icons (Nerd Font Private Use Area)
        0,
    };

    font_large = io.Fonts->AddFontFromFileTTF(
        PROJECT_DIR "/imgui/misc/fonts/"
                    "RobotoMonoNerdFont-Regular.ttf",
        28.0f, &font_config, glyph_ranges);
    font_huge = io.Fonts->AddFontFromFileTTF(
        PROJECT_DIR "/imgui/misc/fonts/"
                    "RobotoMonoNerdFont-Regular.ttf",
        60.0f, &font_config,
        glyph_ranges); // Use Nerd Font for consistency and icon support
    IM_ASSERT(font_large != nullptr && font_huge != nullptr);

    // DEBUG: Check if glyphs were loaded
    ImFont *font = io.Fonts->Fonts[0];
    printf("=== FONT DEBUG ===\n");
    printf("  U+21BB (↻): %s\n",
           font->FindGlyph(0x21BB) ? "FOUND" : "MISSING!");
    printf("  U+2717 (✗): %s\n",
           font->FindGlyph(0x2717) ? "FOUND" : "MISSING!");
    printf("==================\n");

    // --- INITIALIZE CORE SYSTEMS ---
    // 1. Initialize StateManager (loads persisted settings from SQLite)
    StateManager::Instance().Initialize();

    // 2. Apply theme to ImGui style (dark mode by default)
    ThemeManager::Instance().ApplyThemeToImGui();

    // 3. Initialize screens
    ScreenManager::Instance().Initialize();

    // 4. Initialize UI components
    g_top_bar.SetConnectionStatus(ecuConnected);
    g_top_bar.SetLatency(kLineLatency);
    g_bottom_nav.SetLoggingActive(loggingActive);
    g_bottom_nav.SetDTCCount(static_cast<int>(activeDTCs.size()));

    // 5. Initialize delta time tracking
    g_last_frame_time = ImGui::GetTime();

    ImVec4 clear_color = ImVec4(0.1f, 0.12f, 0.14f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        if (fb_width > 0 && fb_height > 0 &&
            (g_SwapChainRebuild || g_MainWindowData.Width != fb_width ||
             g_MainWindowData.Height != fb_height)) {
            ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
            ImGui_ImplVulkanH_CreateOrResizeWindow(
                g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData,
                g_QueueFamily, g_Allocator, fb_width, fb_height,
                g_MinImageCount);
            g_MainWindowData.FrameIndex = 0;
            g_SwapChainRebuild = false;
        }

        // --- CALCULATE DELTA TIME ---
        const double current_time = ImGui::GetTime();
        const float delta_time =
            static_cast<float>(current_time - g_last_frame_time);
        g_last_frame_time = current_time;

        // --- UPDATE CORE SYSTEMS ---
        // 1. Update animation system (needed for fades, pulses, smooth
        // transitions)
        AnimationSystem::Instance().Update(delta_time);

        // 2. Update gesture detection
        TouchGestureHandler::Instance().Update();

        // 3. Update connection status from global state
        g_top_bar.SetConnectionStatus(ecuConnected);
        g_top_bar.SetLatency(kLineLatency);

        // 4. Update bottom nav badges
        g_bottom_nav.SetLoggingActive(loggingActive);
        g_bottom_nav.SetDTCCount(static_cast<int>(activeDTCs.size()));

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (!graphFrozen) {
            UpdateLiveData();
        }

        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDocking;
        ImGui::Begin("MainApp", nullptr, window_flags);
        ImGui::PopStyleVar(3);

        const float top_bar_height = 30.0f;
        const float bottom_nav_height = 50.0f;
        const float content_height =
            viewport->WorkSize.y - top_bar_height - bottom_nav_height;

        g_top_bar.Render(top_bar_height);

        ImGui::SetCursorPosY(top_bar_height);
        ImGui::BeginChild("ContentArea", ImVec2(0, content_height), false,
                          ImGuiWindowFlags_None);
        {
            ScreenManager::Instance().Update(delta_time);
            ScreenManager::Instance().Render();
        }
        ImGui::EndChild();

        g_bottom_nav.Render(bottom_nav_height);

        // Show ECU loading overlay when backend is starting but not yet
        // connected
        if (!ecuConnected && !simulationModeActive) {
            RenderECULoadingOverlay();
        }

        ImGui::End();

        ImGui::Render();
        ImDrawData *main_draw_data = ImGui::GetDrawData();
        const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f ||
                                        main_draw_data->DisplaySize.y <= 0.0f);
        wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
        wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
        wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
        wd->ClearValue.color.float32[3] = clear_color.w;
        if (!main_is_minimized)
            FrameRender(wd, main_draw_data);

        if (!main_is_minimized)
            FramePresent(wd);
    }

    err = vkDeviceWaitIdle(g_Device);
    check_vk_result(err);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    // --- GRACEFUL SHUTDOWN SEQUENCE (bottom-up) ---
    // Stop ECU backend worker thread first (before destroying resources it
    // uses)
    ECUBackend::Instance().Stop();

    // Stop recording if active (fstream RAII will close on destruction)
    RecordingManager::Instance().StopRecording();

    // --- CLEANUP SCREEN INSTANCES ---
    ScreenManager::Instance().Shutdown();

    // Shutdown StateManager (closes SQLite database)
    StateManager::Instance().Shutdown();

    ImGui::DestroyContext();

    CleanupVulkanWindow();
    CleanupVulkan();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
