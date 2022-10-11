#include "App/App.hpp"
#include "Globals.hpp"
#include "System/Debug/Debug.hpp"

#include "Geometry/Primitives.hpp"

#include "Display/Vulkan/VulkanHelper.hpp"

const DynamicArray<const char *> App::ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
#ifdef NDEBUG
const bool App::EnableValidationLayers = false;
#else
const bool App::EnableValidationLayers = true;
#endif

void App::Initialize()
{
    MainWindow = nullptr;
    FullScreen = false;
    AppName = String("Chia Engine");
}

void App::Finalize()
{
    if (EnableValidationLayers)
        VulkanHelper::DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, nullptr);
    vkDestroyInstance(vulkanInstance, nullptr);
    glfwDestroyWindow(MainWindow);
    MainWindow = nullptr;
    glfwTerminate();
}

WindowHandle App::ConstructWindow(const WindowInfo &info)
{
    if (glfwInit() != GLFW_TRUE)
    {
        PRINTLN_ERR("App: Failed to initialize GLFW.");
        return nullptr;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto winHandle = glfwCreateWindow(info.GetWidth(), info.GetHeight(), NULL, NULL, NULL);
    glfwSetWindowTitle(winHandle, "メーン");
    return winHandle;
}

void App::LoadData()
{
    AddRenderable<Cube>();
}

void App::Update()
{
    for (size_t i = 0; i < pRenderers.Length(); i++)
    {
        pRenderers[i]->Update();
    }
}

void App::Render()
{
    for (size_t i = 0; i < pRenderers.Length(); i++)
    {
        pRenderers[i]->Render(&pObjects[0], pObjects.Length());
    }
}

App::App() : windows(), camera(Point3D(1.5, 1.5, 1.5)), pRenderers(), pObjects()
{
    vulkanInstance = {};
    supportedVKLayers = DynamicArray<VkLayerProperties>();
}

App::~App()
{
    for (size_t i = 0; i < windows.Length(); i++)
    {
        if (windows[i] == MainWindow)
            MainWindow = nullptr;
        glfwDestroyWindow(windows[i]);
    }
    for (size_t i = 0; i < pRenderers.Length(); i++)
        delete pRenderers[i];
    for (size_t i = 0; i < pObjects.Length(); i++)
        delete pObjects[i];
}

bool App::InitializeVulkan()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = APPLICATION_NAME;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Chia Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    FindVKLayers();
    if (EnableValidationLayers && !CheckVKValidationLayers())
        return false;

    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = VulkanHelper::GetGLFWRequiredExtensions(EnableValidationLayers);
    createInfo.enabledExtensionCount = glfwExtensions.Length();
    createInfo.ppEnabledExtensionNames = &glfwExtensions[0];

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    if (EnableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.Length());
        createInfo.ppEnabledLayerNames = &ValidationLayers[0];

        VulkanHelper::PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
    {
        return false;
    }
    return InitializeVulkanDebugMessenger();
}

void App::FindVKLayers()
{
    uint32_t layerCounter;
    vkEnumerateInstanceLayerProperties(&layerCounter, nullptr);
    supportedVKLayers = Array<VkLayerProperties>(layerCounter);
    vkEnumerateInstanceLayerProperties(&layerCounter, &supportedVKLayers[0]);
}

bool App::CheckVKValidationLayers()
{
    for (size_t i = 0; i < ValidationLayers.Length(); i++)
    {
        auto layer = ValidationLayers[i];
        bool layerFound = false;
        for (size_t j = 0; j < supportedVKLayers.Length(); i++)
        {
            const auto &supportedLayer = supportedVKLayers[i];
            if (!strcmp(supportedLayer.layerName, layer))
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound)
            return false;
    }
    return true;
}

bool App::InitializeVulkanDebugMessenger()
{
    if (!EnableValidationLayers)
        return true;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    VulkanHelper::PopulateDebugMessengerCreateInfo(createInfo);
    if (VulkanHelper::CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        PRINTLN_ERR("App: Failed to initialize the Vulkan debug messenger.");
        return false;
    }
    return true;
}

int App::Execute()
{
    Initialize();
    LoadData();
    WindowInfo winInfo("Main", false, 800, 800);
    WindowHandle windowHandle = ConstructWindow(winInfo);
    if (!windowHandle)
    {
        Finalize();
        return EXIT_FAILURE;
    }
    MainWindow = windowHandle;

    if (!InitializeVulkan())
    {
        Finalize();
        return EXIT_FAILURE;
    }
    while (!glfwWindowShouldClose(MainWindow))
    {
        glfwPollEvents();
    }
    Finalize();
    return EXIT_SUCCESS;
}
