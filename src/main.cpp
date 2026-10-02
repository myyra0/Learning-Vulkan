#ifndef VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#endif
#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <map>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<char const *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class HelloVulkanApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow *window = nullptr;

    // RAII Vulkan Objects
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;

    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;

    // Device picked stored, added as a new class member
    vk::raii::PhysicalDevice physicalDevice = nullptr;

    // Store device logic handle
    vk::raii::Device device = nullptr;

    // Device queue interface
    vk::raii::Queue graphicsQueue = nullptr;

    void initWindow()
    {
        glfwInit();

        // GLFW was designed for OpenGL, so we must explicitly tell it NOT to create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Disable resizing, look into it later
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        // Initializing the window
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                          vk::DebugUtilsMessageTypeFlagsEXT type,
                                                          const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                          void *pUserData)
    {
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ||
            severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        {
            std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage <<
                    std::endl;
        }

        return vk::False;
    }

    std::vector<const char *> getRequiredInstanceExtensions()
    {
        // Get the required instance extensions from GLFW.
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        // Contacts the Vulkan loader to retrieve a list of every instance-level extension the machine supports
        auto extensionProperties = context.enumerateInstanceExtensionProperties();

        // Check if the required extensions are supported by the Vulkan implementation.
        for (const auto &extension: extensions)
        {
            bool extensionFound = false;
            for (const auto &extensionProperty: extensionProperties)
            {
                if (strcmp(extensionProperty.extensionName, extension) == 0)
                {
                    extensionFound = true;
                    break;
                }
            }
            if (!extensionFound)
            {
                throw std::runtime_error("Required extension not supported: " + std::string(extension));
            }
        }

        return extensions;
    }

    std::vector<const char *> getRequiredInstanceLayers()
    {
        if (!enableValidationLayers)
        {
            return {};
        }

        std::vector<char const *> requiredLayers;
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());

        // Contacts the Vulkan loader to retrieve a list of every validation layer the machine supports
        auto layerProperties = context.enumerateInstanceLayerProperties();

        // Check if the required layers are supported by the system.
        for (const auto &requiredLayer: requiredLayers)
        {
            bool layerFound = false;
            for (const auto &layerProperty: layerProperties)
            {
                if (strcmp(layerProperty.layerName, requiredLayer) == 0)
                {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound)
            {
                throw std::runtime_error("Required layer extension not supported: " + std::string(requiredLayer));
            }
        }
        return requiredLayers;
    }

    void createInstance()
    {
        // Information about our application (Optional but recommended)
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName = "Hello Vulkan",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14
        };

        auto requiredExtensions = getRequiredInstanceExtensions();

        auto requiredLayers = getRequiredInstanceLayers();

        // It tells the Vulkan driver which global extensions and validation layers we want to use
        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };

        // Finally create instance
        instance = vk::raii::Instance(context, createInfo);
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
        {
            return;
        }

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback
        };
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }

    bool checkDeviceSuitable( vk::raii::PhysicalDevice const & pD )
    {
        bool supportsVulkan13 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

        auto queueFamilies = physicalDevice.getQueueFamilyProperties();
        bool supportsGraphics =
                std::ranges::any_of(queueFamilies, [](auto const &qfp)
                {
                    return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);
                });

        std::vector<const char*> requiredDeviceExtension = {vk::KHRSwapchainExtensionName};

        auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions =
                std::ranges::all_of(requiredDeviceExtension,
                                    [&availableDeviceExtensions](auto const &requiredDeviceExtension)
                                    {
                                        return std::ranges::any_of(availableDeviceExtensions,
                                                                   [requiredDeviceExtension](
                                                               auto const &availableDeviceExtension)
                                                                   {
                                                                       return strcmp(
                                                                                  availableDeviceExtension.
                                                                                  extensionName,
                                                                                  requiredDeviceExtension) == 0;
                                                                   });
                                    });


        auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().
                                        extendedDynamicState;

        return supportsVulkan13 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }

    void pickPhysicalDevice()
    {
        auto physicalDevices = instance.enumeratePhysicalDevices();

        if (physicalDevices.empty())
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        // Use a standard vector to store our evaluated devices
        std::vector<std::pair<int, vk::raii::PhysicalDevice> > candidates;

        for (const auto &pd: physicalDevices)
        {
            // Rank suitable devices only
            bool isDeviceSuitable = checkDeviceSuitable(pd);
            if (!isDeviceSuitable)
            {
                continue;
            }

            auto deviceProperties = pd.getProperties();
            auto deviceFeatures = pd.getFeatures();
            uint32_t score = 0;

            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                score += 1000;
            }

            // Maximum possible size of textures affects graphics quality
            score += deviceProperties.limits.maxImageDimension2D;

            if (score > 0)
            {
                candidates.emplace_back(score, pd);
            }
        }

        std::ranges::sort(candidates,
                          [](const auto& a, const auto& b)
                          {
                              return a.first > b.first;
                          });

        if (!candidates.empty())
        {
            physicalDevice = candidates.front().second;
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice()
    {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const &qfp)
        {
            return (qfp.queueFlags &
                    vk::QueueFlagBits::eGraphics) != static_cast
                   <vk::QueueFlags>(0);
        });

        auto graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(),
                                                                 graphicsQueueFamilyProperty));

        float queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };

        // Empty now, come back to it later when needed
        vk::PhysicalDeviceFeatures deviceFeatures;

        // Create a chain of feature structures
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                    vk::PhysicalDeviceVulkan11Features,
                    vk::PhysicalDeviceVulkan13Features,
                    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
                featureChain = {
                    {}, // vk::PhysicalDeviceFeatures2 (empty for now)
                    {.shaderDrawParameters = true}, // Enable shader draw parameters from Vulkan 1.1
                    {.dynamicRendering = true}, // Enable dynamic rendering from Vulkan 1.3
                    {.extendedDynamicState = true} // Enable extended dynamic state from the extension
                };

        std::vector<const char *> requiredDeviceExtension = {
            vk::KHRSwapchainExtensionName
        };

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
            .ppEnabledExtensionNames = requiredDeviceExtension.data()
        };

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);

    }

    void mainLoop()
    {
        bool running = true;
        while (running)
        {
            glfwPollEvents();

            // Check for exit
            if (glfwWindowShouldClose(window))
            {
                running = false;
                break;
            }
        }
    }

    void cleanup()
    {
        // Terminate GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main(int argc, char *argv[])
{
    try
    {
        HelloVulkanApplication app;
        app.run();
    } catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
