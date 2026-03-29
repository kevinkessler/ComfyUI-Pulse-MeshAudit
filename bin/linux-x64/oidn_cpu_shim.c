/*
 * OIDN HIP fallback shim for AMD GPUs.
 *
 * The agnirt binary calls oidnNewDeviceByUUID() to create an OIDN denoiser
 * matched to the Vulkan GPU's UUID. The system OIDN's UUID matching may not
 * find the HIP device. This shim intercepts the call and creates a HIP
 * device directly (type 4), which accesses the same AMD GPU and supports
 * Vulkan external memory sharing.
 *
 * Falls back to CPU device (type 1) if HIP fails.
 *
 * Usage: LD_PRELOAD=oidn_hip_shim.so ./agnirt ...
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>

typedef void* OIDNDevice;
typedef int OIDNDeviceType;
typedef int OIDNError;

#define OIDN_DEVICE_TYPE_CPU 1
#define OIDN_DEVICE_TYPE_HIP 4

OIDNDevice oidnNewDeviceByUUID(const void* uuid) {
    OIDNDevice (*real_new)(OIDNDeviceType) = dlsym(RTLD_DEFAULT, "oidnNewDevice");
    if (!real_new) {
        void* handle = dlopen("libOpenImageDenoise.so.2", RTLD_LAZY | RTLD_NOLOAD);
        if (handle) real_new = dlsym(handle, "oidnNewDevice");
    }
    if (!real_new) {
        fprintf(stderr, "[oidn_shim] Cannot find oidnNewDevice: %s\n", dlerror());
        return NULL;
    }

    /* Try HIP first (GPU memory sharing works) */
    fprintf(stderr, "[oidn_shim] Redirecting oidnNewDeviceByUUID -> HIP device\n");
    OIDNDevice dev = real_new(OIDN_DEVICE_TYPE_HIP);
    if (dev) return dev;

    /* Fall back to CPU */
    fprintf(stderr, "[oidn_shim] HIP failed, falling back to CPU device\n");
    return real_new(OIDN_DEVICE_TYPE_CPU);
}
