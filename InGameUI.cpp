﻿#include "stdafx.h"
#include "d3d9.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_internal.h"

std::vector<void(*)()> content, windows, init;
void onLostDevice() { ImGui_ImplDX9_InvalidateDeviceObjects(); }
void onResetDevice() { ImGui_ImplDX9_CreateDeviceObjects(); }
void onCreateDevice(LPDIRECT3DDEVICE9 pD3DDevice)
{
	ImGui_ImplDX9_Init(pD3DDevice);
	ImGui::GetIO().IniFilename = nullptr;
	ImGui::GetStyle().WindowTitleAlign = ImGuiAlign_Center;
	ImGui::GetStyle().WindowFillAlphaDefault = 0.95f;
	for (size_t i = 0; i < init.size(); i++)
		init[i]();
}

void onEndScene()
{
	ImGui_ImplDX9_NewFrame();
	for (size_t i = 0; i < windows.size(); i++)
		windows[i]();
	ImGui::Render();
}

bool inGameUIEnabled = false;
void renderDDDAFixUI()
{
	if (!inGameUIEnabled)
		return;

	static char titleBuffer[64];
	sprintf_s(titleBuffer, "DDDAFix - %.1f FPS - %d###DDDAFix", ImGui::GetIO().Framerate, ImGui::GetIO().Fonts->Fonts.size());
	ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiSetCond_Once);
	if (ImGui::Begin(titleBuffer, nullptr, ImVec2(450, 600)))
	{
		for (size_t i = 0; i < content.size(); i++)
		{
			ImGui::PushID(i);
			content[i]();
			ImGui::PopID();
		}
	}
	ImGui::End();
}

LPBYTE pInGameUI, oInGameUI;
SHORT WINAPI HGetAsyncKeyState(int vKey) { return ImGui::IsAnyItemActive() ? 0 : GetAsyncKeyState(vKey); }
void __declspec(naked) HInGameUI()
{
	__asm	mov		ebp, HGetAsyncKeyState;
	__asm	jmp		oInGameUI;
}

WPARAM inGameUIHotkey;
bool Hooks::InGameUI()
{
	if (!config.getBool("inGameUI", "enabled", false))
	{
		logFile << "InGameUI: disabled" << std::endl;
		return false;
	}

	BYTE sigRun[] = { 0x8B, 0x2D, 0xCC, 0xCC, 0xCC, 0xCC,	//mov	ebp, ds:GetAsyncKeyState
					0x8D, 0x7E, 0x01 };						//lea	edi, [esi+1]
	if (FindSignature("InGameUI", sigRun, &pInGameUI))
	{
		CreateHook("InGameUI", pInGameUI, &HInGameUI, &oInGameUI, inGameUIEnabled);
		oInGameUI += 6;
	}

	inGameUIHotkey = config.getUInt("hotkeys", "keyUI", VK_F12) & 0xFF;
	InGameUIAddWindow(renderDDDAFixUI);
	D3D9(onCreateDevice, onLostDevice, onResetDevice, onEndScene);
	return true;
}

void Hooks::InGameUIAdd(void(*callback)()) { content.push_back(callback); }
void Hooks::InGameUIAddWindow(void(*callback)()) { windows.push_back(callback); }
void Hooks::InGameUIAddInit(void(*callback)()) { init.push_back(callback); }
LRESULT Hooks::InGameUIEvent(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_KEYDOWN && (HIWORD(lParam) & KF_REPEAT) == 0 && wParam == inGameUIHotkey)
		SwitchHook("InGameUI", pInGameUI, inGameUIEnabled = !inGameUIEnabled);
	return inGameUIEnabled ? ImGui_ImplDX9_WndProcHandler(hwnd, msg, wParam, lParam) : 0;
}

namespace ImGui
{
	bool InputFloatN(const char* label, float* v, int count, float item_width, float min, float max, int precision)
	{
		if (item_width > 0.0f)
			PushItemWidth(item_width * count);
		bool changed = InputFloatN(label, v, count, precision, 0);
		if (item_width > 0.0f)
			PopItemWidth();
		if (changed)
			for (int i = 0; i < count; i++)
			{
				if (v[i] < min)
					v[i] = min;
				if (v[i] > max)
					v[i] = max;
			}
		return changed;
	}

	bool InputFloatEx(const char* label, float* v, float step, float min, float max, int precision)
	{
		if (!InputFloat(label, v, step, 0.0f, precision, 0))
			return false;
		if (*v < min)
			*v = min;
		if (*v > max)
			*v = max;
		return true;
	}

	void TextUnformatted(const char* label, float pos)
	{
		SameLine(pos);
		TextUnformatted(label);
	}
}