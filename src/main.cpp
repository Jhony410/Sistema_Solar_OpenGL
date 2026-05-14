#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <cmath>

// ---- Constantes de ventana ----
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// ---- Camara orbital ----
float radius   = 400.0f;
float camYaw   = 45.0f;
float camPitch = 40.0f;

float lastX = SCR_WIDTH  / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool  firstMouse = true;

// ---- Rotacion del modelo ----
float modelRotY = 0.0f;

// ---- Wireframe ----
bool wireframe    = false;
bool fKeyPrevious = false;

// ---- Tiempo ----
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ================================================================
// SHADERS
// ================================================================
std::string cargarFuenteShader(const char* ruta) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "ERROR: no se pudo abrir " << ruta << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << archivo.rdbuf();
    return ss.str();
}

unsigned int crearProgramaShader(const char* rutaVertex, const char* rutaFragment) {
    std::string vSrc = cargarFuenteShader(rutaVertex);
    std::string fSrc = cargarFuenteShader(rutaFragment);
    const char* vCode = vSrc.c_str();
    const char* fCode = fSrc.c_str();

    int  ok;
    char log[512];

    unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vCode, nullptr);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(vert, 512, nullptr, log); std::cerr << "ERROR vertex:\n" << log << "\n"; }

    unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fCode, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &ok);
    if (!ok) { glGetShaderInfoLog(frag, 512, nullptr, log); std::cerr << "ERROR fragment:\n" << log << "\n"; }

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { glGetProgramInfoLog(prog, 512, nullptr, log); std::cerr << "ERROR link:\n" << log << "\n"; }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return prog;
}

// ================================================================
// CARGADOR OBJ
// ================================================================
struct VertCara {
    int posIdx; // 0-based
    int uvIdx;  // 0-based, -1 si ausente
};

// Parsea un token de cara: "123", "123/456", "123/456/789", "123//789"
VertCara parsearVertCara(const std::string& tok) {
    VertCara vc{ -1, -1 };
    size_t s1 = tok.find('/');
    if (s1 == std::string::npos) {
        vc.posIdx = std::stoi(tok) - 1;
    } else {
        vc.posIdx = std::stoi(tok.substr(0, s1)) - 1;
        size_t s2 = tok.find('/', s1 + 1);
        std::string uvStr = (s2 == std::string::npos)
            ? tok.substr(s1 + 1)
            : tok.substr(s1 + 1, s2 - s1 - 1);
        if (!uvStr.empty())
            vc.uvIdx = std::stoi(uvStr) - 1;
    }
    return vc;
}

// Devuelve vector de floats intercalados: posicion(3) + normal(3) por cada vertice
std::vector<float> cargarOBJ(const char* ruta, int& outNumTriangulos, float& outTileW, float& outTileD) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        std::cerr << "ERROR: no se pudo abrir " << ruta << "\n";
        return {};
    }

    std::vector<glm::vec3> tempPos;
    std::vector<glm::vec2> tempUV;

    // Cada triangulo: 3 VertCara
    std::vector<std::array<VertCara, 3>> triangulos;

    // Indices globales de inicio del objeto de terreno
    // El OBJ puede tener objetos extra (ej. "Sphere" = cupula de cielo)
    // Solo cargamos objetos que NO sean "Sphere"
    bool objetoActivo = true; // si no hay linea "o", cargamos todo
    int  offsetPos    = 0;    // vertices acumulados antes del objeto actual
    int  posInicioObj = 0;    // cuantos verts tenia tempPos al entrar al objeto

    std::string linea;
    while (std::getline(archivo, linea)) {
        if (linea.empty() || linea[0] == '#') continue;
        std::istringstream iss(linea);
        std::string prefijo;
        iss >> prefijo;

        if (prefijo == "o") {
            // Nuevo objeto: actualizamos offset y decidimos si procesarlo
            offsetPos    = (int)tempPos.size();
            posInicioObj = offsetPos;
            std::string nombreObj;
            iss >> nombreObj;
            // Saltar la cupula de cielo
            objetoActivo = (nombreObj != "Sphere");
            if (objetoActivo)
                std::cout << "Cargando objeto: " << nombreObj << "\n";
            else
                std::cout << "Saltando objeto : " << nombreObj << "\n";

        } else if (prefijo == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            // Siempre acumulamos posiciones para mantener los indices globales correctos
            tempPos.push_back({ x, y, z });

        } else if (prefijo == "vt") {
            float u, v;
            iss >> u >> v;
            tempUV.push_back({ u, v });

        } else if (prefijo == "f" && objetoActivo) {
            // Solo triangulamos caras del objeto activo
            std::vector<VertCara> caraVerts;
            std::string tok;
            while (iss >> tok)
                caraVerts.push_back(parsearVertCara(tok));

            // Fan triangulation: (0,1,2), (0,2,3), ...
            for (size_t i = 1; i + 1 < caraVerts.size(); ++i) {
                std::array<VertCara, 3> tri = { caraVerts[0], caraVerts[i], caraVerts[i + 1] };
                triangulos.push_back(tri);
            }
        }
    }

    // ------ Centrar el terreno en el origen (bounding box solo de vertices usados) ------
    // tempPos contiene vertices de TODOS los objetos (incluido Sphere), por eso se
    // mide solo sobre los vertices que realmente referencian los triangulos del terreno
    glm::vec3 bMin( 1e9f), bMax(-1e9f);
    for (auto& tri : triangulos) {
        for (auto& vc : tri) {
            glm::vec3 p = tempPos[vc.posIdx];
            bMin = glm::min(bMin, p);
            bMax = glm::max(bMax, p);
        }
    }
    outTileW = bMax.x - bMin.x;
    outTileD = bMax.z - bMin.z;
    glm::vec3 centro = (bMin + bMax) * 0.5f;
    for (auto& p : tempPos) {
        p -= centro;
    }

    // ------ Calcular normales por vertice (acumulacion de normales de cara) ------
    // Se usa la normal de cara ponderada por area (sin normalizar el cross product)
    // para que caras grandes aporten mas peso → smooth shading de mayor calidad
    std::vector<glm::vec3> normalAcum(tempPos.size(), glm::vec3(0.0f));

    for (auto& tri : triangulos) {
        glm::vec3 p0 = tempPos[tri[0].posIdx];
        glm::vec3 p1 = tempPos[tri[1].posIdx];
        glm::vec3 p2 = tempPos[tri[2].posIdx];
        glm::vec3 fn = glm::cross(p1 - p0, p2 - p0); // ponderado por area
        normalAcum[tri[0].posIdx] += fn;
        normalAcum[tri[1].posIdx] += fn;
        normalAcum[tri[2].posIdx] += fn;
    }
    for (auto& n : normalAcum)
        if (glm::length(n) > 1e-6f) n = glm::normalize(n);

    // ------ Construir buffer intercalado (pos + normal) ------
    std::vector<float> verts;
    verts.reserve(triangulos.size() * 3 * 6);

    for (auto& tri : triangulos) {
        for (auto& vc : tri) {
            glm::vec3 p = tempPos[vc.posIdx];
            glm::vec3 n = normalAcum[vc.posIdx];
            verts.push_back(p.x); verts.push_back(p.y); verts.push_back(p.z);
            verts.push_back(n.x); verts.push_back(n.y); verts.push_back(n.z);
        }
    }

    outNumTriangulos = (int)triangulos.size();
    std::cout << "Vertices cargados : " << tempPos.size() << "\n";
    std::cout << "Triangulos        : " << outNumTriangulos << "\n";
    return verts;
}

// ================================================================
// CALLBACKS
// ================================================================
void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

void mouse_callback(GLFWwindow*, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }

    float xoff = (xpos - lastX) * 0.2f;
    float yoff = (lastY - ypos) * 0.2f;
    lastX = xpos; lastY = ypos;

    camYaw   += xoff;
    camPitch += yoff;
    if (camPitch >  89.0f) camPitch =  89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;
}

void scroll_callback(GLFWwindow*, double, double yoffset) {
    radius -= (float)yoffset * 10.0f;
    if (radius <   5.0f) radius =   5.0f;
    if (radius > 800.0f) radius = 800.0f;
}

// ================================================================
// ENTRADA DE TECLADO
// ================================================================
void procesarEntrada(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // W/S → zoom (radio)
    float speed = 120.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) radius -= speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) radius += speed;
    if (radius <   5.0f) radius =   5.0f;
    if (radius > 800.0f) radius = 800.0f;

    // A/D → rotacion del modelo en Y
    float rotSpeed = 90.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) modelRotY += rotSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) modelRotY -= rotSpeed;

    // F → toggle wireframe (deteccion de flanco)
    bool fKeyNow = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (fKeyNow && !fKeyPrevious) {
        wireframe = !wireframe;
        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            std::cout << "Wireframe: ON\n";
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            std::cout << "Wireframe: OFF\n";
        }
    }
    fKeyPrevious = fKeyNow;
}

// ================================================================
// MAIN
// ================================================================
int main() {
    // -- Inicializar GLFW --
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Visor Terreno OBJ - SIS226", nullptr, nullptr);
    if (!window) {
        std::cerr << "ERROR: no se pudo crear la ventana GLFW\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window,       mouse_callback);
    glfwSetScrollCallback(window,          scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR,  GLFW_CURSOR_DISABLED);

    // -- Inicializar GLAD --
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "ERROR: no se pudo inicializar GLAD\n";
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // -- Shaders --
    unsigned int shaderProgram = crearProgramaShader("src/vertex.glsl", "src/fragment.glsl");

    // -- Cargar OBJ --
    int   numTriangulos = 0;
    float tileW = 64.0f, tileD = 64.0f;
    std::vector<float> vertices = cargarOBJ("SnowTerrain.obj", numTriangulos, tileW, tileD);
    if (vertices.empty()) {
        std::cerr << "ERROR: no se pudieron cargar vertices del OBJ\n";
        return -1;
    }
    int numVertices = (int)(vertices.size() / 6);
    std::cout << "Tile size: " << tileW << " x " << tileD << " unidades\n";

    // -- VAO / VBO --
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        (GLsizeiptr)(vertices.size() * sizeof(float)),
        vertices.data(), GL_STATIC_DRAW);

    // Posicion (location 0) → primeros 3 floats del stride de 6
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location 1) → floats 3-5 del stride de 6
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // ================================================================
    // BUCLE PRINCIPAL
    // ================================================================
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        procesarEntrada(window);

        glClearColor(0.10f, 0.12f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // -- Posicion de camara (coordenadas esfericas) --
        float yawRad   = glm::radians(camYaw);
        float pitchRad = glm::radians(camPitch);
        glm::vec3 camPos(
            radius * cosf(pitchRad) * sinf(yawRad),
            radius * sinf(pitchRad),
            radius * cosf(pitchRad) * cosf(yawRad)
        );

        // -- Luz orbital automatica (alta para iluminar todo el grid) --
        glm::vec3 lightPos(
            sinf(currentFrame * 0.3f) * 300.0f,
            250.0f,
            cosf(currentFrame * 0.3f) * 300.0f
        );

        // -- Matrices --
        glm::mat4 view  = glm::lookAt(camPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj  = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 3000.0f);

        // -- Uniforms constantes por frame --
        glUseProgram(shaderProgram);
        int locModel = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"),  1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"),   1, glm::value_ptr(camPos));
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"),  1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.85f, 0.90f, 0.95f);

        // -- Dibujar grid 11x11 de tiles (5 en cada direccion) --
        const int TILE_RANGE = 5;
        glm::mat4 globalRot = glm::rotate(glm::mat4(1.0f),
            glm::radians(modelRotY), glm::vec3(0.0f, 1.0f, 0.0f));

        glBindVertexArray(VAO);
        for (int ix = -TILE_RANGE; ix <= TILE_RANGE; ++ix) {
            for (int iz = -TILE_RANGE; iz <= TILE_RANGE; ++iz) {
                glm::mat4 tileTrans = glm::translate(glm::mat4(1.0f),
                    glm::vec3(ix * tileW, 0.0f, iz * tileD));
                glm::mat4 tileModel = globalRot * tileTrans;
                glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(tileModel));
                glDrawArrays(GL_TRIANGLES, 0, numVertices);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
