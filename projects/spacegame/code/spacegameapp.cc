//------------------------------------------------------------------------------
// spacegameapp.cc
// (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"
#include <cstring>
#include "imgui.h"
#include "render/renderdevice.h"
#include "render/shaderresource.h"
#include <vector>
#include "render/textureresource.h"
#include "render/model.h"
#include "render/cameramanager.h"
#include "render/lightserver.h"
#include "render/debugrender.h"
#include "core/random.h"
#include "input/inputserver.h"
#include "render/physics.h"
#include <chrono>
#include <flatbuffers/flatbuffers.h>
#include <thread>

#include "proto.h"
#include "spaceship.h"

using namespace Display;
using namespace Render;

namespace Game {
    //------------------------------------------------------------------------------
    /**
    */
    SpaceGameApp::SpaceGameApp() = default;

    //------------------------------------------------------------------------------
    /**
    */
    SpaceGameApp::~SpaceGameApp() = default;

    //------------------------------------------------------------------------------
    /**
    */
    bool
    SpaceGameApp::Open() {
        App::Open();
        this->window = new Display::Window;
        this->window->SetSize(2560, 1440);

        if (this->window->Open()) {
            // set clear color to gray
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

            RenderDevice::Init();
            Net::Initialize();

            // set ui rendering function
            this->window->SetUiRender([this]() {
                this->RenderUI();
            });

            return true;
        }
        return false;
    }

    //------------------------------------------------------------------------------
    /**
    */
    void
    SpaceGameApp::Run() {
        int w;
        int h;
        this->window->GetSize(w, h);
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), float(w) / float(h), 0.01f, 1000.f);
        Camera *cam = CameraManager::GetCamera(CAMERA_MAIN);
        cam->projection = projection;

        // load all resources
        const ModelId models[6] = {
            LoadModel("assets/space/Asteroid_1.glb"),
            LoadModel("assets/space/Asteroid_2.glb"),
            LoadModel("assets/space/Asteroid_3.glb"),
            LoadModel("assets/space/Asteroid_4.glb"),
            LoadModel("assets/space/Asteroid_5.glb"),
            LoadModel("assets/space/Asteroid_6.glb")
        };


        const Physics::ColliderMeshId colliderMeshes[6] = {
            Physics::LoadColliderMesh("assets/space/Asteroid_1_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_2_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_3_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_4_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_5_physics.glb"),
            Physics::LoadColliderMesh("assets/space/Asteroid_6_physics.glb")
        };

        constexpr int numNear = 100;
        constexpr int numFar = 50;
        m_Asteroids.reserve(numNear + numFar);

        // Setup asteroids near
        for (int i = 0; i < numNear; i++) {
            const size_t resourceIndex = Core::FastRandom() % 6;
            constexpr float span = 20.0f;
            vec3 translation(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            vec3 rotationAxis = normalize(translation);
            const float rotation = translation.x;
            mat4 transform = rotate(rotation, rotationAxis) * translate(translation);
            m_Asteroids.emplace_back(models[resourceIndex], transform);

            Server::AddAsteroid(colliderMeshes[resourceIndex], transform);
        }

        // Setup asteroids far
        for (int i = 0; i < numFar; i++) {
            const size_t resourceIndex = Core::FastRandom() % 6;
            constexpr float span = 80.0f;
            const vec3 translation(
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span,
                Core::RandomFloatNTP() * span
            );
            vec3 rotationAxis = normalize(translation);
            const float rotation = translation.x;
            const mat4 transform = rotate(rotation, rotationAxis) * translate(translation);
            m_Asteroids.emplace_back(models[resourceIndex], transform);

            Server::AddAsteroid(colliderMeshes[resourceIndex], transform);
        }

        // Setup skybox
        std::vector<const char *> skybox
        {
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png",
            "assets/space/bg.png"
        };
        TextureResourceId skyboxId = TextureResource::LoadCubemap("skybox", skybox, true);
        RenderDevice::SetSkybox(skyboxId);

        Input::Keyboard *kbd = Input::GetDefaultKeyboard();

        const int numLights = 40;
        Render::PointLightId lights[numLights];
        // Setup lights
        for (int i = 0; i < numLights; i++) {
            glm::vec3 translation = glm::vec3(
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f,
                Core::RandomFloatNTP() * 20.0f
            );
            glm::vec3 color = glm::vec3(
                Core::RandomFloat(),
                Core::RandomFloat(),
                Core::RandomFloat()
            );
            lights[i] = Render::LightServer::CreatePointLight(translation, color, Core::RandomFloat() * 4.0f,
                                                              1.0f + (15 + Core::RandomFloat() * 10.0f));
        }

        const ModelId shipModel = LoadModel("assets/space/spaceship.glb");
        const ModelId laserModel = LoadModel("assets/space/laser.glb");


        SpaceShipCamera camera;

        Client::SetScene(&m_SpaceShips, &m_Lasers);

        KeyMap input;

        float dt = 0.01667f;
        glfwSwapInterval(1);

        bool isOpen = window->IsOpen();

        std::thread serverThread([&isOpen] {
            while (isOpen) {
                Server::Update();
            }
        });
        serverThread.detach();
        uint32 currentFrame = 0;

        // game loop
        while (isOpen) {
            auto timeStart = std::chrono::steady_clock::now();

            //Server::Update();
            //if (currentFrame % 40 == 0)
            Client::Update();

            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            this->window->Update();

            if (kbd->pressed[Input::Key::Code::End]) {
                ShaderResource::ReloadShaders();
            }

            if (input.Set(kbd)) {
                auto fbb = Packet::InputC2S(input.timeSet, input.mask);
                Client::Send(fbb.GetBufferPointer(), fbb.GetSize());
            }


            // Store all drawcalls in the render device
            for (auto const &asteroid: m_Asteroids) {
                RenderDevice::Draw(asteroid.first, asteroid.second);
            }

            for (auto &ship: m_SpaceShips) {
                if (Client::GetId() == ship.first) {
                    camera.target = ship.second.transform.GetMatrix();
                    camera.Update(dt);
                    ship.second.UserUpdate(input, dt);
                } else {
                    ship.second.Interpolate(dt);
                }
                ship.second.Update(dt);
                RenderDevice::Draw(shipModel, ship.second.transform.GetMatrix());
            }

            for (auto &laser: m_Lasers) {
                laser.second.Update(dt);
                RenderDevice::Draw(laserModel, laser.second.transform.GetMatrix());
            }

            // Execute the entire rendering pipeline
            RenderDevice::Render(this->window, dt);

            // transfer new frame to window
            this->window->SwapBuffers();
            currentFrame++;

            auto timeEnd = std::chrono::steady_clock::now();
            dt = std::min(0.04f, std::chrono::duration<float>(timeEnd - timeStart).count());

            if (kbd->pressed[Input::Key::Code::Escape])
                this->Exit();

            isOpen = window->IsOpen();
        }
    }

    //------------------------------------------------------------------------------
    /**
    */
    void
    SpaceGameApp::Exit() {
        this->window->Close();
    }

    //------------------------------------------------------------------------------
    /**
    */
    void
    SpaceGameApp::RenderUI() {
        if (this->window->IsOpen()) {
            ImGui::Begin("Net");

            for (auto &ship: m_SpaceShips) {
                ImGui::DragFloat3("Ship position", &ship.second.transform.GetPosition()[0]);
                ImGui::DragFloat4("Ship orientation", &ship.second.transform.GetOrientation()[0]);
            }

            static bool isConnected = false;
            if (m_IsHost || isConnected) {
                ImGui::Text("%s", Client::Status());
                if (ImGui::Button("Disconnect")) {
                    Client::Disconnect();
                }
            } else {
                static uint16 port = 6969;
                ImGui::InputInt("Port", (int *) &port);
                if (ImGui::Button("Host")) {
                    Server::Create(port);
                    Client::Connect("127.0.0.1", port);
                    m_IsHost = true;
                }
                static char ip[16] = {"127.0.0.1"};
                ImGui::InputText("IP", ip, 15);
                if (ImGui::Button("Connect")) {
                    Client::Connect(ip, port);
                    isConnected = true;
                }
            }

            ImGui::End();

            Debug::DispatchDebugTextDrawing();
        }
    }
} // namespace Game
