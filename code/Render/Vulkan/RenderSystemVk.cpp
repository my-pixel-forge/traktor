#pragma optimize( "", off )
// https://github.com/WilliamLewww/vulkan_ray_tracing_minimal_abstraction/blob/master/headless/src/main.cpp

/*
 * TRAKTOR
 * Copyright (c) 2022-2024 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#if defined(__ANDROID__)
#	include <android_native_app_glue.h>
#endif
#include <cstring>
#include "Core/Log/Log.h"
#include "Core/Misc/Align.h"
#include "Core/Misc/AutoPtr.h"
#include "Core/Misc/Murmur3.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/StringSplit.h"
#include "Core/Misc/TString.h"
#include "Render/VertexElement.h"
#include "Render/Vulkan/AccelerationStructureVk.h"
#include "Render/Vulkan/BufferDynamicVk.h"
#include "Render/Vulkan/BufferStaticVk.h"
#include "Render/Vulkan/ProgramVk.h"
#include "Render/Vulkan/ProgramResourceVk.h"
#include "Render/Vulkan/RenderSystemVk.h"
#include "Render/Vulkan/RenderTargetDepthVk.h"
#include "Render/Vulkan/RenderTargetSetVk.h"
#include "Render/Vulkan/RenderTargetVk.h"
#include "Render/Vulkan/RenderViewVk.h"
#include "Render/Vulkan/TextureVk.h"
#include "Render/Vulkan/VertexLayoutVk.h"
#include "Render/Vulkan/Private/ApiLoader.h"
#include "Render/Vulkan/Private/CommandBuffer.h"
#include "Render/Vulkan/Private/Context.h"
#include "Render/Vulkan/Private/PipelineLayoutCache.h"
#include "Render/Vulkan/Private/Queue.h"
#include "Render/Vulkan/Private/ShaderModuleCache.h"
#include "Render/Vulkan/Private/Utilities.h"
#include "Render/Vulkan/Private/VertexAttributes.h"

#if defined(_WIN32)
#	include "Render/Vulkan/Win32/Window.h"
#elif defined(__LINUX__) || defined(__RPI__)
#	include "Render/Vulkan/Linux/Window.h"
#elif defined(__ANDROID__)
#	include "Core/System/Android/DelegateInstance.h"
#elif defined(__IOS__)
#	include "Render/Vulkan/iOS/Utilities.h"
#endif

namespace traktor::render
{
	namespace
	{

const char* c_validationLayerNames[] = { "VK_LAYER_KHRONOS_validation", nullptr };
#if defined(_WIN32)
const char* c_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils", "VK_KHR_get_physical_device_properties2" };
#elif defined(__LINUX__) || defined(__RPI__)
const char* c_extensions[] = { "VK_KHR_surface", "VK_KHR_xlib_surface", "VK_EXT_debug_utils", "VK_KHR_get_physical_device_properties2" };
#elif defined(__ANDROID__)
const char* c_extensions[] = { "VK_KHR_surface", "VK_KHR_android_surface" };
#elif defined(__MAC__)
const char* c_extensions[] = { "VK_KHR_surface", "VK_EXT_metal_surface", "VK_EXT_debug_utils", "VK_KHR_get_physical_device_properties2" };
#else
const char* c_extensions[] = { "VK_KHR_surface", "VK_EXT_debug_utils", "VK_KHR_get_physical_device_properties2" };
#endif

#if defined(__ANDROID__) || defined(__RPI__)
const char* c_deviceExtensions[] = { "VK_KHR_swapchain" };
#else
const char* c_deviceExtensions[] =
{
	"VK_KHR_swapchain",
	"VK_KHR_storage_buffer_storage_class",	// Required by VK_KHR_16bit_storage, VK_KHR_8bit_storage and VK_KHR_shader_float16_int8.
	"VK_KHR_16bit_storage",
	"VK_KHR_8bit_storage",
	"VK_KHR_shader_non_semantic_info",
	"VK_KHR_shader_float16_int8",
	"VK_EXT_shader_subgroup_ballot",
	"VK_EXT_memory_budget",
	"VK_EXT_descriptor_indexing",
	"VK_KHR_buffer_device_address",

	// Ray tracing
	"VK_KHR_deferred_host_operations",
	"VK_KHR_ray_tracing_pipeline",
	"VK_KHR_acceleration_structure"
};
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	if (pCallbackData && pCallbackData->pMessage)
	{
		std::wstring message = mbstows(pCallbackData->pMessage);
		std::wstring spec;

		const size_t s = message.find(L"The Vulkan spec states: ");
		if (s != message.npos)
		{
			spec = message.substr(s);
			message = message.substr(0, s);
		}

		LogStream* ls = &log::info;
		if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
			ls = &log::warning;
		if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
			ls = &log::error;

		(*ls) << L"-------------------------------------------------------------------------------" << Endl;
		for (auto s : StringSplit< std::wstring >(message, L";"))
			(*ls) << s << Endl;
		if (!spec.empty())
		{
			(*ls) << Endl;
			for (auto s : StringSplit< std::wstring >(spec, L";"))
				(*ls) << s << Endl;
		}
		(*ls) << L"-------------------------------------------------------------------------------" << Endl;
	}
	return VK_FALSE;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkPhysicalDeviceType deviceType)
{
    VkPhysicalDeviceProperties dp;
    // VkPhysicalDeviceFeatures df;
    
	vkGetPhysicalDeviceProperties(device, &dp);
    // vkGetPhysicalDeviceFeatures(device, &df);

	if (dp.deviceType != deviceType)
		return false;

	return true;
}

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"traktor.render.RenderSystemVk", 0, RenderSystemVk, IRenderSystem)

bool RenderSystemVk::create(const RenderSystemDesc& desc)
{
	VkResult result;

#if defined(__LINUX__) || defined(__RPI__)
	if ((m_display = XOpenDisplay(0)) == nullptr)
	{
		log::error << L"Unable to create Vulkan renderer; Failed to open X display" << Endl;
		return false;
	}
#elif defined(__ANDROID__)
	auto app = desc.sysapp.instance->getApplication();
	m_screenWidth = ANativeWindow_getWidth(app->window);
	m_screenHeight = ANativeWindow_getHeight(app->window);
#endif

	if (!initializeVulkanApi())
	{
		log::error << L"Unable to create Vulkan renderer; Failed to initialize Vulkan API." << Endl;
		return false;
	}

	// Check for available layers.
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, 0);

	AutoArrayPtr< VkLayerProperties > layersAvailable(new VkLayerProperties[layerCount]);
	vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable.ptr());

	AlignedVector< const char* > validationLayers;
	if (desc.validation)
	{
		for (uint32_t i = 0; i < layerCount; ++i)
		{
			bool found = false;
			for (uint32_t j = 0; c_validationLayerNames[j] != nullptr; ++j)
			{
				if (strcmp(layersAvailable[i].layerName, c_validationLayerNames[j]) == 0)
					found = true;
			}
			if (found)
				validationLayers.push_back(strdup(layersAvailable[i].layerName));
		}
	}

	// Create Vulkan instance.
	VkApplicationInfo ai = {};
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pNext = nullptr;
	ai.pApplicationName = "Traktor";
	ai.pEngineName = "Traktor";
	ai.engineVersion = 1;
	ai.apiVersion = VK_MAKE_VERSION(1, 2, 0);

	VkInstanceCreateInfo ii = {};
	ii.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	ii.pApplicationInfo = &ai;
	ii.enabledLayerCount = (uint32_t)validationLayers.size();
	ii.ppEnabledLayerNames = validationLayers.c_ptr();
	ii.enabledExtensionCount = sizeof_array(c_extensions);
	ii.ppEnabledExtensionNames = c_extensions;

	if ((result = vkCreateInstance(&ii, 0, &m_instance)) != VK_SUCCESS)
	{
		log::error << L"Failed to create Vulkan instance (" << getHumanResult(result) << L")." << Endl;
		return false;
	}

	// Load Vulkan extensions.
	if (!initializeVulkanExtensions(m_instance))
	{
		log::error << L"Failed to create Vulkan; failed to load extensions." << Endl;
		return false;
	}

#if !defined(__ANDROID__)
	if (desc.validation)
	{
		// Setup debug port callback.
		VkDebugUtilsMessengerCreateInfoEXT dumci = {};
		dumci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		dumci.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		dumci.messageType = /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT /*| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT*/;
		dumci.pfnUserCallback = debugCallback;

		if ((result = vkCreateDebugUtilsMessengerEXT(m_instance, &dumci, nullptr, &m_debugMessenger)) != VK_SUCCESS)
		{
			log::error << L"Failed to create Vulkan; failed to set debug report callback." << Endl;
			return false;
		}
	}
#endif

	// Select physical device.
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, 0);
	if (physicalDeviceCount == 0)
	{
		log::error << L"Failed to create Vulkan; no physical devices." << Endl;
		return false;
	}
	log::info << L"Found " << physicalDeviceCount << L" physical device(s)," << Endl;

	AutoArrayPtr< VkPhysicalDevice > physicalDevices(new VkPhysicalDevice[physicalDeviceCount]);
	vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.ptr());

	for (uint32_t i = 0; !m_physicalDevice && i < physicalDeviceCount; ++i)
	{
		VkPhysicalDeviceProperties pdp = {};
		vkGetPhysicalDeviceProperties(physicalDevices[i], &pdp);
		if (isDeviceSuitable(physicalDevices[i], VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
			m_physicalDevice = physicalDevices[i];
	}
	for (uint32_t i = 0; !m_physicalDevice && i < physicalDeviceCount; ++i)
	{
		VkPhysicalDeviceProperties pdp = {};
		vkGetPhysicalDeviceProperties(physicalDevices[i], &pdp);
		if (isDeviceSuitable(physicalDevices[i], VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU))
			m_physicalDevice = physicalDevices[i];
	}		
	if (!m_physicalDevice)
	{
		log::warning << L"Unable to find a suitable device; attempting to use first reported." << Endl;
		m_physicalDevice = physicalDevices[0];
	}

	// Get physical device graphics queue.
	uint32_t graphicsQueueIndex = ~0;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, 0);

	AutoArrayPtr< VkQueueFamilyProperties > queueFamilyProperties(new VkQueueFamilyProperties[queueFamilyCount]);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilyProperties.ptr());

	// Select graphics queue.
	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			graphicsQueueIndex = i;
			break;
		}
	}

	if (graphicsQueueIndex == ~0)
	{
		log::error << L"Failed to create Vulkan; no suitable graphics queue found." << Endl;
		return false;
	}

	// Create logical device.
	const float queuePriorities[] = { 1.0f, 1.0f };

    VkDeviceQueueCreateInfo dqci = {};
	dqci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	dqci.queueFamilyIndex = graphicsQueueIndex;
	dqci.queueCount = 1;
	dqci.pQueuePriorities = queuePriorities;

    VkDeviceCreateInfo dci = {};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dci.pNext = nullptr;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &dqci;
	dci.enabledLayerCount = (uint32_t)validationLayers.size();
	dci.ppEnabledLayerNames = validationLayers.c_ptr();
    dci.enabledExtensionCount = sizeof_array(c_deviceExtensions);
    dci.ppEnabledExtensionNames = c_deviceExtensions;

#if !defined(__ANDROID__) && !defined(__RPI__)
    VkPhysicalDeviceFeatures features = {};
	features.sampleRateShading = VK_TRUE;
	features.multiDrawIndirect = VK_TRUE;
    features.shaderClipDistance = VK_TRUE;
	features.samplerAnisotropy = VK_TRUE;
    dci.pEnabledFeatures = &features;

	VkPhysicalDevice8BitStorageFeaturesKHR s8 = {};
	s8.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR;
	s8.pNext = nullptr;
	s8.storageBuffer8BitAccess = VK_FALSE;
	s8.uniformAndStorageBuffer8BitAccess = VK_TRUE;
	s8.storagePushConstant8 = VK_FALSE;
	dci.pNext = &s8;

	VkPhysicalDeviceFloat16Int8FeaturesKHR f16 = {};
	f16.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
	f16.pNext = nullptr;
	f16.shaderFloat16 = VK_FALSE;
	f16.shaderInt8 = VK_TRUE;
	s8.pNext = &f16;

	// Bindless textures.
	VkPhysicalDeviceDescriptorIndexingFeatures di = {};
	di.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	di.runtimeDescriptorArray = VK_TRUE;
	di.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	di.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
	di.descriptorBindingPartiallyBound = VK_TRUE;
	di.descriptorBindingVariableDescriptorCount = VK_TRUE;
	f16.pNext = &di;

	VkPhysicalDeviceBufferDeviceAddressFeatures daf{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
	daf.bufferDeviceAddress = VK_TRUE;
	daf.bufferDeviceAddressCaptureReplay = VK_FALSE;
	daf.bufferDeviceAddressMultiDevice = VK_FALSE;
	di.pNext = &daf;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR asf{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	asf.accelerationStructure = VK_TRUE;
	asf.accelerationStructureCaptureReplay = VK_FALSE;
	asf.accelerationStructureIndirectBuild = VK_FALSE;
	asf.accelerationStructureHostCommands = VK_FALSE;
	asf.descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE;
	daf.pNext = &asf;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtp{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	rtp.rayTracingPipeline = VK_TRUE;
	rtp.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
	rtp.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
	rtp.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
	rtp.rayTraversalPrimitiveCulling = VK_FALSE;
	asf.pNext = &rtp;

	VkPhysicalDeviceVulkan11Features v11 = {};
	v11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	v11.storageBuffer16BitAccess = VK_TRUE;
	v11.uniformAndStorageBuffer16BitAccess = VK_TRUE;
	v11.storagePushConstant16 = VK_FALSE;
	v11.storageInputOutput16 = VK_FALSE;
	v11.multiview = VK_FALSE;
	v11.multiviewGeometryShader = VK_FALSE;
	v11.multiviewTessellationShader = VK_FALSE;
	v11.variablePointersStorageBuffer = VK_FALSE;
	v11.variablePointers = VK_FALSE;
	v11.protectedMemory = VK_FALSE;
	v11.samplerYcbcrConversion = VK_FALSE;
	v11.shaderDrawParameters = VK_TRUE;
	rtp.pNext = &v11;
#endif

    if ((result = vkCreateDevice(m_physicalDevice, &dci, 0, &m_logicalDevice)) != VK_SUCCESS)
	{
		log::error << L"Failed to create Vulkan; unable to create logical device (" << getHumanResult(result) << L")." << Endl;
		return false;
	}

	// Create memory allocator.
	VmaVulkanFunctions vf = {};
	vf.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vf.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vf.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vf.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vf.vkAllocateMemory = vkAllocateMemory;
	vf.vkFreeMemory = vkFreeMemory;
	vf.vkMapMemory = vkMapMemory;
	vf.vkUnmapMemory = vkUnmapMemory;
	vf.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vf.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vf.vkBindBufferMemory = vkBindBufferMemory;
	vf.vkBindImageMemory = vkBindImageMemory;
	vf.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vf.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vf.vkCreateBuffer = vkCreateBuffer;
	vf.vkDestroyBuffer = vkDestroyBuffer;
	vf.vkCreateImage = vkCreateImage;
	vf.vkDestroyImage = vkDestroyImage;
	vf.vkCmdCopyBuffer = vkCmdCopyBuffer;
	vf.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
	vf.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;

	VmaAllocatorCreateInfo aci = {};
#if !defined(__RPI__) && !defined(__ANDROID__) && !defined(__IOS__)
	// \note Disabled for now, not clear if we need it and until we do let's leave it disabled.
	//if (vf.vkGetBufferMemoryRequirements2KHR != nullptr && vf.vkGetImageMemoryRequirements2KHR != nullptr)
	//	aci.vulkanApiVersion = VK_API_VERSION_1_2;

	aci.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
#endif
	aci.physicalDevice = m_physicalDevice;
	aci.device = m_logicalDevice;
	aci.pVulkanFunctions = &vf;
	aci.instance = m_instance;
#if defined(__IOS__)
	aci.preferredLargeHeapBlockSize = 32 * 1024 * 1024;
#endif
	if (vmaCreateAllocator(&aci, &m_allocator) != VK_SUCCESS)
	{
		log::error << L"Failed to create Vulkan; failed to create allocator." << Endl;
		return false;
	}

	m_context = new Context(
		m_physicalDevice,
		m_logicalDevice,
		m_allocator,
		graphicsQueueIndex
	);
	if (!m_context->create())
	{
		log::error << L"Failed to create Vulkan; failed to create context." << Endl;
		return false;
	}

	m_shaderModuleCache = new ShaderModuleCache(m_logicalDevice);
	m_pipelineLayoutCache = new PipelineLayoutCache(m_context);
	m_maxAnisotropy = desc.maxAnisotropy;
	m_mipBias = desc.mipBias;

	log::info << L"Vulkan render system created successfully." << Endl;
	return true;
}

void RenderSystemVk::destroy()
{
	m_shaderModuleCache = nullptr;
	m_pipelineLayoutCache = nullptr;
	m_context = nullptr;
	finalizeVulkanApi();
}

bool RenderSystemVk::reset(const RenderSystemDesc& desc)
{
	// \todo Update mipmap bias and maximum anisotropy.
	return true;
}

void RenderSystemVk::getInformation(RenderSystemInformation& outInfo) const
{
	outInfo.dedicatedMemoryTotal = 0;
	outInfo.sharedMemoryTotal = 0;
	outInfo.dedicatedMemoryAvailable = 0;
	outInfo.sharedMemoryAvailable = 0;
}

uint32_t RenderSystemVk::getDisplayCount() const
{
#if defined(_WIN32)
	uint32_t count = 0;

	DISPLAY_DEVICE dd = {};
	dd.cb = sizeof(dd);
	while (EnumDisplayDevices(nullptr, count, &dd, 0))
		++count;

	return count;
#else
	return 1;
#endif
}

uint32_t RenderSystemVk::getDisplayModeCount(uint32_t display) const
{
#if defined(_WIN32)
	uint32_t count = 0;

	DISPLAY_DEVICE dd = {};
	dd.cb = sizeof(dd);
	EnumDisplayDevices(nullptr, display, &dd, 0);

	DEVMODE dmgl = {};
	dmgl.dmSize = sizeof(dmgl);
	while (EnumDisplaySettings(dd.DeviceName, count, &dmgl))
		++count;

	return count;
#else
	return 0;
#endif
}

DisplayMode RenderSystemVk::getDisplayMode(uint32_t display, uint32_t index) const
{
#if defined(_WIN32)
	DISPLAY_DEVICE dd = {};
	dd.cb = sizeof(dd);
	EnumDisplayDevices(nullptr, display, &dd, 0);

	DEVMODE dmgl = {};
	dmgl.dmSize = sizeof(dmgl);
	EnumDisplaySettings(dd.DeviceName, index, &dmgl);

	DisplayMode dm;
	dm.width = dmgl.dmPelsWidth;
	dm.height = dmgl.dmPelsHeight;
	dm.refreshRate = (uint16_t)dmgl.dmDisplayFrequency;
	dm.colorBits = (uint16_t)dmgl.dmBitsPerPel;
	return dm;
#else
	return DisplayMode();
#endif
}

DisplayMode RenderSystemVk::getCurrentDisplayMode(uint32_t display) const
{
	DisplayMode dm;
#if defined(_WIN32)
	DISPLAY_DEVICE dd = {};
	dd.cb = sizeof(dd);
	EnumDisplayDevices(nullptr, display, &dd, 0);

	DEVMODE dmgl = {};
	dmgl.dmSize = sizeof(dmgl);
	EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dmgl);

	dm.width = dmgl.dmPelsWidth;
	dm.height = dmgl.dmPelsHeight;
	dm.dpi = 96;
	dm.refreshRate = dmgl.dmDisplayFrequency;
	dm.colorBits = (uint16_t)dmgl.dmBitsPerPel;
#elif defined(__LINUX__) || defined(__RPI__)
	const int screen = DefaultScreen(m_display);
	const double dpih = ((double)DisplayWidth(m_display, screen)) / (((double)DisplayWidthMM(m_display, screen)) / 25.4);
	const double dpiv = ((double)DisplayHeight(m_display, screen)) / (((double)DisplayHeightMM(m_display, screen)) / 25.4);

	dm.width = DisplayWidth(m_display, screen);
	dm.height = DisplayHeight(m_display, screen);
	dm.dpi = (uint16_t)(std::max(dpih, dpiv));
	dm.refreshRate = 59.98f;
	dm.colorBits = 32;
#elif defined(__ANDROID__)
	dm.width = m_screenWidth;
	dm.height = m_screenHeight;
	dm.dpi = 96;
	dm.refreshRate = 60.0f;
	dm.colorBits = 32;
#elif defined(__IOS__)
	dm.width = getScreenWidth();
	dm.height = getScreenHeight();
	dm.dpi = 96;
	dm.refreshRate = 60.0f;
	dm.colorBits = 32;
#endif
	return dm;
}

float RenderSystemVk::getDisplayAspectRatio(uint32_t display) const
{
	return 0.0f;
}

Ref< IRenderView > RenderSystemVk::createRenderView(const RenderViewDefaultDesc& desc)
{
	Ref< RenderViewVk > renderView = new RenderViewVk(
		m_context,
		m_instance
	);
	if (renderView->create(desc))
		return renderView;
	else
		return nullptr;
}

Ref< IRenderView > RenderSystemVk::createRenderView(const RenderViewEmbeddedDesc& desc)
{
	Ref< RenderViewVk > renderView = new RenderViewVk(
		m_context,
		m_instance
	);
	if (renderView->create(desc))
		return renderView;
	else
		return nullptr;
}

Ref< Buffer > RenderSystemVk::createBuffer(uint32_t usage, uint32_t bufferSize, bool dynamic)
{
	uint32_t usageBits = 0;
	if ((usage & BuVertex) != 0)
		usageBits |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	if ((usage & BuIndex) != 0)
		usageBits |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	if ((usage & BuStructured) != 0)
		usageBits |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if ((usage & BuIndirect) != 0)
		usageBits |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if (usageBits == 0)
		return nullptr;

	if (dynamic)
	{
		Ref< BufferDynamicVk > buffer = new BufferDynamicVk(m_context, bufferSize, m_statistics.buffers);
		if (buffer->create(usageBits, 3 * 4))
			return buffer;
	}
	else
	{
		Ref< BufferStaticVk > buffer = new BufferStaticVk(m_context, bufferSize, m_statistics.buffers);
		if (buffer->create(usageBits))
			return buffer;
	}

	return nullptr;
}

Ref< const IVertexLayout > RenderSystemVk::createVertexLayout(const AlignedVector< VertexElement >& vertexElements)
{
	VkVertexInputBindingDescription vibd = {};
	vibd.binding = 0;
	vibd.stride = getVertexSize(vertexElements);
	vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	AlignedVector< VkVertexInputAttributeDescription > vads;
	int32_t positionElementIndex = -1;

	for (auto vertexElement : vertexElements)
	{
		auto& vad = vads.push_back();

		vad.location = VertexAttributes::getLocation(vertexElement.getDataUsage(), vertexElement.getIndex());
		vad.binding = 0;
		vad.format = c_vkVertexElementFormats[vertexElement.getDataType()];
		vad.offset = vertexElement.getOffset();

		if (vertexElement.getDataUsage() == DataUsage::Position && vertexElement.getIndex() == 0)
			positionElementIndex = int32_t(vads.size() - 1);
	}

	Murmur3 cs;
	cs.begin();
	cs.feedBuffer(vertexElements.c_ptr(), vertexElements.size() * sizeof(VertexElement));
	cs.end();

	return new VertexLayoutVk(vibd, vads, positionElementIndex, cs.get());
}

Ref< ITexture > RenderSystemVk::createSimpleTexture(const SimpleTextureCreateDesc& desc, const wchar_t* const tag)
{
	Ref< TextureVk > texture = new TextureVk(m_context, m_statistics.simpleTextures);
	if (texture->create(desc, tag))
		return texture;
	else
		return nullptr;
}

Ref< ITexture > RenderSystemVk::createCubeTexture(const CubeTextureCreateDesc& desc, const wchar_t* const tag)
{
	Ref< TextureVk > texture = new TextureVk(m_context, m_statistics.cubeTextures);
	if (texture->create(desc, tag))
		return texture;
	else
		return nullptr;
}

Ref< ITexture > RenderSystemVk::createVolumeTexture(const VolumeTextureCreateDesc& desc, const wchar_t* const tag)
{
	Ref< TextureVk > texture = new TextureVk(m_context, m_statistics.volumeTextures);
	if (texture->create(desc, tag))
		return texture;
	else
		return nullptr;
}

Ref< IRenderTargetSet > RenderSystemVk::createRenderTargetSet(const RenderTargetSetCreateDesc& desc, IRenderTargetSet* sharedDepthStencil, const wchar_t* const tag)
{
	Ref< RenderTargetSetVk > renderTargetSet = new RenderTargetSetVk(m_context, m_statistics.renderTargetSets);
	if (renderTargetSet->create(desc, sharedDepthStencil, tag))
		return renderTargetSet;
	else
		return nullptr;
}

Ref< IAccelerationStructure > RenderSystemVk::createTopLevelAccelerationStructure(uint32_t numInstances)
{
	VkResult result;

	Ref< ApiBuffer > instanceBuffer = new ApiBuffer(m_context);
	instanceBuffer->create(
		numInstances * sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		false,
		true
	);

	void* ptr = instanceBuffer->lock();
	if (!ptr)
		return nullptr;

	{
		VkAccelerationStructureInstanceKHR instance = {};
		//instance.transform.matrix =
		//{
		//	{1.0, 0.0, 0.0, 0.0},
		//	{0.0, 1.0, 0.0, 0.0},
		//	{0.0, 0.0, 1.0, 0.0}
		//};
		instance.instanceCustomIndex = 0;
		instance.mask = 0xff;
		instance.instanceShaderBindingTableRecordOffset = 0;
		instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		//instance.accelerationStructureReference = bottomLevelAccelerationStructureDeviceAddress;
	}

	instanceBuffer->unlock();



	VkAccelerationStructureGeometryDataKHR topLevelAccelerationStructureGeometryData =
	{
		.instances =
		{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
			.pNext = NULL,
			.arrayOfPointers = VK_FALSE,
			.data =
			{
				.deviceAddress = instanceBuffer->getDeviceAddress()
			}
		}
	};

	VkAccelerationStructureGeometryKHR topLevelAccelerationStructureGeometry =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.pNext = NULL,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.geometry = topLevelAccelerationStructureGeometryData,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR
	};

	VkAccelerationStructureBuildGeometryInfoKHR topLevelAccelerationStructureBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.pNext = NULL,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = 0,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.srcAccelerationStructure = VK_NULL_HANDLE,
		.dstAccelerationStructure = VK_NULL_HANDLE,
		.geometryCount = 1,
		.pGeometries = &topLevelAccelerationStructureGeometry,
		.ppGeometries = NULL,
		.scratchData =
		{
			.deviceAddress = 0
		}
	};

	VkAccelerationStructureBuildSizesInfoKHR topLevelAccelerationStructureBuildSizesInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
		.pNext = NULL,
		.accelerationStructureSize = 0,
		.updateScratchSize = 0,
		.buildScratchSize = 0
	};


	AlignedVector< uint32_t > topLevelMaxPrimitiveCountList = { 1 };

	vkGetAccelerationStructureBuildSizesKHR(
		m_logicalDevice,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&topLevelAccelerationStructureBuildGeometryInfo,
		topLevelMaxPrimitiveCountList.ptr(),
		&topLevelAccelerationStructureBuildSizesInfo
	);


	Ref< ApiBuffer > buffer = new ApiBuffer(m_context);
	buffer->create(topLevelAccelerationStructureBuildSizesInfo.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false, true);

	VkAccelerationStructureCreateInfoKHR topLevelAccelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.pNext = NULL,
		.createFlags = 0,
		.buffer = *buffer,
		.offset = 0,
		.size = topLevelAccelerationStructureBuildSizesInfo
		.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.deviceAddress = 0
	};

	VkAccelerationStructureKHR topLevelAccelerationStructureHandle = VK_NULL_HANDLE;

	result = vkCreateAccelerationStructureKHR(
		m_logicalDevice,
		&topLevelAccelerationStructureCreateInfo,
		NULL,
		&topLevelAccelerationStructureHandle
	);
	if (result != VK_SUCCESS)
		return nullptr;

	// Allocate scratch buffer.
	ApiBuffer scratchBuffer(m_context);
	if (!scratchBuffer.create(topLevelAccelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false, true))
	{
		safeDestroy(buffer);
		return nullptr;
	}


	topLevelAccelerationStructureBuildGeometryInfo.dstAccelerationStructure = topLevelAccelerationStructureHandle;
	topLevelAccelerationStructureBuildGeometryInfo.scratchData =
	{
		.deviceAddress = scratchBuffer.getDeviceAddress()
	};

	VkAccelerationStructureBuildRangeInfoKHR topLevelAccelerationStructureBuildRangeInfo =
	{
		.primitiveCount = 1,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0
	};

	const VkAccelerationStructureBuildRangeInfoKHR* topLevelAccelerationStructureBuildRangeInfos = &topLevelAccelerationStructureBuildRangeInfo;

	auto commandBuffer = m_context->getGraphicsQueue()->acquireCommandBuffer(T_FILE_LINE_W);
	vkCmdBuildAccelerationStructuresKHR(
		*commandBuffer,
		1,
		&topLevelAccelerationStructureBuildGeometryInfo,
		&topLevelAccelerationStructureBuildRangeInfos
	);
	commandBuffer->submitAndWait();

	// Destroy scratch buffer.
	scratchBuffer.destroy();

	return new AccelerationStructureVk(m_context, buffer, topLevelAccelerationStructureHandle);
}

Ref< IAccelerationStructure > RenderSystemVk::createAccelerationStructure(const Buffer* vertexBuffer, const IVertexLayout* vertexLayout, const Buffer* indexBuffer, IndexType indexType, const AlignedVector< Primitives >& primitives)
{
	VkResult result;

	const VertexLayoutVk* vertexLayoutVk = mandatory_non_null_type_cast< const VertexLayoutVk* >(vertexLayout);
	const int32_t pidx = vertexLayoutVk->getPositionElementIndex();
	if (pidx < 0)
		return nullptr;

	const VkVertexInputAttributeDescription& piad = vertexLayoutVk->getVkVertexInputAttributeDescriptions()[pidx];

	const BufferViewVk* vb = mandatory_non_null_type_cast< const BufferViewVk* >(vertexBuffer->getBufferView());
	const BufferViewVk* ib = mandatory_non_null_type_cast< const BufferViewVk* >(indexBuffer->getBufferView());

	// Get device address of vertex buffer.
	VkBufferDeviceAddressInfo vdai{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	vdai.buffer = vb->getVkBuffer();
	const VkDeviceAddress vaddr = vkGetBufferDeviceAddressKHR(m_logicalDevice, &vdai) + vb->getVkBufferOffset();

	// Get device address of index buffer.
	VkBufferDeviceAddressInfo idai{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	idai.buffer = ib->getVkBuffer();
	const VkDeviceAddress iaddr = vkGetBufferDeviceAddressKHR(m_logicalDevice, &idai) + ib->getVkBufferOffset();

	// Geometry triangles.
	VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
	triangles.vertexFormat = piad.format;
	triangles.vertexData.deviceAddress = vaddr;
	triangles.vertexStride = vertexLayoutVk->getVkVertexInputBindingDescription().stride;
	triangles.indexType = (indexType == IndexType::UInt32) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;
	triangles.indexData.deviceAddress = iaddr;
	triangles.maxVertex = vb->getVkBufferSize() / vertexLayoutVk->getVkVertexInputBindingDescription().stride;

	// Geometry.
	VkAccelerationStructureGeometryKHR geom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
	geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	geom.geometry.triangles = triangles;

	// Build geometry.
	VkAccelerationStructureBuildGeometryInfoKHR asbgi{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	asbgi.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	asbgi.flags = 0;
	asbgi.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	asbgi.srcAccelerationStructure = VK_NULL_HANDLE;
	asbgi.dstAccelerationStructure = VK_NULL_HANDLE;
	asbgi.geometryCount = 1;
	asbgi.pGeometries = &geom;
	asbgi.ppGeometries = nullptr;
	asbgi.scratchData = VkDeviceOrHostAddressKHR();
	asbgi.scratchData.deviceAddress = 0;

	// Query size of buffers required for AS.
	VkAccelerationStructureBuildSizesInfoKHR asbsi{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
	asbsi.accelerationStructureSize = 0;
	asbsi.updateScratchSize = 0;
	asbsi.buildScratchSize = 0;

	const uint32_t primitiveCount = indexBuffer->getBufferSize() / ((indexType == IndexType::UInt32) ? 4 : 2);
	AlignedVector< uint32_t > bottomLevelMaxPrimitiveCountList = { primitiveCount };

	vkGetAccelerationStructureBuildSizesKHR(
		m_logicalDevice,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&asbgi,
		bottomLevelMaxPrimitiveCountList.ptr(),
		&asbsi
	);

	// Allocate buffer to hold AS data.
	Ref< ApiBuffer > buffer = new ApiBuffer(m_context);
	buffer->create(asbsi.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false, true);

	// Create AS.
	VkAccelerationStructureCreateInfoKHR asci{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
	asci.createFlags = 0;
	asci.buffer = *buffer;
	asci.offset = 0;
	asci.size = asbsi.accelerationStructureSize;
	asci.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	asci.deviceAddress = 0;

	VkAccelerationStructureKHR as = VK_NULL_HANDLE;
	result = vkCreateAccelerationStructureKHR(
		m_logicalDevice,
		&asci,
		NULL,
		&as
	);
	if (result != VK_SUCCESS)
		return nullptr;

	// Allocate scratch buffer.
	ApiBuffer scratchBuffer(m_context);
	if (!scratchBuffer.create(asbsi.buildScratchSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, false, true))
	{
		safeDestroy(buffer);
		return nullptr;
	}

	// Build AS.
	asbgi.dstAccelerationStructure = as;
	asbgi.scratchData.deviceAddress = scratchBuffer.getDeviceAddress();

	AlignedVector< VkAccelerationStructureBuildRangeInfoKHR > buildRanges;
	for (const auto& primitive : primitives)
	{
		if (primitive.type != PrimitiveType::Triangles)
			continue;

		VkAccelerationStructureBuildRangeInfoKHR& offset = buildRanges.push_back();
		offset.firstVertex = primitive.offset;
		offset.primitiveCount = primitive.count;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;
	}

	AlignedVector< VkAccelerationStructureBuildRangeInfoKHR* > buildRangePtrs;
	for (auto& buildRange : buildRanges)
		buildRangePtrs.push_back(&buildRange);

	auto commandBuffer = m_context->getGraphicsQueue()->acquireCommandBuffer(T_FILE_LINE_W);
	vkCmdBuildAccelerationStructuresKHR(
		*commandBuffer,
		1,
		&asbgi,
		buildRangePtrs.ptr()
	);
	commandBuffer->submitAndWait();

	// Destroy scratch buffer.
	scratchBuffer.destroy();

	return new AccelerationStructureVk(m_context, buffer, as);
}

Ref< IProgram > RenderSystemVk::createProgram(const ProgramResource* programResource, const wchar_t* const tag)
{
	Ref< const ProgramResourceVk > resource = dynamic_type_cast< const ProgramResourceVk* >(programResource);
	if (!resource)
		return nullptr;

	Ref< ProgramVk > program = new ProgramVk(m_context, m_statistics.programs);
	if (program->create(m_shaderModuleCache, m_pipelineLayoutCache, resource, m_maxAnisotropy, m_mipBias, tag))
		return program;
	else
		return nullptr;
}

void RenderSystemVk::purge()
{
	if (m_context)
		m_context->savePipelineCache();
}

void RenderSystemVk::getStatistics(RenderSystemStatistics& outStatistics) const
{
	outStatistics = m_statistics;
}

void* RenderSystemVk::getInternalHandle() const
{
	return (*((void **)(m_instance)));
}

}
