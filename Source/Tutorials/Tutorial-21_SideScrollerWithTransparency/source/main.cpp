// TGP.cpp : Defines the entry point for the application.
//

#include <fstream>
#include <iomanip>
#include <tge/animation/AnimationPlayer.h>
#include <tge/drawers/CustomShapeDrawer.h>
#include <tge/drawers/LineDrawer.h>
#include <tge/graphics/AmbientLight.h>
#include <tge/graphics/DirectionalLight.h>
#include <tge/graphics/PointLight.h>
#include <tge/input/InputManager.h>
#include <tge/error/ErrorManager.h>
#include <tge/graphics/Camera.h>
#include <tge/graphics/DX11.h>
#include <tge/drawers/ModelDrawer.h>
#include <tge/graphics/FullscreenEffect.h>
#include <tge/graphics/GraphicsEngine.h>
#include <tge/graphics/RenderTarget.h>
#include <tge/Model/ModelFactory.h>
#include <tge/Model/ModelInstance.h>
#include <tge/graphics/GraphicsStateStack.h>
#include <tge/primitives/CustomShape.h>
#include <tge/primitives/LinePrimitive.h>
#include <tge/sprite/sprite.h>
#include <tge/drawers/SpriteDrawer.h>
#include <tge/texture/TextureManager.h>
#include <tge/Timer.h>
#include <tge/settings/settings.h>
#include <tge/text/text.h>
#include <tge/text/TextService.h>

#pragma comment (lib, "d3d11.lib")

using namespace Tga;

float camSpeed = 1000.f;
float camRotSpeed = 1.f;

enum class PlayerState
{
    Idle,
    Walking,
    Jumping
};

enum class PlayerDirection
{
    Left,
    Right,
};

struct PlayerData
{
    PlayerState state;
    PlayerDirection direction;
    Vector3f playerPosition;
};

void Go(void);
int main(const int /*argc*/, const char* /*argc*/[])
{
    Go();
    return 0;
}

Tga::InputManager* SInputManager;

void Go(void)
{
    Tga::LoadSettings(TGE_PROJECT_SETTINGS_FILE);
	Tga::EngineConfiguration& cfg = Tga::Settings::GetEngineConfiguration();

    cfg.myWinProcCallback = [](HWND, UINT message, WPARAM wParam, LPARAM lParam)
    {
        SInputManager->UpdateEvents(message, wParam, lParam);
        return 0;
    };

    cfg.myApplicationName = L"TGA 2D Tutorial 21";
    cfg.myActivateDebugSystems = Tga::DebugFeature::Fps |
        Tga::DebugFeature::Mem |
        Tga::DebugFeature::Drawcalls |
        Tga::DebugFeature::Cpu |
        Tga::DebugFeature::Filewatcher |
        Tga::DebugFeature::OptimizeWarnings;

    if (!Tga::Engine::Start())
    {
        ERROR_PRINT("Fatal error! Engine could not start!");
        system("pause");
        return;
    }

    {
        bool bShouldRun = true;

        HWND windowHandle = *Tga::Engine::GetInstance()->GetHWND();

        Tga::InputManager myInputManager(windowHandle);
        SInputManager = &myInputManager;
                
        MSG msg = { 0 };

        Timer timer;

        Tga::Vector2ui intResolution = Tga::Engine::GetInstance()->GetRenderSize();
        Tga::Vector2f resolution = { (float)intResolution.x, (float)intResolution.y };

        Tga::Camera sideScrollingCamera;
        {
            sideScrollingCamera.SetPerspectiveProjection(
                90,
                {
                    resolution.x,
                    resolution.y
                },
                0.1f,
                50000.0f)
                ;

            sideScrollingCamera.GetTransform().Rotate(Rotator(10, 0, 0));
        }

        Tga::Camera freeFlyCamera;
        Rotator freeFlyCameraRotation = Rotator(30, 0, 0);
        {
            freeFlyCamera.SetPerspectiveProjection(
                90,
                {
                    resolution.x,
                    resolution.y
                },
                0.1f,
                50000.0f)
                ;


            freeFlyCamera.GetTransform().SetRotation(freeFlyCameraRotation);
        }

        bool isFreeFly = false;
        bool isCursorHidden = false;
        Vector2i capturedCameraPos;

        ModelFactory& modelFactory = ModelFactory::GetInstance();

        ModelInstance platformModel = modelFactory.GetModelInstance("environment/platform_2.fbx");
        platformModel.SetTexture(0, 0, Tga::Engine::GetInstance()->GetTextureManager().GetTexture("environment/gradientMap_c.tga"));
        platformModel.GetTransform().Scale({ 2.f, 2.f, 2.f });

        AnimatedModelInstance characterModel = modelFactory.GetAnimatedModelInstance("character/popp_sk.fbx");
        characterModel.SetTexture(0, 0, Tga::Engine::GetInstance()->GetTextureManager().GetTexture("character/atlas.tga"));

        ModelInstance cube = modelFactory.GetUnitCube();
        cube.SetTexture(0, 0, Tga::Engine::GetInstance()->GetTextureManager().GetTexture("cubetexture.tga"));

        AnimationPlayer idleAnimation = modelFactory.GetAnimationPlayer("character/idle.fbx", characterModel.GetModel());
        AnimationPlayer runAnimation = modelFactory.GetAnimationPlayer("character/run.fbx", characterModel.GetModel());
        AnimationPlayer jumpAnimation = modelFactory.GetAnimationPlayer("character/jump.fbx", characterModel.GetModel());

        PlayerData playerData;
        Vector3f smoothCameraOffset;

        idleAnimation.SetIsLooping(true);
        runAnimation.SetIsLooping(true);

        idleAnimation.Play();
        runAnimation.Play();


        Tga::SpriteSharedData tgeSpriteSharedData;
        tgeSpriteSharedData.myTexture = Tga::Engine::GetInstance()->GetTextureManager().GetTexture("sprites/tge_logo_w.dds");

        std::vector<Tga::Sprite3DInstanceData> tgeSpriteInstances;
        for (unsigned int i = 0; i < 20000; i++)
        {
            float randomX = static_cast<float>(rand() % 1000) / 1000.0f;
            float randomY = static_cast<float>(rand() % 1000) / 1000.0f;
            float randomZ = static_cast<float>(rand() % 1000) / 1000.0f;

            Tga::Sprite3DInstanceData instance = {};
            instance.myTransform = Matrix4x4f::CreateFromScale(200.f);
            instance.myTransform.SetPosition(Tga::Vector3f(-10000.f + 20000.f*randomX, -5000.f  + 10000.f * randomY, 100 + 9900.f * randomZ));
            instance.myColor = Tga::Color(0.2f,0.3f,0.6f, 1);
            tgeSpriteInstances.push_back(instance);
        }

        Tga::SpriteSharedData fogPlaneSpriteSharedData;
        fogPlaneSpriteSharedData.myTexture = Tga::Engine::GetInstance()->GetTextureManager().GetWhiteSquareTexture();

        Tga::SpriteSharedData effectSpriteSharedData;
        effectSpriteSharedData.myTexture = Tga::Engine::GetInstance()->GetTextureManager().GetTexture("sprites/Particle.dds");

        Tga::SpriteSharedData cloudSpriteSharedData;
        cloudSpriteSharedData.myTexture = Tga::Engine::GetInstance()->GetTextureManager().GetTexture("sprites/cloud.dds");

        Tga::Font font48 = Tga::Engine::GetInstance()->GetTextService().GetOrLoad("Text/arial.ttf", Tga::FontSize_48);
        Tga::Font font18 = Tga::Engine::GetInstance()->GetTextService().GetOrLoad("Text/arial.ttf", Tga::FontSize_18);

        while (bShouldRun)
        {

            timer.Update();
            myInputManager.Update();


            if (myInputManager.IsKeyPressed(VK_ESCAPE))
            {
                PostQuitMessage(0);
            }


            {
                // Toggle free fly on and off with TAB
                if (myInputManager.IsKeyPressed(VK_TAB))
                {
                    if (!isFreeFly)
                    {
                        freeFlyCamera = sideScrollingCamera;
                        isFreeFly = true;
                    }
                    else
                    {
                        isFreeFly = false;
                    }
                }

                // Handle freefly camera
                if (isFreeFly && GetForegroundWindow() == windowHandle)
                {
                    if (!isCursorHidden)
                    {
                        POINT cursorPos;
                        GetCursorPos(&cursorPos);

                        capturedCameraPos.x = cursorPos.x;
                        capturedCameraPos.y = cursorPos.y;

                        myInputManager.HideMouse();
                        myInputManager.CaptureMouse();

                        isCursorHidden = true;
                    }

                    SetCursorPos(capturedCameraPos.x, capturedCameraPos.y);

                    Vector3f camMovement;
                    Vector3f camRotation;

                    if (myInputManager.IsKeyHeld(0x57)) // W
                    {
                        camMovement += freeFlyCamera.GetTransform().GetForward() * 1.0f;
                    }
                    if (myInputManager.IsKeyHeld(0x53)) // S
                    {
                        camMovement += freeFlyCamera.GetTransform().GetForward() * -1.0f;
                    }
                    if (myInputManager.IsKeyHeld(0x41)) // A
                    {
                        camMovement += freeFlyCamera.GetTransform().GetRight() * -1.0f;
                    }
                    if (myInputManager.IsKeyHeld(0x44)) // D
                    {
                        camMovement += freeFlyCamera.GetTransform().GetRight() * 1.0f;
                    }

                    freeFlyCamera.GetTransform().SetPosition(freeFlyCamera.GetTransform().GetPosition() + camMovement * camSpeed * timer.GetDeltaTime());

                    const Vector2f mouseDelta = myInputManager.GetMouseDelta();

                    camRotation.X = mouseDelta.Y;
                    camRotation.Y = mouseDelta.X;

                    freeFlyCameraRotation += camRotation * camRotSpeed;

                    freeFlyCamera.GetTransform().SetRotation(freeFlyCameraRotation);

                    if (myInputManager.IsKeyPressed(VK_SHIFT))
                    {
                        // When we hold shift, "sprint".
                        camSpeed *= 4;
                    }

                    if (myInputManager.IsKeyReleased(VK_SHIFT))
                    {
                        // When we let go, "walk".
                        camSpeed /= 4;
                    }
                }
                else
                {
                    if (isCursorHidden)
                    {
                        myInputManager.ShowMouse();
                        myInputManager.ReleaseMouse();

                        isCursorHidden = false;
                    }
                }


            }

            // Handle character movement
            {
                bool isTryingToJump = false;
                bool isTryingToMoveLeft = false;
                bool isTryingToMoveRight = false;

                if (!isFreeFly)
                {
                    if (myInputManager.IsKeyHeld('W'))
                        isTryingToJump = true;
                    if (myInputManager.IsKeyHeld('A'))
                        isTryingToMoveLeft = true;
                    if (myInputManager.IsKeyHeld('D'))
                        isTryingToMoveRight = true;

                    if (isTryingToMoveLeft && isTryingToMoveRight)
                    {
                        isTryingToMoveLeft = false;
                        isTryingToMoveRight = false;
                    }
                }

                {
                    if (isTryingToJump)
                    {
                        if (jumpAnimation.GetState() != AnimationState::Playing)
                        {
                            jumpAnimation.SetFrame(5);  // skip beginning of jump animation
                            jumpAnimation.Play();
                        }
                    }

                    if (jumpAnimation.GetFrame() > 17) // skip end of jump animation
                        jumpAnimation.Stop();

                    bool isMoving = false;


                    if (isTryingToMoveLeft)
                    {
                        isMoving = true;

                        playerData.direction = PlayerDirection::Left;
                        playerData.playerPosition += timer.GetDeltaTime()*Vector3f(-300.f,0.f,0.f);
                    }
                    else if (isTryingToMoveRight)
                    {
                        isMoving = true;

                        playerData.direction = PlayerDirection::Right;
                        playerData.playerPosition += timer.GetDeltaTime() * Vector3f(300.f, 0.f, 0.f);

                    }

                    if (jumpAnimation.GetState() == AnimationState::Playing)
                    {
                        playerData.state = PlayerState::Jumping;
                    }
                    else
                    {
                        if (playerData.state != PlayerState::Walking && isMoving)
                        {
                            runAnimation.SetFrame(20); // start run at frame 20, since that alignes animations the best;
                            playerData.state = PlayerState::Walking;
                        }
                        if (playerData.state != PlayerState::Idle && !isMoving)
                        {
                            idleAnimation.SetFrame(0);
                            playerData.state = PlayerState::Idle;
                        }

                    }

                    Vector3f cameraOffset;
                    if (playerData.direction == PlayerDirection::Left)
                    {
                        characterModel.GetTransform().ResetScaleAndRotation();
                        characterModel.GetTransform().Scale(Vector3f(50.f, 50.f, -50.f));
                        cameraOffset = Vector3f(-300.f, 0.f, 0.f);
                    }
                    else
                    {
                        characterModel.GetTransform().ResetScaleAndRotation();
                        characterModel.GetTransform().Scale(Vector3f(-50.f, 50.f, -50.f));
                        cameraOffset = Vector3f(300.f, 0.f, 0.f);
                    }

                    if (playerData.state == PlayerState::Jumping)
                        cameraOffset += Vector3f(0.f, 200.f, 0.f);

                    characterModel.GetTransform().SetPosition(playerData.playerPosition);

                    // smooth changes to camera offset, to make it look nicer:
                    float secondsToMoveHalfDistance = 0.25f;
                    float k = powf(0.5f, timer.GetDeltaTime()/secondsToMoveHalfDistance);
                    smoothCameraOffset = smoothCameraOffset + (1 - k) * (cameraOffset - smoothCameraOffset);

                    sideScrollingCamera.GetTransform().SetPosition(playerData.playerPosition + smoothCameraOffset + Vector3f(0.0f, 180.f, -500.0f));
                }

                // update animations and set animation based on character state
                {
                    idleAnimation.Update(timer.GetDeltaTime());
                    runAnimation.Update(timer.GetDeltaTime());
                    jumpAnimation.Update(timer.GetDeltaTime());

                    switch (playerData.state)
                    {
                    case PlayerState::Idle:
                        characterModel.SetPose(idleAnimation);
                        break;
                    case PlayerState::Walking:
                        characterModel.SetPose(runAnimation);
                        break;
                    case PlayerState::Jumping:
                        characterModel.SetPose(jumpAnimation);
                        break;
                    default:
                        break;
                    }    
                }
            }

            if (!Tga::Engine::GetInstance()->BeginFrame())
            {
                break;
            }

            DX11::BackBuffer->SetAsActiveTarget(DX11::DepthBuffer); // use depth buffer for 3D rendering

            GraphicsEngine& graphicsEngine = Tga::Engine::GetInstance()->GetGraphicsEngine();
            GraphicsStateStack& graphicsStateStack = Tga::Engine::GetInstance()->GetGraphicsEngine().GetGraphicsStateStack();

            // First draw everything using the 3d camera
            {
                graphicsStateStack.Push();

                if (isFreeFly)
                    graphicsStateStack.SetCamera(freeFlyCamera);
                else
                    graphicsStateStack.SetCamera(sideScrollingCamera);

                // first draw opaque meshes and sprites
                {
                    graphicsStateStack.Push();

                    // Set up to use alpha channel for clipping not blending. This way pixels where the alpha channel is > 0.5 are drawn and below hidden.
                    graphicsStateStack.SetBlendState(BlendState::Disabled);
                    graphicsStateStack.SetAlphaTestThreshold(0.5f);

                    for (int i = -10; i < 10; i++)
                    {
                        platformModel.GetTransform().SetPosition({ (float)i * 400.f, -200, 0 });
                        graphicsEngine.GetModelDrawer().Draw(platformModel);
                    }

                    graphicsEngine.GetModelDrawer().Draw(characterModel);

                    // draw tge sprites in background. These sprites have an alpha channel that controls where the text is drawn/hidden

                    SpriteDrawer& spriteDrawer = graphicsEngine.GetSpriteDrawer();

                    spriteDrawer.Draw(tgeSpriteSharedData, &tgeSpriteInstances[0], tgeSpriteInstances.size());

                    {
                        // draw sprites in three ways:
                        // this one has varying AlphaTestThreshold values. This animates how much of the sprites are shown

                        graphicsStateStack.Push();


                        float t = 3.f * (float)timer.GetTotalTime();
                        graphicsStateStack.SetAlphaTestThreshold(0.5f + 0.45f * sinf(t));

                        Vector3f position = { 1600 + 200, 100, -50 };

                        float size = 200.f;
                        Tga::Sprite3DInstanceData instance = {};
                        instance.myTransform = Matrix4x4f::CreateFromScale(size);
                        instance.myTransform.SetPosition(position + Vector3f{ -0.5f * size, 0.5f * size,0.f});
                        instance.myColor = { 0.7f, 0.4f,0.1f,1.0f };
                        spriteDrawer.Draw(cloudSpriteSharedData, instance);


                        graphicsStateStack.Pop();
                    }

                    // Draw all the information texts
                    {
                        graphicsStateStack.Push();

                        graphicsStateStack.Translate({ -1600.f, 100.f, 50.f });
                        graphicsStateStack.Scale(0.25f);

                        Tga::Text meshText(font48);
                        meshText.SetText(
                            "Meshes can also use blending. Here is a mesh drawn with additative blending \n\n"
                            "To make it appear glowing, a couple of sprites are used.");
                        meshText.SetPosition({ -0.5f * meshText.GetWidth(), 0.f });
                        meshText.Render();

                        graphicsStateStack.Pop();
                    }

                    {
                        graphicsStateStack.Push();

                        graphicsStateStack.Translate({ -800.f, 100.f, 50.f });
                        graphicsStateStack.Scale(0.25f);


                        Tga::Text firefliesText(font48);
                        firefliesText.SetText(
                            "Here is a simple particle effect. Each firefly is drawn with two sprites \n\n"
                            "One large and red, and one small and white. \n\n"
                            "To make the fireflies appear to glow, the blend mode is set to additative");
                        firefliesText.SetPosition({ -0.5f * firefliesText.GetWidth(), 0.f });
                        firefliesText.Render();

                        graphicsStateStack.Pop();
                    }

                    {
                        graphicsStateStack.Push();

                        graphicsStateStack.Translate({ 0.f, 250.f, 50.f });
                        graphicsStateStack.Scale(0.25f);

                        Tga::Text startText(font48);
                        startText.SetText(
                            "Use WASD to move. \n\n"
                            "Explore left and right to find examples and tips");
                        startText.SetPosition({ -0.5f * startText.GetWidth(), 0.f });
                        startText.Render();

                        graphicsStateStack.Pop();
                    }

                    {
                        graphicsStateStack.Push();

                        graphicsStateStack.Translate({ 800.f, 250.f, 50.f });
                        graphicsStateStack.Scale(0.25f);

                        Tga::Text debugCameraText(font48);
                        debugCameraText.SetText(
                            "The background is drawn using alpha tested sprites and semi transparent planes\n\n"
                            "With Tab you can toggle between controlling the character and a freefly debug camera\n\n"
                            "Use this to see how the examples are constructed");
                        debugCameraText.SetPosition({ -0.5f * debugCameraText.GetWidth(), 0.f });
                        debugCameraText.Render();

                        graphicsStateStack.Pop();
                    }


                    {
                        graphicsStateStack.Push();

                        graphicsStateStack.Translate({ 1600.f, 300.f, 50.f });
                        graphicsStateStack.Scale(0.25f);

                        Tga::Text blendModesText(font48);
                        blendModesText.SetText(
                            "One sprite is drawn in three different ways:\n\n"
                            " 1. Alpha blending\n\n"
                            " 2. Additative blending\n\n"
                            " 3. No blending, but varying alpha test threshold");
                        blendModesText.SetPosition({ -0.5f * blendModesText.GetWidth(), 0.f });
                        blendModesText.Render();

                        graphicsStateStack.Pop();
                    }

                    graphicsStateStack.Pop();

                }

                // after this, draw transparent object, which has to be ordered back to front
                {
                    graphicsStateStack.Push();

                    graphicsStateStack.SetBlendState(BlendState::AlphaBlend);
                    graphicsStateStack.SetDepthStencilState(DepthStencilState::WriteLessOrEqual);

                    // fog effect using semi-transparent planes

                    SpriteDrawer& spriteDrawer = graphicsEngine.GetSpriteDrawer();

                    for (int i = 9; i >= 0; i--)
                    {
                        Tga::Sprite3DInstanceData instance = {};
                        instance.myTransform = Matrix4x4f::CreateFromScale(20000.f);
                        instance.myTransform.SetPosition({ -10000, 10000, 100 + (float)i*1000 });
                        instance.myColor = { 0.1f,0.2f,0.5f,0.2f };
                        spriteDrawer.Draw(fogPlaneSpriteSharedData, instance);
                    }

                    // Effect example, fireflies flying in an eight 
                    // using additative blending
                    {
                        {
                            graphicsStateStack.Push();

                            graphicsStateStack.SetBlendState(BlendState::AdditiveBlend);

                            {
                                SpriteBatchScope scope = spriteDrawer.BeginBatch(effectSpriteSharedData);
                                for (int i = 0; i < 5; i++)
                                {
                                    float t = (float)timer.GetTotalTime() + 2.f * 3.1415f * (float)i / 5.f;

                                    Vector3f position = { -800 + 150.f * cosf(t), 200 + 50.f * sinf(2.f * (float)t), 50 * sinf(t) };

                                    {
                                        float size = 100.f;
                                        Tga::Sprite3DInstanceData instance = {};
                                        instance.myTransform = Matrix4x4f::CreateFromScale(size);
                                        instance.myTransform.SetPosition(position + Vector3f{-0.5f*size, 0.5f*size, 0.f});
                                        instance.myColor = { 0.9f,0.3f,0.3f,1.0f };

                                        scope.Draw(instance);
                                    }

                                    {
                                        float size = 10.f;

                                        Tga::Sprite3DInstanceData instance = {};
                                        instance.myTransform = Matrix4x4f::CreateFromScale(10.f);
                                        instance.myTransform.SetPosition(position + Vector3f{ -0.5f * size, 0.5f * size, 0.f });
                                        instance.myColor = { 1.0f,1.0f,1.0f,1.0f };

                                        scope.Draw(instance);
                                    }
                                }
                            }

                            graphicsStateStack.Pop();
                        }
                    }

                    // Mesh with alpha blending example
                    {
                        cube.GetTransform().SetPosition({ -1600, 200.f, 100.f });
                        cube.GetTransform().Rotate(Vector3f{ 45.f,45.f,0.f }*timer.GetDeltaTime());
                        graphicsEngine.GetModelDrawer().Draw(cube);

                        // glow effect using additative blending

                        {
                            graphicsStateStack.Push();

                            graphicsStateStack.SetBlendState(BlendState::AdditiveBlend);

                            {
                                float size = 800.f;

                                Tga::Sprite3DInstanceData instance = {};
                                instance.myTransform = Matrix4x4f::CreateFromScale(size);
                                instance.myTransform.SetPosition(cube.GetTransform().GetPosition() + Vector3f{ -0.5f * size, 0.5f * size, 0.f });
                                instance.myColor = { 1.0f,0.5f,0.2f,0.5f };

                                spriteDrawer.Draw(effectSpriteSharedData, instance);
                            }


                            {
                                float size = 500.f;

                                Tga::Sprite3DInstanceData instance = {};
                                instance.myTransform = Matrix4x4f::CreateFromScale(size);
                                instance.myTransform.SetPosition(cube.GetTransform().GetPosition() + Vector3f{ -0.5f * size, 0.5f * size, 0.f });
                                instance.myColor = { 0.75f,1.0f,0.5f,0.5f };

                                spriteDrawer.Draw(effectSpriteSharedData, instance);
                            }

                            graphicsStateStack.Pop();
                        }
                    }

                    // draw sprites in three ways: alpha blended sprite
                    {
                        graphicsStateStack.Push();
                        graphicsStateStack.SetBlendState(BlendState::AlphaBlend);

                        Vector3f position = { 1600 - 200, 100, -50 };
                        float size = 200.f;
                        Tga::Sprite3DInstanceData instance = {};
                        instance.myTransform = Matrix4x4f::CreateFromScale(size);
                        instance.myTransform.SetPosition(position + Vector3f{ -0.5f * size, 0.5f * size,0.f });
                        instance.myColor = { 0.7f, 0.4f,0.1f,1.0f };
                        spriteDrawer.Draw(cloudSpriteSharedData, instance);


                        graphicsStateStack.Pop();
                    }

                    // draw sprites in three ways: additative blended sprite
                    {
                        graphicsStateStack.Push();
                        graphicsStateStack.SetBlendState(BlendState::AdditiveBlend);

                        Vector3f position = { 1600 + 0, 100, -50 };
                        float size = 200.f;
                        Tga::Sprite3DInstanceData instance = {};
                        instance.myTransform = Matrix4x4f::CreateFromScale(size);
                        instance.myTransform.SetPosition(position + Vector3f{ -0.5f * size, 0.5f * size,0.f });
                        instance.myColor = { 0.7f, 0.4f,0.1f,1.0f };
                        spriteDrawer.Draw(cloudSpriteSharedData, instance);


                        graphicsStateStack.Pop();
                    }

                    graphicsStateStack.Pop();

                }

                graphicsStateStack.Pop(); // this switches back to the default camera
            }

            // last, draw UI on top, with default camera 

            DX11::BackBuffer->SetAsActiveTarget();  // do not use depth buffer for UI rendering
            
            {
                Tga::Text uiText(font18);
                uiText.SetText(
                    "This text is not drawn in 3D. It is locked to the screen");
                uiText.SetPosition({ 10.f, 10.f });
                uiText.Render();
            }

            Tga::Engine::GetInstance()->EndFrame();

        }
    }

    Tga::Engine::GetInstance()->Shutdown();

    return;
}
