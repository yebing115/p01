#pragma once

#include <imgui.h>
#include <MathGeoLib.h>

bool imgui_init(void* hwnd);
void imgui_new_frame(const float2& win_size, const float2& mouse_pos,
                     bool mouse_down[3], float mouse_wheel, float dt);
void imgui_shutdown();
