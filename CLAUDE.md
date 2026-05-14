# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bat
compilar.bat
```

Or directly:
```
g++ src/main.cpp src/glad.c -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -std=c++17 -O2 -o programa.exe
```

The executable must be launched from the project root — shader paths (`shaders/*.vert/frag`) and texture paths (`texturas/*.jpg`) are relative to the working directory.

There is no test suite, CMake, or package manager. All vendored libraries (GLAD, GLFW, GLM, stb_image) live under `include/` and `lib/`.

## Architecture

All simulation logic lives in a single translation unit: `src/main.cpp` + `src/glad.c`. The four headers are utility classes with no cpp counterparts.

### Header classes

| File | Role |
|------|------|
| `Shader.h` | Wraps a GLSL program. Overloaded `set(name, value)` covers all uniform types (int, float, vec2–4, mat3, mat4). |
| `Camera.h` | FPS camera storing position + yaw/pitch. Call `moveForward/Backward/Left/Right/Up/Down(dt)`, `processMouse(dx,dy)`, `processScroll(y)`. `resetPosition()` returns to `(0,60,250)` looking at origin. |
| `Sphere.h` | UV sphere, unit radius, shared by every planet. Vertex layout: `[pos(3), normal(3), uv(2)]` at locations 0/1/2. UV uses `v = i/stacks` (not `1-i/stacks`) — this is correct together with `stbi_set_flip_vertically_on_load(true)`; inverting either one alone flips all planet textures upside-down. |
| `Planet.h` | Plain struct. `worldPos()` recurses through `parent` (only Moon→Earth is non-null). `modelMatrix()` applies: translate to `worldPos()` → axial tilt (Z-axis) → self-rotation (Y-axis) → uniform scale. |

### main.cpp structure

`main()` runs in this order:
1. GLFW/GLAD init (MSAA 4×, `GL_PROGRAM_POINT_SIZE` enabled)
2. Load five `Shader` programs from `shaders/`
3. Create shared `Sphere(64,64)`, `RingMesh`, stars VAO, orbit VAOs
4. Declare and configure the 10 `Planet` instances
5. Main loop → `processInput()` → update all planets with `dt = deltaTime * timeScale * (!paused)` → render

**Render order** (matters for blending):
1. Stars — `GL_POINTS`, `glDepthMask(GL_FALSE)`
2. Orbits — `GL_LINE_LOOP`, alpha blend, `uCenter` uniform offsets each circle to the right parent
3. Sun — self-emissive shader with `uTime` pulse
4. Planets + Moon — Blinn-Phong, `uLightPos = (0,0,0)` (Sun at origin)
5. Saturn ring — alpha blend, `glDisable(GL_CULL_FACE)`, double-sided lighting via `abs(dot)`

### Shader uniforms

All uniforms are prefixed `u`. `planet.frag` accepts `uFlashlight` (int 0/1) and `uFlashlightPos` (camera position) to add a camera-space point light and boost ambient to 0.45.

The orbit shader uses `uCenter` (vec3) to translate the pre-built circle VAO — set to `(0,0,0)` for planets orbiting the Sun, or `earth.worldPos()` for the Moon's orbit.

The Saturn ring texture is generated procedurally in `generateRingTexture()` — no external file needed.

### Key globals in main.cpp

- `timeScale` — multiplied into `dt`; toggled ×2/÷2 with `+`/`-`
- `paused`, `showOrbits`, `flashlight` — bool toggles with edge-detection pattern (`kX_prev`)
- `simTime` — accumulated simulation time independent of wall clock

### Known quirks

- Neptune's texture file is spelled **`nepturno.jpg`** (typo in filename). Any rename must match the `loadTexture()` call in `main.cpp`.
- `src/vertex.glsl` and `src/fragment.glsl` are leftovers from the previous terrain viewer; they are not used.
- All five shader programs are reloaded from disk every time the application starts (no hot-reload).
