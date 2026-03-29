# ComfyUI Pulse MeshAudit (AMD/ROCm Fork)

A ComfyUI custom node for auditing 3D mesh files by rendering them with a headless Vulkan renderer and displaying an interactive carousel of renders with detailed mesh statistics.

**This fork adds AMD GPU support** for systems running ROCm (e.g., Radeon RX 7000 series, Radeon 8060S / Strix Halo). The upstream version only bundles an NVIDIA CUDA-based OIDN denoiser, which crashes on AMD hardware.

## Changes from Upstream

### Problem

The upstream `agnirt` renderer bundles OpenImageDenoise (OIDN) 2.3.3 with only a CUDA device plugin. On AMD GPUs:

1. The bundled OIDN 2.3.3 cannot create any device (not even CPU) — it appears to be a broken build
2. `agnirt` calls `oidnNewDeviceByUUID()` to create a GPU-matched denoiser, which requires a working device plugin
3. `agnirt` crashes fatally (`std::runtime_error: Failed to create OIDN device`) on startup, preventing all rendering — not just path tracing
4. The `comfy-env.toml` specifies Debian/Ubuntu package names (`libvulkan1`) which don't work on Fedora/RHEL

### Solution

This fork makes three changes:

1. **OIDN HIP shim** (`bin/linux-x64/oidn_cpu_shim.so` + `.c` source): An `LD_PRELOAD` library that intercepts `oidnNewDeviceByUUID()` and redirects it to create an OIDN HIP device (AMD GPU) with CPU fallback. This works with the system-installed OIDN 2.4.0+ which includes HIP support.

2. **Updated `mesh_audit_node.py`**:
   - Adds `/usr/lib64` and `/opt/rocm/lib` to `LD_LIBRARY_PATH` so the system OIDN and ROCm HIP runtime are found
   - Sets `LD_PRELOAD` to load the OIDN shim automatically when present
   - Removes hardcoded developer home directory paths from the audit log search

3. **Bundled OIDN 2.3.3 moved to `bin/linux-x64/oidn_bundled_backup/`** to prevent the broken libraries from being loaded. The system OIDN (installed via package manager) is used instead.

### Files Changed

| File | Change |
|------|--------|
| `mesh_audit_node.py` | AMD library paths, LD_PRELOAD shim, removed hardcoded paths |
| `bin/linux-x64/oidn_cpu_shim.c` | New — OIDN HIP shim source |
| `bin/linux-x64/oidn_cpu_shim.so` | New — compiled shim |
| `bin/linux-x64/oidn_bundled_backup/` | Moved broken bundled OIDN libs here |

## Features

- **16-Render Carousel**: 4 camera angles x 4 shading modes (Path Trace, Wireframe, Geometry Analysis, Sliver Triangles)
- **Interactive Image Viewer**: Click to fullscreen, arrow key navigation
- **Asset Statistics Panel**: Scene metadata, geometry stats, quality metrics
- **Pathtracer with HIP-accelerated denoising** on AMD GPUs

## Platform Support

| Platform | Status | GPU Requirement |
|----------|--------|----------------|
| Linux x64 (Ubuntu/Debian) | Supported | Vulkan-capable GPU |
| Linux x64 (Fedora/RHEL + AMD GPU) | Supported (this fork) | AMD GPU with ROCm + Vulkan |
| Windows x64 | Supported | Vulkan-capable GPU |

## Installation on Fedora / AMD ROCm

### Prerequisites

- ComfyUI installed and working
- AMD GPU with ROCm and Vulkan support
- Python 3.8+

### Steps

1. **Install system dependencies:**

```bash
# Vulkan driver (provides RADV ICD for AMD GPUs)
sudo dnf install -y mesa-vulkan-drivers

# OpenImageDenoise with HIP support (replaces broken bundled OIDN)
sudo dnf install -y oidn-libs
```

2. **Clone into ComfyUI custom_nodes:**

```bash
cd /path/to/ComfyUI/custom_nodes
git clone git@github.com:kevinkessler/ComfyUI-Pulse-MeshAudit.git comfyui-pulse-meshaudit
```

3. **Rebuild the OIDN shim** (optional — precompiled `.so` is included):

```bash
cd comfyui-pulse-meshaudit/bin/linux-x64
gcc -shared -fPIC -o oidn_cpu_shim.so oidn_cpu_shim.c -ldl
```

4. **Restart ComfyUI** and search for the **PulseMeshAudit** node under **Pulse/MeshAudit**.

### Verification

You can test the renderer directly:

```bash
cd /path/to/ComfyUI/custom_nodes/comfyui-pulse-meshaudit/bin/linux-x64
export LD_LIBRARY_PATH="$(pwd):/usr/lib64:/opt/rocm/lib"
LD_PRELOAD=./oidn_cpu_shim.so ./agnirt vulkan ../../assets/cannon_generated.glb \
    -headless -shading-mode all \
    --camera perspective front right left \
    -env ../../assets/sunny_rose_garden_4k.exr \
    -o /tmp/test_render
```

Expected: 16 output images and an `audit_log_*.json` file.

## Usage

1. Add **PulseMeshAudit** node to canvas
2. Set `file_path` to a mesh file (`.glb`, `.obj`, `.gltf`)
3. Execute the node
4. View the 16-render carousel and mesh statistics

## Upstream

Forked from [krishnancr/ComfyUI-Pulse-MeshAudit](https://github.com/krishnancr/ComfyUI-Pulse-MeshAudit).

## Citing this Work

If you use this node in your YouTube videos or commercial workflows, please include a link to the upstream repo and credit it as: "Pulse - MeshAudit by Krishnan Ramachandran"
