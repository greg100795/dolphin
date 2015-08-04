// Copyright 2015 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "InputCommon/InputConfig.h"

enum Hotkey
{
	HK_OPEN,
	HK_CHANGE_DISC,
	HK_REFRESH_LIST,

	HK_PLAY_PAUSE,
	HK_STOP,
	HK_RESET,
	HK_FRAME_ADVANCE,

	HK_START_RECORDING,
	HK_PLAY_RECORDING,
	HK_EXPORT_RECORDING,
	HK_READ_ONLY_MODE,

	HK_FULLSCREEN,
	HK_SCREENSHOT,
	HK_EXIT,

	HK_WIIMOTE1_CONNECT,
	HK_WIIMOTE2_CONNECT,
	HK_WIIMOTE3_CONNECT,
	HK_WIIMOTE4_CONNECT,
	HK_BALANCEBOARD_CONNECT,

	HK_VOLUME_DOWN,
	HK_VOLUME_UP,
	HK_VOLUME_TOGGLE_MUTE,

	HK_INCREASE_IR,
	HK_DECREASE_IR,

	HK_TOGGLE_AR,
	HK_TOGGLE_EFBCOPIES,
	HK_TOGGLE_FOG,
	HK_TOGGLE_THROTTLE,

	HK_DECREASE_FRAME_LIMIT,
	HK_INCREASE_FRAME_LIMIT,

	HK_FREELOOK_DECREASE_SPEED,
	HK_FREELOOK_INCREASE_SPEED,
	HK_FREELOOK_RESET_SPEED,
	HK_FREELOOK_UP,
	HK_FREELOOK_DOWN,
	HK_FREELOOK_LEFT,
	HK_FREELOOK_RIGHT,
	HK_FREELOOK_ZOOM_IN,
	HK_FREELOOK_ZOOM_OUT,
	HK_FREELOOK_RESET,

	HK_DECREASE_DEPTH,
	HK_INCREASE_DEPTH,
	HK_DECREASE_CONVERGENCE,
	HK_INCREASE_CONVERGENCE,

	HK_LOAD_STATE_SLOT_1,
	HK_LOAD_STATE_SLOT_2,
	HK_LOAD_STATE_SLOT_3,
	HK_LOAD_STATE_SLOT_4,
	HK_LOAD_STATE_SLOT_5,
	HK_LOAD_STATE_SLOT_6,
	HK_LOAD_STATE_SLOT_7,
	HK_LOAD_STATE_SLOT_8,
	HK_LOAD_STATE_SLOT_9,
	HK_LOAD_STATE_SLOT_10,

	HK_SAVE_STATE_SLOT_1,
	HK_SAVE_STATE_SLOT_2,
	HK_SAVE_STATE_SLOT_3,
	HK_SAVE_STATE_SLOT_4,
	HK_SAVE_STATE_SLOT_5,
	HK_SAVE_STATE_SLOT_6,
	HK_SAVE_STATE_SLOT_7,
	HK_SAVE_STATE_SLOT_8,
	HK_SAVE_STATE_SLOT_9,
	HK_SAVE_STATE_SLOT_10,

	HK_SELECT_STATE_SLOT_1,
	HK_SELECT_STATE_SLOT_2,
	HK_SELECT_STATE_SLOT_3,
	HK_SELECT_STATE_SLOT_4,
	HK_SELECT_STATE_SLOT_5,
	HK_SELECT_STATE_SLOT_6,
	HK_SELECT_STATE_SLOT_7,
	HK_SELECT_STATE_SLOT_8,
	HK_SELECT_STATE_SLOT_9,
	HK_SELECT_STATE_SLOT_10,

	HK_SAVE_STATE_SLOT_SELECTED,
	HK_LOAD_STATE_SLOT_SELECTED,

	HK_LOAD_LAST_STATE_1,
	HK_LOAD_LAST_STATE_2,
	HK_LOAD_LAST_STATE_3,
	HK_LOAD_LAST_STATE_4,
	HK_LOAD_LAST_STATE_5,
	HK_LOAD_LAST_STATE_6,
	HK_LOAD_LAST_STATE_7,
	HK_LOAD_LAST_STATE_8,
	HK_LOAD_LAST_STATE_9,
	HK_LOAD_LAST_STATE_10,

	HK_SAVE_FIRST_STATE,
	HK_UNDO_LOAD_STATE,
	HK_UNDO_SAVE_STATE,
	HK_SAVE_STATE_FILE,
	HK_LOAD_STATE_FILE,

	NUM_HOTKEYS,
};

struct HotkeyStatus
{
	u32 button[(NUM_HOTKEYS + 31) / 32];
	s8  err;
};

class HotkeyManager : public ControllerEmu
{
public:
	HotkeyManager();
	~HotkeyManager();

	std::string GetName() const;
	void GetInput(HotkeyStatus* const hk);
	void LoadDefaults(const ControllerInterface& ciface);

private:
	Buttons* m_keys[(NUM_HOTKEYS + 31) / 32];
	ControlGroup* m_options;
};

namespace HotkeyManagerEmu
{
	void Initialize(void* const hwnd);
	void Shutdown();
	void LoadConfig();

	InputConfig* GetConfig();
	void GetStatus();
	bool IsEnabled();
	void Enable(bool enable_toggle);
	bool IsPressed(int Id, bool held);
}
