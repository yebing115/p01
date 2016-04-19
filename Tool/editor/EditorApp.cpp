#include "EditorApp.h"
#include "AppConfig.h"
#include "C3PCH.h"
#include "Graphics/RenderSystem.h"

EditorApp::EditorApp() {
  auto world = GameWorld::CreateInstance();
  auto renderer = new RenderSystem;
  world->AddSystem(renderer);

  _debug_camera = world->CreateEntity();
  world->CreateCamera(_debug_camera);
  world->SetCameraVerticalFovAndAspectRatio(_debug_camera, DegToRad(80.f), GraphicsRenderer::Instance()->GetWindowAspect());
  world->SetCameraClipPlane(_debug_camera, 1.f, 3000.f);
  world->SetCameraPos(_debug_camera, vec(-180, 80, 70));
  world->SetCameraFront(_debug_camera, vec::unitX);

  _model = world->CreateEntity();
  world->CreateTransform(_model);
  world->CreateComponent(_model, MODEL_RENDERER_COMPONENT);
  renderer->SetModelFilename(_model, "Models/Sponza/sponza.mex");
}

void EditorApp::OnUpdate(float dt, bool paused) {
  (void)dt;
  (void)paused;
  GameWorld::Instance()->Update(dt, paused);

  UpdateDebugCamera(dt);
}

void EditorApp::OnRender(float dt, bool paused) {
  (void)dt;
  (void)paused;

  GameWorld::Instance()->Render(dt, paused);
  return;

  auto GR = GraphicsRenderer::Instance();
  auto view = GR->PushView();
  auto win_size = GR->GetWindowSize();
  GR->SetViewRect(view, 0, 0, win_size.x, win_size.y);
  GR->SetViewClear(view, C3_CLEAR_COLOR | C3_CLEAR_DEPTH, 0x808080ff, 1.f);
  GR->Touch(view);
  GR->PopView();
}

void EditorApp::UpdateDebugCamera(float dt) {
  ImGui::Begin("Camera");
  auto world = GameWorld::Instance();
  auto IM = InputManager::Instance();
  auto win_size = GraphicsRenderer::Instance()->GetWindowSize();
  if (!ImGui::GetIO().WantCaptureKeyboard) {
  }
  if (!ImGui::GetIO().WantCaptureMouse) {
    float wheel = IM->GetMouseWheel();
    if (wheel > 0.f) world->ZoomCamera(_debug_camera, 1.5f);
    else if (wheel < 0.f) world->ZoomCamera(_debug_camera, 1.f / 1.5f);
    if (IM->IsMouseDragging(MIDDLE_BUTTON)) {
      if (IM->IsKeyShiftDown()) {
        auto delta = IM->GetMouseDelta();
        world->PanCamera(_debug_camera, -delta);
      } else if (IM->IsKeyAltDown()) {
        float2 p0 = IM->GetMousePosPrev() - win_size * 0.5f;
        float2 p1 = IM->GetMousePos() - win_size * 0.5f;
        if (!p0.IsZero() && !p1.IsZero() && !p0.Equals(p1)) {
          float angle = Atan2(p1.y, p1.x) - Atan2(p0.y, p0.x);
          Quat q(world->GetCameraFront(_debug_camera), angle);
          world->TransformCamera(_debug_camera, q);
        }
      } else if (!IM->IsKeyCtrlDown()) {
        auto delta = IM->GetMouseDelta() / win_size;
        Quat q0(world->GetCameraUp(_debug_camera), -delta.x * pi);
        Quat q1(world->GetCameraRight(_debug_camera), -delta.y * pi);
        world->TransformCamera(_debug_camera, q0 * q1);
      }
    }
  }
  ImGui::Text("camera pos: %s\n", world->GetCameraPos(_debug_camera).ToString().c_str());
  ImGui::Text("camera v_fov: %.3f\n", RadToDeg(world->GetCameraVerticalFov(_debug_camera)));
  ImGui::End();
}