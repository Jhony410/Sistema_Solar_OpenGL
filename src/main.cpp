// ============================================================
//  Sistema Solar 3D  -  OpenGL 3.3 Core Profile
//  Computacion Grafica  -  2026-I
// ============================================================

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "Shader.h"
#include "Camera.h"
#include "Sphere.h"
#include "Planet.h"

// ============================================================
// CONSTANTES DE ESCENA
// ============================================================
static const int   SCR_W      = 1280;
static const int   SCR_H      = 720;
static const int   ORBIT_SEG  = 180;   // segmentos por linea de orbita
static const int   NUM_STARS  = 4000;  // estrellas del fondo espacial
static const float PI         = 3.14159265358979f;

// ============================================================
// ESTADO GLOBAL (input / tiempo)
// ============================================================
Camera camera;
float  lastX        = SCR_W * 0.5f;
float  lastY        = SCR_H * 0.5f;
bool   firstMouse   = true;
float  deltaTime    = 0.0f;
float  lastFrame    = 0.0f;
float  simTime      = 0.0f;   // tiempo de simulacion acumulado
float  timeScale    = 1.0f;   // 1.0 = velocidad normal
bool   paused       = false;
bool   showOrbits   = true;

// Deteccion de flanco para teclas de toggle
bool kO_prev = false, kP_prev = false;
bool kPlus_prev = false, kMinus_prev = false;
bool kR_prev = false, kL_prev = false;
bool flashlight = false;   // destello de emergencia (tecla L)

// ============================================================
// HELPERS MATEMATICOS
// ============================================================
inline float smoothstepf(float e0, float e1, float x) {
    float t = std::max(0.0f, std::min((x - e0) / (e1 - e0), 1.0f));
    return t * t * (3.0f - 2.0f * t);
}

// ============================================================
// CALLBACKS
// ============================================================
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; return; }
    float dx =  (float)(xpos - lastX);
    float dy = -(float)(ypos - lastY);   // invertido: pantalla Y baja, camara sube
    lastX = (float)xpos;
    lastY = (float)ypos;
    camera.processMouse(dx, dy);
}

void scroll_callback(GLFWwindow*, double, double y) {
    camera.processScroll((float)y);
}

// ============================================================
// CARGA DE TEXTURA DESDE ARCHIVO
// ============================================================
unsigned int loadTexture(const std::string& path) {
    unsigned int id;
    glGenTextures(1, &id);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);

    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : (ch == 3 ? GL_RGB : GL_RED);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
        stbi_image_free(data);
        std::cout << "[Textura] OK : " << path
                  << "  (" << w << "x" << h << " ch=" << ch << ")\n";
    } else {
        std::cerr << "[Textura] ERR: " << path << "\n";
        unsigned char pix[] = {100, 100, 160, 255};
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pix);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return id;
}

// ============================================================
// TEXTURA PROCEDURAL DE LOS ANILLOS DE SATURNO
// Genera una franja horizontal con bandas de color y opacidad
// similares a los anillos reales (D, C, B, Cassini, A, F)
// ============================================================
unsigned int generateRingTexture() {
    const int W = 512, H = 4;
    std::vector<unsigned char> px(W * H * 4, 0);

    std::mt19937 rng(7777);
    std::uniform_real_distribution<float> jitter(-0.03f, 0.03f);

    for (int x = 0; x < W; ++x) {
        float t = (float)x / (W - 1);   // 0 = borde interior, 1 = borde exterior

        float r, g, b, a;
        r = 0.88f; g = 0.80f; b = 0.64f; a = 0.0f;

        if      (t < 0.05f)           { a = t/0.05f * 0.20f; r=0.70f; g=0.65f; b=0.55f; }
        else if (t < 0.06f)           { a = smoothstepf(0.05f,0.06f,t) * 0.20f; }
        // Anillo C
        else if (t < 0.22f)           { a = 0.38f; r=0.72f; g=0.67f; b=0.58f; }
        else if (t < 0.235f)          { a = 0.05f; }        // hueco Maxwell
        // Anillo B (el mas denso y brillante)
        else if (t < 0.44f)           { a = 0.92f; r=0.96f; g=0.89f; b=0.72f; }
        else if (t < 0.455f)          { a = 0.70f; r=0.90f; g=0.84f; b=0.68f; }
        // Division de Cassini
        else if (t < 0.52f) {
            float f = (t - 0.44f) / (0.52f - 0.44f);
            a = (1.0f - smoothstepf(0.0f, 1.0f, f)) * 0.88f;
            if (a < 0.04f) a = 0.04f;
        }
        // Anillo A
        else if (t < 0.70f)           { a = 0.78f; r=0.93f; g=0.86f; b=0.70f; }
        // Hueco de Encke (delgado)
        else if (t < 0.725f)          { a = 0.08f; }
        else if (t < 0.80f)           { a = 0.68f; r=0.90f; g=0.83f; b=0.68f; }
        // Anillo F (tenue y angosto)
        else if (t < 0.84f)           { a = 0.28f; r=0.80f; g=0.74f; b=0.62f; }
        // Borde exterior: desvanecimiento
        else { a = smoothstepf(1.0f, 0.84f, t) * 0.18f; }

        // Variacion aleatoria de densidad para que no sea tan uniforme
        a = std::max(0.0f, std::min(1.0f, a + jitter(rng)));

        for (int y = 0; y < H; ++y) {
            int idx = (y * W + x) * 4;
            px[idx+0] = (unsigned char)(r * 255);
            px[idx+1] = (unsigned char)(g * 255);
            px[idx+2] = (unsigned char)(b * 255);
            px[idx+3] = (unsigned char)(a * 255);
        }
    }

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

// ============================================================
// VAO DE LINEA DE ORBITA  (circulo centrado en el origen)
// En el shader se suma uCenter para trasladar al padre correcto
// ============================================================
GLuint createOrbitVAO(float radius) {
    std::vector<float> v;
    v.reserve(ORBIT_SEG * 3);
    for (int i = 0; i < ORBIT_SEG; ++i) {
        float a = 2.0f * PI * i / ORBIT_SEG;
        v.push_back(radius * cosf(a));
        v.push_back(0.0f);
        v.push_back(radius * sinf(a));
    }
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size()*sizeof(float)),
        v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return vao;
}

// ============================================================
// GEOMETRIA DEL ANILLO DE SATURNO
// ============================================================
struct RingMesh {
    GLuint vao, vbo, ebo;
    int    indexCount;
};

RingMesh createRingMesh(float inner, float outer, int segs = 160) {
    std::vector<float>        verts;
    std::vector<unsigned int> inds;

    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * PI * i / segs;
        float c = cosf(a), s = sinf(a);
        float u = (float)i / segs;

        verts.insert(verts.end(), {inner*c, 0.f, inner*s,  0.f,1.f,0.f,  u, 0.f});
        verts.insert(verts.end(), {outer*c, 0.f, outer*s,  0.f,1.f,0.f,  u, 1.f});
    }

    for (int i = 0; i < segs; ++i) {
        unsigned int b = i * 2;
        inds.push_back(b);   inds.push_back(b+1); inds.push_back(b+2);
        inds.push_back(b+2); inds.push_back(b+1); inds.push_back(b+3);
    }

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size()*sizeof(float)),
        verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(inds.size()*sizeof(unsigned int)),
        inds.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    return {vao, vbo, ebo, (int)inds.size()};
}

// ============================================================
// CAMPO DE ESTRELLAS  (puntos aleatorios en esfera grande)
// ============================================================
GLuint createStarsVAO() {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> d01(0.0f, 1.0f);

    std::vector<float> v;
    v.reserve(NUM_STARS * 4);

    for (int i = 0; i < NUM_STARS; ++i) {
        float theta = 2.0f * PI * d01(rng);
        float phi   = acosf(2.0f * d01(rng) - 1.0f);
        float R     = 2600.0f;

        v.push_back(R * sinf(phi) * cosf(theta));
        v.push_back(R * cosf(phi));
        v.push_back(R * sinf(phi) * sinf(theta));
        v.push_back(d01(rng) * 0.7f + 0.3f);   // brillo 0.3..1.0
    }

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size()*sizeof(float)),
        v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return vao;
}

// ============================================================
// PROCESAR ENTRADA DE TECLADO
// ============================================================
void processInput(GLFWwindow* win) {
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // Movimiento de camara (WASD + EQ para subir/bajar)
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) camera.moveForward (deltaTime);
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) camera.moveBackward(deltaTime);
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) camera.moveLeft    (deltaTime);
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) camera.moveRight   (deltaTime);
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) camera.moveUp      (deltaTime);
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) camera.moveDown    (deltaTime);

    // Toggle orbitas (O) - flanco de subida
    bool kO = (glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS);
    if (kO && !kO_prev) showOrbits = !showOrbits;
    kO_prev = kO;

    // Pausa / Resume (P)
    bool kP = (glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS);
    if (kP && !kP_prev) {
        paused = !paused;
        std::cout << (paused ? "[Sim] PAUSADO\n" : "[Sim] REANUDADO\n");
    }
    kP_prev = kP;

    // Acelerar simulacion (+)
    bool kPlus = (glfwGetKey(win, GLFW_KEY_EQUAL) == GLFW_PRESS ||
                  glfwGetKey(win, GLFW_KEY_KP_ADD) == GLFW_PRESS);
    if (kPlus && !kPlus_prev) {
        timeScale = std::min(timeScale * 2.0f, 64.0f);
        std::cout << "[Sim] Velocidad x" << timeScale << "\n";
    }
    kPlus_prev = kPlus;

    // Ralentizar simulacion (-)
    bool kMinus = (glfwGetKey(win, GLFW_KEY_MINUS)    == GLFW_PRESS ||
                   glfwGetKey(win, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS);
    if (kMinus && !kMinus_prev) {
        timeScale = std::max(timeScale * 0.5f, 0.0625f);
        std::cout << "[Sim] Velocidad x" << timeScale << "\n";
    }
    kMinus_prev = kMinus;

    // Resetear camara (R)
    bool kR = (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
    if (kR && !kR_prev) camera.resetPosition();
    kR_prev = kR;

    // Destello de emergencia (L) - ilumina todo desde la camara
    bool kL = (glfwGetKey(win, GLFW_KEY_L) == GLFW_PRESS);
    if (kL && !kL_prev) {
        flashlight = !flashlight;
        std::cout << (flashlight ? "[Luz] Destello ON\n" : "[Luz] Destello OFF\n");
    }
    kL_prev = kL;
}

// ============================================================
// MAIN
// ============================================================
int main() {
    // ----------------------------------------------------------
    // 1. INICIALIZACION DE GLFW Y OPENGL
    // ----------------------------------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);   // MSAA 4x

    GLFWwindow* win = glfwCreateWindow(SCR_W, SCR_H,
        "Sistema Solar 3D  |  WASD=mover  Mouse=rotar  Scroll=velocidad  O=orbitas  P=pausa  R=reset",
        nullptr, nullptr);
    if (!win) {
        std::cerr << "ERROR: no se pudo crear la ventana.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSetCursorPosCallback      (win, mouse_callback);
    glfwSetScrollCallback         (win, scroll_callback);
    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: no se pudo inicializar GLAD.\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE);    // para gl_PointSize en el vertex shader

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n"
              << "Renderer: " << glGetString(GL_RENDERER) << "\n\n";

    // ----------------------------------------------------------
    // 2. CARGA DE SHADERS
    // ----------------------------------------------------------
    Shader shPlanet ("shaders/planet.vert", "shaders/planet.frag");
    Shader shSun    ("shaders/sun.vert",    "shaders/sun.frag");
    Shader shOrbit  ("shaders/orbit.vert",  "shaders/orbit.frag");
    Shader shRing   ("shaders/ring.vert",   "shaders/ring.frag");
    Shader shStars  ("shaders/stars.vert",  "shaders/stars.frag");

    // ----------------------------------------------------------
    // 3. GEOMETRIA COMPARTIDA
    // ----------------------------------------------------------
    Sphere sphere(64, 64);     // todos los planetas usan la misma esfera

    RingMesh ring = createRingMesh(2.6f, 5.0f);   // anillos de Saturno
    GLuint   ringTex  = generateRingTexture();

    GLuint starsVAO = createStarsVAO();

    // ----------------------------------------------------------
    // 4. DEFINICION DE PLANETAS
    // Escala visual (no astronomica) diseñada para apreciarse bien
    //   radius     = radio visual del planeta
    //   orbitRadius= distancia al padre
    //   orbitSpeed = velocidad angular rad/s de simulacion
    //   rotSpeed   = velocidad de rotacion propia rad/s
    //   axialTilt  = inclinacion del eje en grados
    // ----------------------------------------------------------
    Planet sol, mercury, venus, earth, moon, mars, jupiter, saturn, uranus, neptune;

    // Sol -------------------------------------------------------
    sol.name          = "Sol";
    sol.radius        = 5.0f;
    sol.orbitRadius   = 0.0f;
    sol.orbitSpeed    = 0.0f;
    sol.rotationSpeed = 0.04f;
    sol.axialTilt     = 7.25f;
    sol.texture       = loadTexture("texturas/sol.jpg");

    // Mercurio --------------------------------------------------
    mercury.name          = "Mercurio";
    mercury.radius        = 0.35f;
    mercury.orbitRadius   = 10.0f;
    mercury.orbitSpeed    = 1.0f;
    mercury.rotationSpeed = 0.018f;
    mercury.axialTilt     = 0.03f;
    mercury.texture       = loadTexture("texturas/mercurio.jpg");

    // Venus -----------------------------------------------------
    venus.name          = "Venus";
    venus.radius        = 0.87f;
    venus.orbitRadius   = 17.0f;
    venus.orbitSpeed    = 0.46f;
    venus.rotationSpeed = -0.012f;    // rotacion retrograda
    venus.axialTilt     = 177.4f;
    venus.texture       = loadTexture("texturas/venus.jpg");

    // Tierra ----------------------------------------------------
    earth.name          = "Tierra";
    earth.radius        = 0.92f;
    earth.orbitRadius   = 25.0f;
    earth.orbitSpeed    = 0.25f;
    earth.rotationSpeed = 0.80f;
    earth.axialTilt     = 23.5f;
    earth.texture       = loadTexture("texturas/tierra.jpg");

    // Luna (orbita la Tierra) -----------------------------------
    moon.name          = "Luna";
    moon.radius        = 0.25f;
    moon.orbitRadius   = 3.2f;
    moon.orbitSpeed    = 3.4f;        // 13.4x mas rapida que la Tierra
    moon.rotationSpeed = 0.08f;       // rotacion lenta (casi bloqueada)
    moon.axialTilt     = 6.68f;
    moon.texture       = loadTexture("texturas/luna.jpg");
    moon.parent        = &earth;      // orbita alrededor de la Tierra

    // Marte -----------------------------------------------------
    mars.name          = "Marte";
    mars.radius        = 0.49f;
    mars.orbitRadius   = 34.0f;
    mars.orbitSpeed    = 0.153f;
    mars.rotationSpeed = 0.77f;
    mars.axialTilt     = 25.2f;
    mars.texture       = loadTexture("texturas/marte.jpg");

    // Jupiter ---------------------------------------------------
    jupiter.name          = "Jupiter";
    jupiter.radius        = 2.5f;
    jupiter.orbitRadius   = 54.0f;
    jupiter.orbitSpeed    = 0.058f;
    jupiter.rotationSpeed = 2.0f;
    jupiter.axialTilt     = 3.1f;
    jupiter.texture       = loadTexture("texturas/jupiter.jpg");

    // Saturno ---------------------------------------------------
    saturn.name          = "Saturno";
    saturn.radius        = 2.0f;
    saturn.orbitRadius   = 76.0f;
    saturn.orbitSpeed    = 0.034f;
    saturn.rotationSpeed = 1.85f;
    saturn.axialTilt     = 26.7f;
    saturn.texture       = loadTexture("texturas/saturno.jpg");

    // Urano -----------------------------------------------------
    uranus.name          = "Urano";
    uranus.radius        = 1.5f;
    uranus.orbitRadius   = 97.0f;
    uranus.orbitSpeed    = 0.023f;
    uranus.rotationSpeed = -1.5f;     // rotacion retrograda
    uranus.axialTilt     = 97.77f;    // eje casi horizontal
    uranus.texture       = loadTexture("texturas/urano.jpg");

    // Neptuno ---------------------------------------------------
    neptune.name          = "Neptuno";
    neptune.radius        = 1.4f;
    neptune.orbitRadius   = 116.0f;
    neptune.orbitSpeed    = 0.016f;
    neptune.rotationSpeed = 1.6f;
    neptune.axialTilt     = 28.3f;
    neptune.texture       = loadTexture("texturas/nepturno.jpg");   // nombre real del archivo

    // Lista de todos los cuerpos (excepto Sol) para iterar facilmente
    std::vector<Planet*> bodies = {
        &mercury, &venus, &earth, &moon,
        &mars, &jupiter, &saturn, &uranus, &neptune
    };

    // ----------------------------------------------------------
    // 5. LINEAS DE ORBITA
    // OrbitInfo: VAO del circulo, y puntero al planeta padre (null = Sol)
    // ----------------------------------------------------------
    struct OrbitInfo { GLuint vao; Planet* center; };

    std::vector<OrbitInfo> orbits = {
        { createOrbitVAO(mercury.orbitRadius), nullptr    },
        { createOrbitVAO(venus.orbitRadius),   nullptr    },
        { createOrbitVAO(earth.orbitRadius),   nullptr    },
        { createOrbitVAO(moon.orbitRadius),    &earth     },
        { createOrbitVAO(mars.orbitRadius),    nullptr    },
        { createOrbitVAO(jupiter.orbitRadius), nullptr    },
        { createOrbitVAO(saturn.orbitRadius),  nullptr    },
        { createOrbitVAO(uranus.orbitRadius),  nullptr    },
        { createOrbitVAO(neptune.orbitRadius), nullptr    },
    };

    // ----------------------------------------------------------
    // 6. ESTADO FPS (para titulo de ventana)
    // ----------------------------------------------------------
    double fpsTimer   = 0.0;
    int    fpsFrames  = 0;
    float  fpsDisplay = 0.0f;

    // ----------------------------------------------------------
    // 7. BUCLE PRINCIPAL
    // ----------------------------------------------------------
    while (!glfwWindowShouldClose(win)) {

        // --- Tiempo ---
        float now   = (float)glfwGetTime();
        deltaTime   = now - lastFrame;
        lastFrame   = now;

        // --- Avanzar simulacion (se detiene con P, acelera con +/-) ---
        float dt = paused ? 0.0f : deltaTime * timeScale;
        simTime += dt;

        for (Planet* p : bodies) p->update(dt);
        sol.update(dt);

        // --- FPS counter en titulo ---
        fpsFrames++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 0.5) {
            fpsDisplay = fpsFrames / (float)fpsTimer;
            fpsFrames  = 0;
            fpsTimer   = 0.0;

            std::ostringstream title;
            title << "Sistema Solar 3D  |  "
                  << std::fixed << std::setprecision(0) << fpsDisplay << " FPS"
                  << "  |  x" << timeScale
                  << (paused     ? "  [PAUSADO]"       : "")
                  << (flashlight ? "  [DESTELLO ON]"   : "")
                  << "  |  WASD+EQ=mover  O=orbitas  P=pausa  L=destello  R=reset  +/-=vel";
            glfwSetWindowTitle(win, title.str().c_str());
        }

        // --- Input ---
        processInput(win);

        // --- Limpiar buffers ---
        glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Matrices de vista y proyeccion ---
        glm::mat4 view = camera.getView();
        glm::mat4 proj = glm::perspective(
            glm::radians(camera.fov),
            (float)SCR_W / SCR_H,
            0.5f, 6000.0f
        );

        // ===========================================================
        // A. ESTRELLAS (primero, con escritura de profundidad apagada)
        // ===========================================================
        glDepthMask(GL_FALSE);
        shStars.use();
        shStars.set("view",       view);
        shStars.set("projection", proj);
        glBindVertexArray(starsVAO);
        glDrawArrays(GL_POINTS, 0, NUM_STARS);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        // ===========================================================
        // B. ORBITAS (lineas semitransparentes)
        // ===========================================================
        if (showOrbits) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            shOrbit.use();
            shOrbit.set("view",       view);
            shOrbit.set("projection", proj);
            shOrbit.set("uColor",     glm::vec4(0.55f, 0.65f, 0.85f, 0.25f));

            for (auto& o : orbits) {
                glm::vec3 center = o.center ? o.center->worldPos() : glm::vec3(0.0f);
                shOrbit.set("uCenter", center);
                glBindVertexArray(o.vao);
                glDrawArrays(GL_LINE_LOOP, 0, ORBIT_SEG);
                glBindVertexArray(0);
            }

            glDisable(GL_BLEND);
        }

        // ===========================================================
        // C. SOL  (auto-iluminado, shader especial)
        // ===========================================================
        {
            shSun.use();
            shSun.set("view",       view);
            shSun.set("projection", proj);
            shSun.set("uTime",      now);

            glm::mat4 sunModel = sol.modelMatrix();
            shSun.set("model", sunModel);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, sol.texture);
            shSun.set("uTexture", 0);

            sphere.draw();
        }

        // ===========================================================
        // D. PLANETAS Y LUNA  (iluminacion Phong desde el Sol)
        // ===========================================================
        {
            shPlanet.use();
            shPlanet.set("view",         view);
            shPlanet.set("projection",   proj);
            shPlanet.set("uLightPos",      glm::vec3(0.0f));  // Sol en el origen
            shPlanet.set("uViewPos",       camera.position);
            shPlanet.set("uLightColor",    glm::vec3(1.0f, 0.95f, 0.82f));
            shPlanet.set("uAmbient",       0.07f);
            shPlanet.set("uSpecularStr",   0.25f);
            shPlanet.set("uShininess",     24.0f);
            shPlanet.set("uTexture",       0);
            shPlanet.set("uFlashlight",    flashlight ? 1 : 0);
            shPlanet.set("uFlashlightPos", camera.position);

            for (Planet* p : bodies) {
                glm::mat4 model  = p->modelMatrix();
                glm::mat3 normal = glm::mat3(glm::transpose(glm::inverse(model)));

                shPlanet.set("model",        model);
                shPlanet.set("normalMatrix", normal);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, p->texture);

                sphere.draw();
            }
        }

        // ===========================================================
        // E. ANILLOS DE SATURNO  (semitransparentes, doble cara)
        // ===========================================================
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // Desactivar culling para ver el anillo desde arriba y abajo
            glDisable(GL_CULL_FACE);

            shRing.use();
            shRing.set("view",       view);
            shRing.set("projection", proj);
            shRing.set("uLightPos",  glm::vec3(0.0f));
            shRing.set("uLightColor",glm::vec3(1.0f, 0.95f, 0.82f));
            shRing.set("uAmbient",   0.18f);
            shRing.set("uTexture",   0);

            glm::mat4 ringModel(1.0f);
            ringModel = glm::translate(ringModel, saturn.worldPos());
            ringModel = glm::rotate(ringModel,
                glm::radians(saturn.axialTilt), glm::vec3(0.0f, 0.0f, 1.0f));

            glm::mat3 ringNorm = glm::mat3(glm::transpose(glm::inverse(ringModel)));

            shRing.set("model",        ringModel);
            shRing.set("normalMatrix", ringNorm);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ringTex);

            glBindVertexArray(ring.vao);
            glDrawElements(GL_TRIANGLES, ring.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);

            glDisable(GL_BLEND);
            // Restaurar culling de cara frontal (OpenGL no lo activa por defecto,
            // pero es buena practica dejarlo consistente)
        }

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    // ----------------------------------------------------------
    // 8. LIMPIEZA
    // ----------------------------------------------------------
    for (auto& o : orbits) glDeleteVertexArrays(1, &o.vao);
    glDeleteVertexArrays(1, &starsVAO);
    glDeleteVertexArrays(1, &ring.vao);
    glDeleteBuffers(1, &ring.vbo);
    glDeleteBuffers(1, &ring.ebo);
    glDeleteTextures(1, &ringTex);

    glfwTerminate();
    return 0;
}
