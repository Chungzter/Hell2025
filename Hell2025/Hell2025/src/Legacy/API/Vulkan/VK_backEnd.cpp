#include <glad/gl.h>
#include <glfw/glfw3.h>

#include "VK_backend.h"
#include "Managers/vK_device_manager.h"
#include "Managers/vK_command_manager.h"
#include "Managers/vK_frame_manager.h"
#include "Managers/vK_swapchain_manager.h"
#include "Managers/vK_synchronization_manager.h"
#include "Managers/vk_texture_manager.h"
#include "vk_util.h"
#include <iostream>

namespace VulkanBackEnd {

    UploadContext g_uploadContext;

    GLFWwindow* g_window;
    int WIDTH = 1280;
    int HEIGHT = 720;

    bool Init() {

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        g_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

        while (!glfwWindowShouldClose(g_window)) {
            glfwPollEvents();
        }

        glfwDestroyWindow(g_window);

        glfwTerminate();

        return true;
    }

    void Destroy() {
        VulkanDeviceManager::Destroy();
        VulkanSwapchainManager::Destroy();
    }

    void UpdateTextureBaking() {

    }

    void AllocateTextureMemory(Texture& texture) {
        VulkanTextureManager::AllocateTexture(texture);
    }

    UploadContext& GetUploadContext() {
        return g_uploadContext;
    }
}
