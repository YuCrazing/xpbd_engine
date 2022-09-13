// #include <chrono>
// #include <iostream>
// #include <signal.h>
// #include <inttypes.h>
// #include <unistd.h>
// #include <memory>
// #include <vector>

// #include <taichi/ui/gui/gui.h>
// #include <taichi/ui/backends/vulkan/renderer.h>
// #include "taichi_core_impl.h"
// #include "c_api_test_utils.h"
// #include "taichi/taichi_core.h"
// #include "taichi/taichi_vulkan.h"
// // #include "taichi/ui/backends/vulkan/scene.h"
// // #include "taichi/ui/backends/vulkan/window.h"
// // #include "taichi/ui/backends/vulkan/canvas.h"

void ti_memcpy(taichi::lang::DeviceAllocation& dst_dev_alloc, taichi::lang::DevicePtr& src_dev_alloc_ptr, size_t size, taichi::ui::vulkan::AppContext* app_context) {
    taichi::lang::Device::MemcpyCapability memcpy_cap = taichi::lang::Device::check_memcpy_capability(dst_dev_alloc.get_ptr(), src_dev_alloc_ptr, size);
    if (memcpy_cap == taichi::lang::Device::MemcpyCapability::Direct) {
        taichi::lang::Device::memcpy_direct(dst_dev_alloc.get_ptr(), src_dev_alloc_ptr, size);
    } else if (memcpy_cap == taichi::lang::Device::MemcpyCapability::RequiresStagingBuffer) {
        void *dst_mapped = app_context->device().map(dst_dev_alloc);
        taichi::lang::DeviceAllocation attr_buffer(src_dev_alloc_ptr);
        void *src_mapped = src_dev_alloc_ptr.device->map(attr_buffer);
        memcpy(dst_mapped, src_mapped, size);
        app_context->device().unmap(dst_dev_alloc);
        src_dev_alloc_ptr.device->unmap(attr_buffer);
    } else {
        TI_NOT_IMPLEMENTED;
    }
}

static taichi::lang::DeviceAllocation get_devalloc(TiRuntime runtime, TiMemory memory) {
      Runtime* real_runtime = (Runtime *)runtime;
      return devmem2devalloc(*real_runtime, memory);
}

static taichi::lang::DeviceAllocation get_ndarray_with_imported_memory(TiRuntime runtime,
                                                                       TiDataType dtype,
                                                                       std::vector<int> arr_shape,
                                                                       std::vector<int> element_shape,
                                                                       taichi::lang::vulkan::VulkanDevice * vk_device,
                                                                       capi::utils::TiNdarrayAndMem& ndarray) {
      assert(dtype == TiDataType::TI_DATA_TYPE_F32);
      size_t alloc_size = 4;

      TiNdShape shape;
      shape.dim_count = static_cast<uint32_t>(arr_shape.size());
      for(size_t i = 0; i < arr_shape.size(); i++) {
        alloc_size *= arr_shape[i];
        shape.dims[i] = arr_shape[i];
      }
      
      TiNdShape e_shape;
      e_shape.dim_count = static_cast<uint32_t>(element_shape.size());
      for(size_t i = 0; i < element_shape.size(); i++) {
        alloc_size *= element_shape[i];
        e_shape.dims[i] = element_shape[i];
      }
      
      taichi::lang::Device::AllocParams alloc_params;
      alloc_params.host_read = false;
      alloc_params.host_write = false;
      alloc_params.size = alloc_size;
      alloc_params.usage = taichi::lang::AllocUsage::Storage;
      
      auto res = vk_device->allocate_memory(alloc_params);

      TiVulkanMemoryInteropInfo interop_info;
      interop_info.buffer = vk_device->get_vkbuffer(res).get()->buffer;
      interop_info.size = vk_device->get_vkbuffer(res).get()->size;
      interop_info.usage = vk_device->get_vkbuffer(res).get()->usage;
    
      ndarray.runtime_ = runtime;
      ndarray.memory_ = ti_import_vulkan_memory(ndarray.runtime_, &interop_info);

      TiNdArray arg_array = {.memory = ndarray.memory_,
                             .shape = std::move(shape),
                             .elem_shape = std::move(e_shape),
                             .elem_type = dtype};

      TiArgumentValue arg_value = {.ndarray = std::move(arg_array)};
        
      ndarray.arg_ = {.type = TiArgumentType::TI_ARGUMENT_TYPE_NDARRAY,
                             .value = std::move(arg_value)};
      
      return res;
}



static taichi::Arch get_taichi_arch(TiArch arch) {
    switch(arch) {
        case TiArch::TI_ARCH_VULKAN: {
            return taichi::Arch::vulkan;
        }
        case TiArch::TI_ARCH_X64: {
            return taichi::Arch::x64;
        }
        case TiArch::TI_ARCH_CUDA: {
            return taichi::Arch::cuda;
        }
        default: {
            return taichi::Arch::x64;
        }
    }
}

static taichi::ui::FieldSource get_field_source(TiArch arch) {
    switch(arch) {
        case TiArch::TI_ARCH_VULKAN: {
            return taichi::ui::FieldSource::TaichiVulkan;
        }
        case TiArch::TI_ARCH_X64: {
            return taichi::ui::FieldSource::TaichiX64;
        }
        case TiArch::TI_ARCH_CUDA: {
            return taichi::ui::FieldSource::TaichiCuda;
        }
        default: {
            return taichi::ui::FieldSource::TaichiX64;
        }
    }
}

glm::vec3 euler_to_vec(float yaw, float pitch) {
    auto v = glm::vec3(0.0, 0.0, 0.0);
    v[0] = -sin(yaw) * cos(pitch);
    v[1] = sin(pitch);
    v[2] = -cos(yaw) * cos(pitch);
    return v;
}


void vec_to_euler(glm::vec3 v, float& yaw, float& pitch) {
    v = glm::normalize(v);
    pitch = asin(v[1]);

    auto sin_yaw = -v[0] / cos(pitch);
    auto cos_yaw = -v[2] / cos(pitch);

    auto eps = 1e-6;

    if(abs(sin_yaw) < eps)
        yaw = 0;
    else {
        yaw = acos(cos_yaw);
        if(sin_yaw < 0)
            yaw = -yaw;
    }
}


void handle_user_inputs(taichi::ui::Camera* camera, GLFWwindow* window, int win_width, int win_height) {
    
    static float movement_speed = 0.1;
    static bool first_time_detect_mouse_input = true;
    static double last_mouse_x, last_mouse_y, curr_mouse_x, curr_mouse_y;

    auto front = glm::normalize(camera->lookat - camera->position);
    auto position_change = glm::vec3(0.0, 0.0, 0.0);
    auto left = glm::cross(camera->up, front);
    auto up = camera->up;
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        position_change += front * movement_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        position_change += -front * movement_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        position_change += left * movement_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        position_change += -left * movement_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        position_change += up * movement_speed;
    }
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        position_change += -up * movement_speed;
    }
    camera->position = camera->position + position_change;
    camera->lookat = camera->lookat + position_change;


    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {

        glfwGetCursorPos(window, &curr_mouse_x, &curr_mouse_y);
        curr_mouse_x /= win_width;
        curr_mouse_y /= win_height;
        curr_mouse_y = 1.0 - curr_mouse_y;

        if(first_time_detect_mouse_input) {
            first_time_detect_mouse_input = false;
            last_mouse_x = curr_mouse_x;
            last_mouse_y = curr_mouse_y;
        }

        auto dx = curr_mouse_x - last_mouse_x;
        auto dy = curr_mouse_y - last_mouse_y;

        float yaw, pitch;
        vec_to_euler(front, yaw, pitch);
        float yaw_speed = 2;
        float pitch_speed = 2;

        yaw -= dx * yaw_speed;
        pitch += dy * pitch_speed;

        float pitch_limit = glm::pi<float>() / 2 * 0.99;
        if(pitch > pitch_limit)
            pitch = pitch_limit;
        else if(pitch < -pitch_limit)
            pitch = -pitch_limit;

        front = euler_to_vec(yaw, pitch);
        camera->lookat = glm::vec3(camera->position + front);
        camera->up = glm::vec3(0, 1, 0);

        last_mouse_x = curr_mouse_x;
        last_mouse_y = curr_mouse_y;
    }else{
        first_time_detect_mouse_input = true;
    }
}