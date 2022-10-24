#include "App/App.hpp"
#include "pch.hpp"
#include "Globals.hpp"
#include "System/Debug/Debug.hpp"
#include "Display/Vulkan/VulkanHelper.hpp"

static const DynamicArray<const char *> ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
static const bool EnableValidationLayers = false;
#else
static const bool EnableValidationLayers = true;
#endif

static bool CreateVulkanInstance()
{
    char appName[1024];
    AppName.ToUTF8(appName, sizeof(appName));
    uint32_t nGLFWExtensions;
    const char **glfwExtensionNames;
    glfwExtensionNames = glfwGetRequiredInstanceExtensions(&nGLFWExtensions);
    return VulkanHelper::CreateInstance(appName, AppVersion, "Chia Engine", EngineVersion, nGLFWExtensions, glfwExtensionNames,
                                 0, NULL, NULL, &VulkanInstance);
}

bool App::Initialize()
{
    MainWindow = nullptr;
    FullScreen = false;

    if (glfwInit() != GLFW_TRUE)
        return false;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    return CreateVulkanInstance();
}

void App::Finalize()
{
    vkDestroyInstance(VulkanInstance, NULL);
    glfwDestroyWindow(MainWindow);
    MainWindow = nullptr;
}

bool App::LoadData()
{
    return true;
}

void App::Update()
{
}

void App::Render()
{
}

App::App() : pMainWindow(nullptr)
{
}

App::~App()
{
}

int App::Execute()
{
    if (!Initialize())
        return EXIT_FAILURE;
    if (!LoadData())
        return EXIT_FAILURE;
    WindowInfo winInfo(String("Main"), false, 800, 800);
    return EXIT_SUCCESS;
}
