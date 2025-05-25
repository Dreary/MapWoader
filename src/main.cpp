#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
namespace fs = std::filesystem;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "stb_image.h"
#include "tinyxml2.h"
using namespace tinyxml2;

#include "Math/Vector3.h"
#include <Engine/Math/Vector3S.h>
#include "Objects/Transform.h"

#include "VulkanGraphics/FileFormats/NifParser.h"
#include "VulkanGraphics/FileFormats/PackageNodes.h"
#include "VulkanGraphics/Scene/MeshData.h"

#include "MeshLoader.h"
#include "block.h"
#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture_manager.h"
#include "textureloader.h"

class DualStreamBuf : public std::streambuf
{
public:
    DualStreamBuf(std::streambuf *buf1, std::streambuf *buf2)
        : m_buf1(buf1), m_buf2(buf2) {}

protected:
    virtual int overflow(int c) override
    {
        if (c == EOF)
            return !EOF;
        if (m_buf1)
            m_buf1->sputc(c);
        if (m_buf2)
            m_buf2->sputc(c);
        return c;
    }

    virtual int sync() override
    {
        int r1 = m_buf1 ? m_buf1->pubsync() : 0;
        int r2 = m_buf2 ? m_buf2->pubsync() : 0;
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }

private:
    std::streambuf *m_buf1;
    std::streambuf *m_buf2;
};

GLuint CreateWhiteTexture()
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    unsigned char whitePixel[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texID;
}

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Shader *shader = nullptr;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window, Camera &camera)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (camera.EnableRotation && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.Position += glm::vec3(0.0f, 1.0f, 0.0f) * camera.MovementSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
    if (camera.EnableRotation && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.Position -= glm::vec3(0.0f, 1.0f, 0.0f) * camera.MovementSpeed * deltaTime;
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    static bool firstMouse = true;
    static float lastX = 400, lastY = 300;
    Camera *camera = reinterpret_cast<Camera *>(glfwGetWindowUserPointer(window));
    if (!camera)
        return;

    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    if (camera->EnableRotation)
        camera->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    Camera *camera = reinterpret_cast<Camera *>(glfwGetWindowUserPointer(window));
    if (camera)
        camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

bool RayIntersectsAABB(const glm::vec3 &rayOrigin, const glm::vec3 &rayDir,
                       const glm::vec3 &aabbMin, const glm::vec3 &aabbMax, float &t)
{
    float tmin = (aabbMin.x - rayOrigin.x) / rayDir.x;
    float tmax = (aabbMax.x - rayOrigin.x) / rayDir.x;
    if (tmin > tmax)
        std::swap(tmin, tmax);

    float tymin = (aabbMin.y - rayOrigin.y) / rayDir.y;
    float tymax = (aabbMax.y - rayOrigin.y) / rayDir.y;
    if (tymin > tymax)
        std::swap(tymin, tymax);
    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (aabbMin.z - rayOrigin.z) / rayDir.z;
    float tzmax = (aabbMax.z - rayOrigin.z) / rayDir.z;
    if (tzmin > tzmax)
        std::swap(tzmin, tzmax);
    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    t = tmin;
    return true;
}

void DrawWireCubeModern(const glm::vec3 &center, const glm::vec3 &halfSize, const glm::mat4 &view, const glm::mat4 &projection, Shader *shader)
{
    static GLuint vao = 0, vbo = 0, ebo = 0;
    if (!vao)
    {
        glm::vec3 vertices[8];
        GLuint indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0, // bottom
            4, 5, 5, 6, 6, 7, 7, 4, // top
            0, 4, 1, 5, 2, 6, 3, 7  // sides
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), nullptr, GL_DYNAMIC_DRAW); // filled later

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
        glEnableVertexAttribArray(0);
    }

    // Update vertex buffer
    glm::vec3 min = center - halfSize;
    glm::vec3 max = center + halfSize;
    glm::vec3 verts[8] = {
        {min.x, min.y, min.z}, {max.x, min.y, min.z}, {max.x, max.y, min.z}, {min.x, max.y, min.z}, {min.x, min.y, max.z}, {max.x, min.y, max.z}, {max.x, max.y, max.z}, {min.x, max.y, max.z}};
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    // Use your shader
    shader->use();

    glm::mat4 model = glm::mat4(1.0f); // No translation, verts are in world space
    shader->setMat4("model", model);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);

    glBindVertexArray(vao);
    glLineWidth(2.0f);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
}

struct SceneObject
{
    std::string name;
    std::string modelPath;
    glm::vec3 position;
    glm::vec3 rotation;
    Mesh *mesh = nullptr;
    GLuint textureID = 0;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    bool printed = false;
};

std::vector<SceneObject> LoadXBlockScene(const std::string &xblockPath, const std::string &modelBasePath,
                                         std::ostream &logStream)
{
    auto PrintProgress = [](size_t current, size_t total)
    {
        const int barWidth = 50;
        float progress = static_cast<float>(current) / total;
        std::cout << "[";
        int pos = static_cast<int>(barWidth * progress);
        for (int i = 0; i < barWidth; ++i)
            std::cout << (i < pos ? "=" : (i == pos ? ">" : " "));
        std::cout << "] " << int(progress * 100.0) << "% (" << current << "/" << total << ")\r";
        std::cout.flush();
    };

    std::vector<SceneObject> sceneObjects;

    // Caching the .nif
    std::unordered_map<std::string, std::string> nifLookup;
    for (const auto &entry : fs::recursive_directory_iterator(modelBasePath))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() != ".nif")
            continue; // Skip .dds and others

        std::string stem = entry.path().stem().string();
        std::string lowered;
        std::transform(stem.begin(), stem.end(), std::back_inserter(lowered), ::tolower);

        nifLookup[lowered] = entry.path().string(); // overwrite is fine
    }

    XMLDocument doc;
    if (doc.LoadFile(xblockPath.c_str()) != XML_SUCCESS)
    {
        std::cerr << "[ERROR] Failed to load XBlock file: " << xblockPath << "\n";
        return sceneObjects;
    }

    XMLElement *game = doc.FirstChildElement("game");
    if (!game)
        return sceneObjects;

    XMLElement *entitySet = game->FirstChildElement("entitySet");
    if (!entitySet)
        return sceneObjects;

    size_t totalEntities = 0;
    for (XMLElement *entity = entitySet->FirstChildElement("entity"); entity; entity = entity->NextSiblingElement("entity"))
        ++totalEntities;

    size_t processed = 0;
    for (XMLElement *entity = entitySet->FirstChildElement("entity"); entity; entity = entity->NextSiblingElement("entity"))
    {
        ++processed;
        PrintProgress(processed, totalEntities);

        const char *modelName = entity->Attribute("modelName");
        const char *name = entity->Attribute("name");
        if (!modelName || !name)
            continue;

        glm::vec3 position(0), rotation(0);

        for (XMLElement *prop = entity->FirstChildElement("property"); prop; prop = prop->NextSiblingElement("property"))
        {
            const char *propName = prop->Attribute("name");
            XMLElement *setElem = prop->FirstChildElement("set");
            if (!propName || !setElem)
                continue;

            const char *value = setElem->Attribute("value");
            if (!value)
                continue;

            if (strcmp(propName, "Position") == 0)
            {
                sscanf(value, "%f, %f, %f", &position.x, &position.y, &position.z);
            }
            else if (strcmp(propName, "Rotation") == 0)
            {
                sscanf(value, "%f, %f, %f", &rotation.x, &rotation.y, &rotation.z);
            }
        }

        // reposition orientation
        position = glm::vec3(position.x, position.z, -position.y);

        std::string cleanName = modelName;
        if (!cleanName.empty() && cleanName.back() == '_')
        {
            cleanName.pop_back();
        }

        std::string lookupName = cleanName;
        std::transform(lookupName.begin(), lookupName.end(), lookupName.begin(), ::tolower);

        auto it = nifLookup.find(lookupName);
        if (it == nifLookup.end())
        {
            std::cerr << "[WARN] Could not find NIF for: " << cleanName << "\n";
            continue;
        }

        std::string fullNifPath = it->second;

        std::ifstream nifFile(fullNifPath, std::ios::binary);
        if (!nifFile)
        {
            std::cerr << "[WARN] Could not open NIF: " << fullNifPath << "\n";
            continue;
        }

        // Check file size before reading
        nifFile.seekg(0, std::ios::end);
        std::streamsize fileSize = nifFile.tellg();
        nifFile.seekg(0, std::ios::beg);

        if (fileSize <= 0)
        {
            std::cerr << "[ERROR] NIF file is zero-length or unreadable: " << fullNifPath << "\n";
            continue;
        }
        else if (fileSize < 64) // Minimum size threshold (adjust as needed)
        {
            std::cerr << "[WARN] NIF file is suspiciously small (" << fileSize << " bytes): " << fullNifPath << "\n";
        }

        std::string buffer((std::istreambuf_iterator<char>(nifFile)), std::istreambuf_iterator<char>());
        if (buffer.empty())
        {
            std::cerr << "[ERROR] NIF file is empty or failed to read: " << fullNifPath << "\n";
            continue;
        }

        // Validate magic header
        const std::string expectedMagic = "Gamebryo File Format";
        if (buffer.size() < expectedMagic.size() ||
            std::string_view(buffer.data(), expectedMagic.size()) != expectedMagic)
        {
            std::cerr << "[ERROR] Invalid NIF magic header: " << fullNifPath << "\n";
            continue;
        }

        NifParser parser;
        // Optional: Validate first 4 bytes if NIF has a known header/magic
        if (buffer.size() < 4 || std::string_view(buffer.data(), 4) != "Game")
        { // Replace "Game" with real NIF magic if known
            std::cerr << "[ERROR] Invalid or unrecognized NIF file format: " << fullNifPath << "\n";
            continue;
        }
        // try-catch block to not die from 1 bad nif
        try
        {
            parser.Parse(std::string_view(buffer.data(), buffer.size()));
        }
        catch (const std::exception &e)
        {
            std::cerr << "[ERROR] Exception while parsing NIF: " << fullNifPath << "\n";
            std::cerr << "Reason: " << e.what() << "\n";
            continue;
        }
        auto &package = parser.Package;

        for (const auto &node : package->Nodes)
        {
            if (node.Mesh && node.Mesh->GetVertices() > 0)
            {
                if (!node.Mesh->GetFormat()->GetAttribute("texcoord"))
                {
                    std::cerr << "[WARN] Skipping mesh without texcoord: " << name << "\n";
                    continue;
                }

                Mesh *mesh = MeshLoader::LoadFromNode(node);

                glm::mat4 modelMatrix = node.Transform ? node.Transform->LocalTransformGLM() : glm::mat4(1.0f);

                std::string textureFileName = fs::path(fullNifPath).stem().string() + ".dds";
                std::string parentFolder = "unknown";
                fs::path nifPath = fs::path(fullNifPath).parent_path();
                if (!nifPath.empty())
                {
                    parentFolder = nifPath.parent_path().filename().string();
                }

                std::string texturePath = "resources/textures/textures/" + parentFolder + "/" + textureFileName;
                std::replace(texturePath.begin(), texturePath.end(), '\\', '/');

                GLuint textureID = 0;

                // First attempt: best guess path
                if (fs::exists(texturePath))
                {
                    textureID = LoadDDSTexture(texturePath);
                    if (textureID == 0)
                        std::cerr << "[WARN] Failed to load texture: " << texturePath << "\n";
                }
                else
                {
                    // Fallback: full recursive scan in textures/textures/
                    bool found = false;
                    for (const auto &entry : fs::recursive_directory_iterator("resources/textures/textures"))
                    {
                        if (!entry.is_regular_file() || entry.path().extension() != ".dds")
                            continue;

                        std::string candidate = entry.path().filename().string();
                        if (candidate == textureFileName)
                        {
                            texturePath = entry.path().string();
                            std::replace(texturePath.begin(), texturePath.end(), '\\', '/');

                            textureID = LoadDDSTexture(texturePath);
                            if (textureID == 0)
                                std::cerr << "[WARN] Failed to load fallback texture: " << texturePath << "\n";
                            else
                                std::cout << "[INFO] Fallback matched texture: " << texturePath << "\n";

                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        std::cerr << "[WARN] No texture found for " << name << ": " << textureFileName << "\n";
                    }
                }

                if (!mesh || mesh->indexCount > 10000 || mesh->indexCount == 0)
                {
                    std::cerr << "[ERROR] Invalid mesh indexCount: "
                              << (mesh ? mesh->indexCount : 0)
                              << " for " << name << " (" << fullNifPath << ")\n";
                    delete mesh;
                    continue;
                }
                SceneObject obj{name, fullNifPath, position, rotation, mesh, textureID};
                obj.modelMatrix = modelMatrix;
                sceneObjects.push_back(obj);
            }
        }
    }

    logStream << "[INFO] Finished loading " << sceneObjects.size() << " SceneObjects from " << xblockPath << "\n";
    return sceneObjects;
}

int main()
{
    if (!glfwInit())
        return -1;

    static std::ofstream logStream("log_output.txt");
    if (!logStream.is_open())
    {
        std::cerr << "Failed to open log file.\n";
    }
    else
    {
        static DualStreamBuf dualOutBuf(std::cout.rdbuf(), logStream.rdbuf());
        static DualStreamBuf dualErrBuf(std::cerr.rdbuf(), logStream.rdbuf());
        std::cout.rdbuf(&dualOutBuf);
        std::cerr.rdbuf(&dualErrBuf);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Map Editor", nullptr, nullptr);
    if (!window)
        return -1;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Camera camera(glm::vec3(0.0f, 2300.0f, 10.0f));
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    shader = new Shader("shaders/vertex.glsl", "shaders/fragment.glsl");

    std::vector<SceneObject> sceneObjects = LoadXBlockScene(
        "resources/map.xblock",
        "resources/textures/", logStream);

    SceneObject *selectedObject = nullptr;
    static GLuint fallbackTex = CreateWhiteTexture();
    bool mouseCaptured = false;
    bool leftMousePressedLastFrame = false;

    while (!glfwWindowShouldClose(window))
    {
        int fbWidth = 0, fbHeight = 0;
        float aspect = 1.0f;
        glm::mat4 view(1.0f);
        glm::mat4 projection(1.0f);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        std::cout << "[DEBUG] deltaTime: " << deltaTime << "\n";

        processInput(window, camera);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int drawCount = 0;
        int maxDrawLog = 10;

        for (SceneObject &obj : sceneObjects)
        {
            if (drawCount < maxDrawLog)
            {
                std::cout << "[DEBUG] Drawing object: " << obj.name << "\n";
                std::cout << "         Texture ID: " << obj.textureID << "\n";
                if (obj.mesh)
                    std::cout << "         Indices: " << obj.mesh->indexCount << "\n";
                else
                    std::cout << "         Mesh is null!\n";
            }
            drawCount++;

            shader->use();

            glm::mat4 model = obj.modelMatrix;

            // Apply object's own rotation *after* adjusting world up-axis
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));

            view = camera.GetViewMatrix();
            glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
            if (fbHeight == 0)
                fbHeight = 1;
            aspect = static_cast<float>(fbWidth) / fbHeight;
            projection = glm::perspective(glm::radians(camera.Zoom), aspect, 1.0f, 10000.0f);

            shader->setMat4("model", model);
            shader->setMat4("view", view);
            shader->setMat4("projection", projection);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, obj.textureID ? obj.textureID : fallbackTex);
            shader->setInt("texture1", 0);

            if (obj.mesh)
            {
                obj.mesh->Draw();
            }

            // 🔲 Draw outline for selected object
            if (selectedObject && selectedObject->mesh)
            {
                view = camera.GetViewMatrix();
                glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
                if (fbHeight == 0)
                    fbHeight = 1;
                aspect = static_cast<float>(fbWidth) / fbHeight;
                projection = glm::perspective(glm::radians(camera.Zoom), aspect, 1.0f, 10000.0f);

                glm::vec3 center = selectedObject->position;
                glm::vec3 halfSize(75.0f); // adjust if needed

                DrawWireCubeModern(center, halfSize, view, projection, shader);
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2(300, 150), ImGuiCond_Once);
            ImGui::SetNextWindowSizeConstraints(ImVec2(300, 100), ImVec2(FLT_MAX, FLT_MAX));

            ImGui::Begin("Debug Info");
            ImGui::Text("FPS: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Camera: (%.1f, %.1f, %.1f)", camera.Position.x, camera.Position.y, camera.Position.z);
            if (selectedObject)
            {
                ImGui::Separator();
                ImGui::Text("Selected:");
                ImGui::Text("Name: %s", selectedObject->name.c_str());
                ImGui::Text("Model: %s", selectedObject->modelPath.c_str());
                ImGui::Text("Pos: (%.1f, %.1f, %.1f)", selectedObject->position.x, selectedObject->position.y, selectedObject->position.z);
            }

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            glfwPollEvents();

            // Ray picking
            ImVec2 mouse = ImGui::GetMousePos();

            glfwGetFramebufferSize(window, &fbWidth, &fbHeight); // use framebuffer size!
            if (fbHeight == 0)
                fbHeight = 1;
            aspect = static_cast<float>(fbWidth) / fbHeight;
            projection = glm::perspective(glm::radians(camera.Zoom), aspect, 1.0f, 10000.0f);
            view = camera.GetViewMatrix();

            float x = (2.0f * mouse.x) / fbWidth - 1.0f;
            float y = 1.0f - (2.0f * mouse.y) / fbHeight;
            glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

            if (fbHeight == 0)
                fbHeight = 1;
            aspect = static_cast<float>(fbWidth) / fbHeight;
            projection = glm::perspective(glm::radians(camera.Zoom), aspect, 1.0f, 10000.0f);
            view = camera.GetViewMatrix();

            glm::vec4 rayEye = glm::inverse(projection) * rayClip;
            rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

            glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

            static bool leftMousePressedLastFrame = false;
            bool leftMousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            if (!mouseCaptured && leftMousePressedNow && !leftMousePressedLastFrame)
            {
                std::cout << "[DEBUG] Mouse click detected, performing ray test...\n";
                std::cout << "[DEBUG] Ray origin: " << camera.Position.x << ", " << camera.Position.y << ", " << camera.Position.z << "\n";
                std::cout << "[DEBUG] Ray dir: " << rayWorld.x << ", " << rayWorld.y << ", " << rayWorld.z << "\n";

                float closestHit = 1e9f;
                selectedObject = nullptr; // <- important: use global one

                for (SceneObject &obj : sceneObjects)
                {
                    glm::vec3 center = obj.position;
                    glm::vec3 halfSize(75.0f); // <- help wtf do i do

                    glm::vec3 min = center - halfSize;
                    glm::vec3 max = center + halfSize;

                    float t;
                    if (RayIntersectsAABB(camera.Position, rayWorld, min, max, t))
                    {
                        if (t > 0.0f && t < closestHit)
                        {
                            closestHit = t;
                            selectedObject = &obj;
                        }
                    }
                }

                if (selectedObject)
                    std::cout << "[DEBUG] Selected object: " << selectedObject->name << "\n";
            }

            leftMousePressedLastFrame = leftMousePressedNow;

            // Camera control toggle
            static bool altHeldLastFrame = false;

            bool altPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;

            if (altPressed && !altHeldLastFrame)
            {
                mouseCaptured = !mouseCaptured;

                if (mouseCaptured)
                {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    camera.EnableRotation = true;
                }
                else
                {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    camera.EnableRotation = false;
                }
            }

            altHeldLastFrame = altPressed;
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwTerminate();

        delete shader;
        return 0;
    }
}
