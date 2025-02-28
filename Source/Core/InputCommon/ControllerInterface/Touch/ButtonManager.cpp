// Copyright 2013 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Common/Assert.h"
#include "Common/FileUtil.h"
#include "Common/IniFile.h"
#include "Common/StringUtil.h"
#include "Common/Thread.h"

#include "Core/Core.h"
#include "Core/HW/GCPad.h"
#include "Core/HW/GCPadEmu.h"
#include "Core/HW/Wiimote.h"
#include "Core/HW/WiimoteEmu/Extension/Classic.h"
#include "Core/HW/WiimoteEmu/Extension/Nunchuk.h"
#include "Core/HW/WiimoteEmu/WiimoteEmu.h"

#include "InputCommon/ControllerEmu/ControlGroup/ControlGroup.h"
#include "InputCommon/ControllerEmu/StickGate.h"
#include "InputCommon/ControllerInterface/Touch/ButtonManager.h"

namespace ButtonManager
{
namespace
{
constexpr char TOUCHSCREEN_KEY[] = "Touchscreen";
constexpr std::array<const char*, 157> CONFIG_STRINGS{{
    // GC
    "InputA",
    "InputB",
    "InputStart",
    "InputX",
    "InputY",
    "InputZ",
    "DPadUp",
    "DPadDown",
    "DPadLeft",
    "DPadRight",
    "MainUp",
    "MainDown",
    "MainLeft",
    "MainRight",
    "CStickUp",
    "CStickDown",
    "CStickLeft",
    "CStickRight",
    "InputL",
    "InputR",
    "InputLAnalog",
    "InputRAnalog",
    // Wiimote
    "WiimoteA",
    "WiimoteB",
    "WiimoteMinus",
    "WiimotePlus",
    "WiimoteHome",
    "Wiimote1",
    "Wiimote2",
    "WiimoteUp",
    "WiimoteDown",
    "WiimoteLeft",
    "WiimoteRight",
    "IRUp",
    "IRDown",
    "IRLeft",
    "IRRight",
    "IRForward",
    "IRBackward",
    "IRHide",
    "SwingUp",
    "SwingDown",
    "SwingLeft",
    "SwingRight",
    "SwingForward",
    "SwingBackward",
    "TiltForward",
    "TiltBackward",
    "TiltLeft",
    "TiltRight",
    "TiltModifier",
    "ShakeX",
    "ShakeY",
    "ShakeZ",
    // Nunchuk
    "NunchukC",
    "NunchukZ",
    "NunchukUp",
    "NunchukDown",
    "NunchukLeft",
    "NunchukRight",
    "NunchukSwingUp",
    "NunchukSwingDown",
    "NunchukSwingLeft",
    "NunchukSwingRight",
    "NunchukSwingForward",
    "NunchukSwingBackward",
    "NunchukTiltForward",
    "NunchukTiltBackward",
    "NunchukTiltLeft",
    "NunchukTiltRight",
    "NunchukTiltModifier",
    "NunchukShakeX",
    "NunchukShakeY",
    "NunchukShakeZ",
    // Classic
    "ClassicA",
    "ClassicB",
    "ClassicX",
    "ClassicY",
    "ClassicMinus",
    "ClassicPlus",
    "ClassicHome",
    "ClassicZL",
    "ClassicZR",
    "ClassicUp",
    "ClassicDown",
    "ClassicLeft",
    "ClassicRight",
    "ClassicLeftStickUp",
    "ClassicLeftStickDown",
    "ClassicLeftStickLeft",
    "ClassicLeftStickRight",
    "ClassicRightStickUp",
    "ClassicRightStickDown",
    "ClassicRightStickLeft",
    "ClassicRightStickRight",
    "ClassicTriggerL",
    "ClassicTriggerR",
    // Guitar
    "GuitarMinus",
    "GuitarPlus",
    "GuitarGreen",
    "GuitarRed",
    "GuitarYellow",
    "GuitarBlue",
    "GuitarOrange",
    "GuitarStrumUp",
    "GuitarStrumDown",
    "GuitarUp",
    "GuitarDown",
    "GuitarLeft",
    "GuitarRight",
    "GuitarWhammy",
    // Drums
    "DrumsMinus",
    "DrumsPlus",
    "DrumsRed",
    "DrumsYellow",
    "DrumsBlue",
    "DrumsGreen",
    "DrumsOrange",
    "DrumsBass",
    "DrumsUp",
    "DrumsDown",
    "DrumsLeft",
    "DrumsRight",
    // Turntable
    "TurntableGreenLeft",
    "TurntableRedLeft",
    "TurntableBlueLeft",
    "TurntableGreenRight",
    "TurntableRedRight",
    "TurntableBlueRight",
    "TurntableMinus",
    "TurntablePlus",
    "TurntableHome",
    "TurntableEuphoria",
    "TurntableLeftTLeft",
    "TurntableLeftTRight",
    "TurntableRightTLeft",
    "TurntableRightTRight",
    "TurntableUp",
    "TurntableDown",
    "TurntableLeft",
    "TurntableRight",
    "TurntableEffDial",
    "TurntableCrossLeft",
    "TurntableCrossRight",
    // Wiimote IMU
    "WiimoteAccelLeft",
    "WiimoteAccelRight",
    "WiimoteAccelForward",
    "WiimoteAccelBackward",
    "WiimoteAccelUp",
    "WiimoteAccelDown",
    "WiimoteGyroPitchUp",
    "WiimoteGyroPitchDown",
    "WiimoteGyroRollLeft",
    "WiimoteGyroRollRight",
    "WiimoteGyroYawLeft",
    "WiimoteGyroYawRight",
    // Rumble
    "Rumble",
}};

constexpr std::array<ButtonType, 157> CONFIG_TYPES{{
    // GC
    BUTTON_A,
    BUTTON_B,
    BUTTON_START,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_Z,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    STICK_MAIN_UP,
    STICK_MAIN_DOWN,
    STICK_MAIN_LEFT,
    STICK_MAIN_RIGHT,
    STICK_C_UP,
    STICK_C_DOWN,
    STICK_C_LEFT,
    STICK_C_RIGHT,
    TRIGGER_L,
    TRIGGER_R,
    TRIGGER_L_ANALOG,
    TRIGGER_R_ANALOG,
    // Wiimote
    WIIMOTE_BUTTON_A,
    WIIMOTE_BUTTON_B,
    WIIMOTE_BUTTON_MINUS,
    WIIMOTE_BUTTON_PLUS,
    WIIMOTE_BUTTON_HOME,
    WIIMOTE_BUTTON_1,
    WIIMOTE_BUTTON_2,
    WIIMOTE_UP,
    WIIMOTE_DOWN,
    WIIMOTE_LEFT,
    WIIMOTE_RIGHT,
    WIIMOTE_IR_UP,
    WIIMOTE_IR_DOWN,
    WIIMOTE_IR_LEFT,
    WIIMOTE_IR_RIGHT,
    WIIMOTE_IR_FORWARD,
    WIIMOTE_IR_BACKWARD,
    WIIMOTE_IR_HIDE,
    WIIMOTE_SWING_UP,
    WIIMOTE_SWING_DOWN,
    WIIMOTE_SWING_LEFT,
    WIIMOTE_SWING_RIGHT,
    WIIMOTE_SWING_FORWARD,
    WIIMOTE_SWING_BACKWARD,
    WIIMOTE_TILT_FORWARD,
    WIIMOTE_TILT_BACKWARD,
    WIIMOTE_TILT_LEFT,
    WIIMOTE_TILT_RIGHT,
    WIIMOTE_TILT_MODIFIER,
    WIIMOTE_SHAKE_X,
    WIIMOTE_SHAKE_Y,
    WIIMOTE_SHAKE_Z,
    // Nunchuk
    NUNCHUK_BUTTON_C,
    NUNCHUK_BUTTON_Z,
    NUNCHUK_STICK_UP,
    NUNCHUK_STICK_DOWN,
    NUNCHUK_STICK_LEFT,
    NUNCHUK_STICK_RIGHT,
    NUNCHUK_SWING_UP,
    NUNCHUK_SWING_DOWN,
    NUNCHUK_SWING_LEFT,
    NUNCHUK_SWING_RIGHT,
    NUNCHUK_SWING_FORWARD,
    NUNCHUK_SWING_BACKWARD,
    NUNCHUK_TILT_FORWARD,
    NUNCHUK_TILT_BACKWARD,
    NUNCHUK_TILT_LEFT,
    NUNCHUK_TILT_RIGHT,
    NUNCHUK_TILT_MODIFIER,
    NUNCHUK_SHAKE_X,
    NUNCHUK_SHAKE_Y,
    NUNCHUK_SHAKE_Z,
    // Classic
    CLASSIC_BUTTON_A,
    CLASSIC_BUTTON_B,
    CLASSIC_BUTTON_X,
    CLASSIC_BUTTON_Y,
    CLASSIC_BUTTON_MINUS,
    CLASSIC_BUTTON_PLUS,
    CLASSIC_BUTTON_HOME,
    CLASSIC_BUTTON_ZL,
    CLASSIC_BUTTON_ZR,
    CLASSIC_DPAD_UP,
    CLASSIC_DPAD_DOWN,
    CLASSIC_DPAD_LEFT,
    CLASSIC_DPAD_RIGHT,
    CLASSIC_STICK_LEFT_UP,
    CLASSIC_STICK_LEFT_DOWN,
    CLASSIC_STICK_LEFT_LEFT,
    CLASSIC_STICK_LEFT_RIGHT,
    CLASSIC_STICK_RIGHT_UP,
    CLASSIC_STICK_RIGHT_DOWN,
    CLASSIC_STICK_RIGHT_LEFT,
    CLASSIC_STICK_RIGHT_RIGHT,
    CLASSIC_TRIGGER_L,
    CLASSIC_TRIGGER_R,
    // Guitar
    GUITAR_BUTTON_MINUS,
    GUITAR_BUTTON_PLUS,
    GUITAR_FRET_GREEN,
    GUITAR_FRET_RED,
    GUITAR_FRET_YELLOW,
    GUITAR_FRET_BLUE,
    GUITAR_FRET_ORANGE,
    GUITAR_STRUM_UP,
    GUITAR_STRUM_DOWN,
    GUITAR_STICK_UP,
    GUITAR_STICK_DOWN,
    GUITAR_STICK_LEFT,
    GUITAR_STICK_RIGHT,
    GUITAR_WHAMMY_BAR,
    // Drums
    DRUMS_BUTTON_MINUS,
    DRUMS_BUTTON_PLUS,
    DRUMS_PAD_RED,
    DRUMS_PAD_YELLOW,
    DRUMS_PAD_BLUE,
    DRUMS_PAD_GREEN,
    DRUMS_PAD_ORANGE,
    DRUMS_PAD_BASS,
    DRUMS_STICK_UP,
    DRUMS_STICK_DOWN,
    DRUMS_STICK_LEFT,
    DRUMS_STICK_RIGHT,
    // Turntable
    TURNTABLE_BUTTON_GREEN_LEFT,
    TURNTABLE_BUTTON_RED_LEFT,
    TURNTABLE_BUTTON_BLUE_LEFT,
    TURNTABLE_BUTTON_GREEN_RIGHT,
    TURNTABLE_BUTTON_RED_RIGHT,
    TURNTABLE_BUTTON_BLUE_RIGHT,
    TURNTABLE_BUTTON_MINUS,
    TURNTABLE_BUTTON_PLUS,
    TURNTABLE_BUTTON_HOME,
    TURNTABLE_BUTTON_EUPHORIA,
    TURNTABLE_TABLE_LEFT_LEFT,
    TURNTABLE_TABLE_LEFT_RIGHT,
    TURNTABLE_TABLE_RIGHT_LEFT,
    TURNTABLE_TABLE_RIGHT_RIGHT,
    TURNTABLE_STICK_UP,
    TURNTABLE_STICK_DOWN,
    TURNTABLE_STICK_LEFT,
    TURNTABLE_STICK_RIGHT,
    TURNTABLE_EFFECT_DIAL,
    TURNTABLE_CROSSFADE_LEFT,
    TURNTABLE_CROSSFADE_RIGHT,
    // Wiimote IMU
    WIIMOTE_ACCEL_LEFT,
    WIIMOTE_ACCEL_RIGHT,
    WIIMOTE_ACCEL_FORWARD,
    WIIMOTE_ACCEL_BACKWARD,
    WIIMOTE_ACCEL_UP,
    WIIMOTE_ACCEL_DOWN,
    WIIMOTE_GYRO_PITCH_UP,
    WIIMOTE_GYRO_PITCH_DOWN,
    WIIMOTE_GYRO_ROLL_LEFT,
    WIIMOTE_GYRO_ROLL_RIGHT,
    WIIMOTE_GYRO_YAW_LEFT,
    WIIMOTE_GYRO_YAW_RIGHT,
    // Rumble
    RUMBLE,
}};

std::unordered_map<std::string, InputDevice*> m_controllers;

void AddBind(const std::string& dev, sBind* bind)
{
  auto it = m_controllers.find(dev);
  if (it != m_controllers.end())
  {
    it->second->AddBind(bind);
    return;
  }
  m_controllers[dev] = new InputDevice(dev);
  m_controllers[dev]->AddBind(bind);
}
}  // Anonymous namespace

void Init(const std::string& game_id)
{
  // Initialize pad 0(gc 1) and pad 4(wii 1) as touch overlay controller
  for (int a = 0; a < 5; a += 4)
  {
    // GC
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_A, BIND_BUTTON, BUTTON_A, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_B, BIND_BUTTON, BUTTON_B, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_START, BIND_BUTTON, BUTTON_START, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_X, BIND_BUTTON, BUTTON_X, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_Y, BIND_BUTTON, BUTTON_Y, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_Z, BIND_BUTTON, BUTTON_Z, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_UP, BIND_BUTTON, BUTTON_UP, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_DOWN, BIND_BUTTON, BUTTON_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_LEFT, BIND_BUTTON, BUTTON_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, BUTTON_RIGHT, BIND_BUTTON, BUTTON_RIGHT, 1.0f));

    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_MAIN_UP, BIND_AXIS, STICK_MAIN_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_MAIN_DOWN, BIND_AXIS, STICK_MAIN_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_MAIN_LEFT, BIND_AXIS, STICK_MAIN_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_MAIN_RIGHT, BIND_AXIS, STICK_MAIN_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_C_UP, BIND_AXIS, STICK_C_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_C_DOWN, BIND_AXIS, STICK_C_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_C_LEFT, BIND_AXIS, STICK_C_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, STICK_C_RIGHT, BIND_AXIS, STICK_C_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TRIGGER_L, BIND_AXIS, TRIGGER_L, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TRIGGER_R, BIND_AXIS, TRIGGER_R, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TRIGGER_L_ANALOG, BIND_AXIS, TRIGGER_L_ANALOG, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TRIGGER_R_ANALOG, BIND_AXIS, TRIGGER_R_ANALOG, 1.0f));

    // Wiimote
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_BUTTON_A, BIND_BUTTON, WIIMOTE_BUTTON_A, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_BUTTON_B, BIND_BUTTON, WIIMOTE_BUTTON_B, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_BUTTON_MINUS, BIND_BUTTON, WIIMOTE_BUTTON_MINUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_BUTTON_PLUS, BIND_BUTTON, WIIMOTE_BUTTON_PLUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_BUTTON_HOME, BIND_BUTTON, WIIMOTE_BUTTON_HOME, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_BUTTON_1, BIND_BUTTON, WIIMOTE_BUTTON_1, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_BUTTON_2, BIND_BUTTON, WIIMOTE_BUTTON_2, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_UP, BIND_BUTTON, WIIMOTE_UP, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_DOWN, BIND_BUTTON, WIIMOTE_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_LEFT, BIND_BUTTON, WIIMOTE_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_RIGHT, BIND_BUTTON, WIIMOTE_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_IR_HIDE, BIND_BUTTON, WIIMOTE_IR_HIDE, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_TILT_MODIFIER, BIND_BUTTON, WIIMOTE_TILT_MODIFIER, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_SHAKE_X, BIND_BUTTON, WIIMOTE_SHAKE_X, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_SHAKE_Y, BIND_BUTTON, WIIMOTE_SHAKE_Y, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_SHAKE_Z, BIND_BUTTON, WIIMOTE_SHAKE_Z, 1.0f));

    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_IR_UP, BIND_AXIS, WIIMOTE_IR_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_IR_DOWN, BIND_AXIS, WIIMOTE_IR_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_IR_LEFT, BIND_AXIS, WIIMOTE_IR_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_IR_RIGHT, BIND_AXIS, WIIMOTE_IR_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_IR_FORWARD, BIND_AXIS, WIIMOTE_IR_FORWARD, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_IR_BACKWARD, BIND_AXIS, WIIMOTE_IR_BACKWARD, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_SWING_UP, BIND_AXIS, WIIMOTE_SWING_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_SWING_DOWN, BIND_AXIS, WIIMOTE_SWING_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_SWING_LEFT, BIND_AXIS, WIIMOTE_SWING_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_SWING_RIGHT, BIND_AXIS, WIIMOTE_SWING_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_SWING_FORWARD, BIND_AXIS, WIIMOTE_SWING_FORWARD, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_SWING_BACKWARD, BIND_AXIS, WIIMOTE_SWING_BACKWARD, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_TILT_FORWARD, BIND_AXIS, WIIMOTE_TILT_FORWARD, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_TILT_BACKWARD, BIND_AXIS, WIIMOTE_TILT_BACKWARD, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_TILT_LEFT, BIND_AXIS, WIIMOTE_TILT_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_TILT_RIGHT, BIND_AXIS, WIIMOTE_TILT_RIGHT, 1.0f));

    // Hotkeys
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, HOTKEYS_SIDEWAYS_TOGGLE, BIND_BUTTON, HOTKEYS_SIDEWAYS_TOGGLE, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, HOTKEYS_UPRIGHT_TOGGLE, BIND_BUTTON, HOTKEYS_UPRIGHT_TOGGLE, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, HOTKEYS_SIDEWAYS_HOLD, BIND_BUTTON, HOTKEYS_SIDEWAYS_HOLD, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, HOTKEYS_UPRIGHT_HOLD, BIND_BUTTON, HOTKEYS_UPRIGHT_HOLD, 1.0f));

    // Wii: Nunchuk
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_BUTTON_C, BIND_BUTTON, NUNCHUK_BUTTON_C, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_BUTTON_Z, BIND_BUTTON, NUNCHUK_BUTTON_Z, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_TILT_MODIFIER, BIND_BUTTON, NUNCHUK_TILT_MODIFIER, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_SHAKE_X, BIND_BUTTON, NUNCHUK_SHAKE_X, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_SHAKE_Y, BIND_BUTTON, NUNCHUK_SHAKE_Y, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_SHAKE_Z, BIND_BUTTON, NUNCHUK_SHAKE_Z, 1.0f));

    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_SWING_UP, BIND_AXIS, NUNCHUK_SWING_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_SWING_DOWN, BIND_AXIS, NUNCHUK_SWING_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_SWING_LEFT, BIND_AXIS, NUNCHUK_SWING_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_SWING_RIGHT, BIND_AXIS, NUNCHUK_SWING_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_SWING_FORWARD, BIND_AXIS, NUNCHUK_SWING_FORWARD, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_SWING_BACKWARD, BIND_BUTTON, NUNCHUK_SWING_BACKWARD, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_TILT_FORWARD, BIND_AXIS, NUNCHUK_TILT_FORWARD, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_TILT_BACKWARD, BIND_AXIS, NUNCHUK_TILT_BACKWARD, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_TILT_LEFT, BIND_AXIS, NUNCHUK_TILT_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_TILT_RIGHT, BIND_AXIS, NUNCHUK_TILT_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_STICK_UP, BIND_AXIS, NUNCHUK_STICK_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, NUNCHUK_STICK_DOWN, BIND_AXIS, NUNCHUK_STICK_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_STICK_LEFT, BIND_AXIS, NUNCHUK_STICK_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, NUNCHUK_STICK_RIGHT, BIND_AXIS, NUNCHUK_STICK_RIGHT, 1.0f));

    // Wii: Classic
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_BUTTON_A, BIND_BUTTON, CLASSIC_BUTTON_A, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_BUTTON_B, BIND_BUTTON, CLASSIC_BUTTON_B, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_BUTTON_X, BIND_BUTTON, CLASSIC_BUTTON_X, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_BUTTON_Y, BIND_BUTTON, CLASSIC_BUTTON_Y, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_BUTTON_MINUS, BIND_BUTTON, CLASSIC_BUTTON_MINUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_BUTTON_PLUS, BIND_BUTTON, CLASSIC_BUTTON_PLUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_BUTTON_HOME, BIND_BUTTON, CLASSIC_BUTTON_HOME, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_BUTTON_ZL, BIND_BUTTON, CLASSIC_BUTTON_ZL, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_BUTTON_ZR, BIND_BUTTON, CLASSIC_BUTTON_ZR, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_DPAD_UP, BIND_BUTTON, CLASSIC_DPAD_UP, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_DPAD_DOWN, BIND_BUTTON, CLASSIC_DPAD_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_DPAD_LEFT, BIND_BUTTON, CLASSIC_DPAD_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_DPAD_RIGHT, BIND_BUTTON, CLASSIC_DPAD_RIGHT, 1.0f));

    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_LEFT_UP, BIND_AXIS, CLASSIC_STICK_LEFT_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_LEFT_DOWN, BIND_AXIS, CLASSIC_STICK_LEFT_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_LEFT_LEFT, BIND_AXIS, CLASSIC_STICK_LEFT_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_LEFT_RIGHT, BIND_AXIS, CLASSIC_STICK_LEFT_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_RIGHT_UP, BIND_AXIS, CLASSIC_STICK_RIGHT_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_RIGHT_DOWN, BIND_AXIS, CLASSIC_STICK_RIGHT_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_RIGHT_LEFT, BIND_AXIS, CLASSIC_STICK_RIGHT_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, CLASSIC_STICK_RIGHT_RIGHT, BIND_AXIS, CLASSIC_STICK_RIGHT_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_TRIGGER_L, BIND_AXIS, CLASSIC_TRIGGER_L, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, CLASSIC_TRIGGER_R, BIND_AXIS, CLASSIC_TRIGGER_R, 1.0f));

    // Wii: Guitar
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, GUITAR_BUTTON_MINUS, BIND_BUTTON, GUITAR_BUTTON_MINUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, GUITAR_BUTTON_PLUS, BIND_BUTTON, GUITAR_BUTTON_PLUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_FRET_GREEN, BIND_BUTTON, GUITAR_FRET_GREEN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_FRET_RED, BIND_BUTTON, GUITAR_FRET_RED, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, GUITAR_FRET_YELLOW, BIND_BUTTON, GUITAR_FRET_YELLOW, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_FRET_BLUE, BIND_BUTTON, GUITAR_FRET_BLUE, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, GUITAR_FRET_ORANGE, BIND_BUTTON, GUITAR_FRET_ORANGE, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_STRUM_UP, BIND_BUTTON, GUITAR_STRUM_UP, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_STRUM_DOWN, BIND_BUTTON, GUITAR_STRUM_DOWN, 1.0f));

    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_STICK_UP, BIND_AXIS, GUITAR_STICK_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_STICK_DOWN, BIND_AXIS, GUITAR_STICK_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_STICK_LEFT, BIND_AXIS, GUITAR_STICK_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_STICK_RIGHT, BIND_AXIS, GUITAR_STICK_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, GUITAR_WHAMMY_BAR, BIND_AXIS, GUITAR_WHAMMY_BAR, 1.0f));

    // Wii: Drums
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, DRUMS_BUTTON_MINUS, BIND_BUTTON, DRUMS_BUTTON_MINUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_BUTTON_PLUS, BIND_BUTTON, DRUMS_BUTTON_PLUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_PAD_RED, BIND_BUTTON, DRUMS_PAD_RED, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_PAD_YELLOW, BIND_BUTTON, DRUMS_PAD_YELLOW, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_PAD_BLUE, BIND_BUTTON, DRUMS_PAD_BLUE, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_PAD_GREEN, BIND_BUTTON, DRUMS_PAD_GREEN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_PAD_ORANGE, BIND_BUTTON, DRUMS_PAD_ORANGE, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_PAD_BASS, BIND_BUTTON, DRUMS_PAD_BASS, 1.0f));

    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_STICK_UP, BIND_AXIS, DRUMS_STICK_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_STICK_DOWN, BIND_AXIS, DRUMS_STICK_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_STICK_LEFT, BIND_AXIS, DRUMS_STICK_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, DRUMS_STICK_RIGHT, BIND_AXIS, DRUMS_STICK_RIGHT, 1.0f));

    // Wii: Turntable
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TURNTABLE_BUTTON_GREEN_LEFT, BIND_BUTTON,
                                       TURNTABLE_BUTTON_GREEN_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_BUTTON_RED_LEFT, BIND_BUTTON, TURNTABLE_BUTTON_RED_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TURNTABLE_BUTTON_BLUE_LEFT, BIND_BUTTON,
                                       TURNTABLE_BUTTON_BLUE_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TURNTABLE_BUTTON_GREEN_RIGHT, BIND_BUTTON,
                                       TURNTABLE_BUTTON_GREEN_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TURNTABLE_BUTTON_RED_RIGHT, BIND_BUTTON,
                                       TURNTABLE_BUTTON_RED_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TURNTABLE_BUTTON_BLUE_RIGHT, BIND_BUTTON,
                                       TURNTABLE_BUTTON_BLUE_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_BUTTON_MINUS, BIND_BUTTON, TURNTABLE_BUTTON_MINUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_BUTTON_PLUS, BIND_BUTTON, TURNTABLE_BUTTON_PLUS, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_BUTTON_HOME, BIND_BUTTON, TURNTABLE_BUTTON_HOME, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_BUTTON_EUPHORIA, BIND_BUTTON, TURNTABLE_BUTTON_EUPHORIA, 1.0f));

    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_TABLE_LEFT_LEFT, BIND_AXIS, TURNTABLE_TABLE_LEFT_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_TABLE_LEFT_RIGHT, BIND_AXIS, TURNTABLE_TABLE_LEFT_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_TABLE_RIGHT_LEFT, BIND_AXIS, TURNTABLE_TABLE_RIGHT_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, TURNTABLE_TABLE_RIGHT_RIGHT, BIND_AXIS,
                                       TURNTABLE_TABLE_RIGHT_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_STICK_UP, BIND_AXIS, TURNTABLE_STICK_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_STICK_DOWN, BIND_AXIS, TURNTABLE_STICK_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_STICK_LEFT, BIND_AXIS, TURNTABLE_STICK_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_STICK_RIGHT, BIND_AXIS, TURNTABLE_STICK_RIGHT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_EFFECT_DIAL, BIND_AXIS, TURNTABLE_EFFECT_DIAL, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_CROSSFADE_LEFT, BIND_AXIS, TURNTABLE_CROSSFADE_LEFT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, TURNTABLE_CROSSFADE_RIGHT, BIND_AXIS, TURNTABLE_CROSSFADE_RIGHT, 1.0f));

    // Wiimote IMU
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_ACCEL_LEFT, BIND_AXIS, WIIMOTE_ACCEL_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_ACCEL_RIGHT, BIND_AXIS, WIIMOTE_ACCEL_RIGHT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_ACCEL_FORWARD, BIND_AXIS, WIIMOTE_ACCEL_FORWARD, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_ACCEL_BACKWARD, BIND_AXIS, WIIMOTE_ACCEL_BACKWARD, 1.0f));
    AddBind(TOUCHSCREEN_KEY, new sBind(a, WIIMOTE_ACCEL_UP, BIND_AXIS, WIIMOTE_ACCEL_UP, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_ACCEL_DOWN, BIND_AXIS, WIIMOTE_ACCEL_DOWN, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_GYRO_PITCH_UP, BIND_AXIS, WIIMOTE_GYRO_PITCH_UP, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_GYRO_PITCH_DOWN, BIND_AXIS, WIIMOTE_GYRO_PITCH_DOWN, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_GYRO_ROLL_LEFT, BIND_AXIS, WIIMOTE_GYRO_ROLL_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_GYRO_ROLL_RIGHT, BIND_AXIS, WIIMOTE_GYRO_ROLL_RIGHT, -1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_GYRO_YAW_LEFT, BIND_AXIS, WIIMOTE_GYRO_YAW_LEFT, 1.0f));
    AddBind(TOUCHSCREEN_KEY,
            new sBind(a, WIIMOTE_GYRO_YAW_RIGHT, BIND_AXIS, WIIMOTE_GYRO_YAW_RIGHT, -1.0f));
  }
  // Init our controller bindings
  IniFile ini;
  ini.Load(File::GetUserPath(D_CONFIG_IDX) + std::string("Dolphin.ini"), true);
  ini.Load(File::GetUserPath(D_GAMESETTINGS_IDX) + std::string(game_id + ".ini"), true);
  for (u32 a = 0; a < CONFIG_STRINGS.size(); ++a)
  {
    for (int pad_id = 0; pad_id < 8; ++pad_id)
    {
      std::ostringstream config;
      config << CONFIG_STRINGS[a] << "_" << pad_id;
      BindType type;
      int bindnum;
      char dev[128]{};
      bool hasbind = false;
      char modifier = '+';
      std::string value;
      ini.GetOrCreateSection("Android")->Get(config.str(), &value, "None");
      if (value == "None")
        continue;
      if (std::string::npos != value.find("Axis"))
      {
        hasbind = true;
        type = BIND_AXIS;
        if (StringBeginsWith(value, "Device ''"))
          sscanf(value.c_str(), "Device ''-Axis %d%c", &bindnum, &modifier);
        else
          sscanf(value.c_str(), "Device '%127[^\']'-Axis %d%c", dev, &bindnum, &modifier);
      }
      else if (std::string::npos != value.find("Button"))
      {
        hasbind = true;
        type = BIND_BUTTON;
        if (StringBeginsWith(value, "Device ''"))
          sscanf(value.c_str(), "Device ''-Button %d", &bindnum);
        else
          sscanf(value.c_str(), "Device '%127[^\']'-Button %d", dev, &bindnum);
      }
      if (hasbind)
        AddBind(std::string(dev),
                new sBind(pad_id, CONFIG_TYPES[a], type, bindnum, modifier == '-' ? -1.0f : 1.0f));
    }
  }
}

bool GetButtonPressed(int pad_id, ButtonType button)
{
  bool pressed = m_controllers[TOUCHSCREEN_KEY]->ButtonValue(pad_id, button);

  for (const auto& ctrl : m_controllers)
    pressed |= ctrl.second->ButtonValue(pad_id, button);

  return pressed;
}

float GetAxisValue(int pad_id, ButtonType axis)
{
  float value = m_controllers[TOUCHSCREEN_KEY]->AxisValue(pad_id, axis);
  if (value == 0.0f)
  {
    for (const auto& ctrl : m_controllers)
    {
      value = ctrl.second->AxisValue(pad_id, axis);
      if (value != 0.0f)
        return value;
    }
  }
  return value;
}

double GetInputRadiusAtAngle(int pad_id, ButtonType stick, double angle)
{
  // To avoid a crash, don't access controllers before they've been initialized by the boot process
  if (!Core::IsRunningAndStarted())
    return 0;

  ControllerEmu::ControlGroup* group;

  switch (stick)
  {
  case STICK_MAIN:
  case STICK_EMULATION:
    group = Pad::GetGroup(pad_id, PadGroup::MainStick);
    break;
  case STICK_C:
    group = Pad::GetGroup(pad_id, PadGroup::CStick);
    break;
  case NUNCHUK_STICK:
    group = Wiimote::GetNunchukGroup(pad_id, WiimoteEmu::NunchukGroup::Stick);
    break;
  case CLASSIC_STICK_LEFT:
    group = Wiimote::GetClassicGroup(pad_id, WiimoteEmu::ClassicGroup::LeftStick);
    break;
  case CLASSIC_STICK_RIGHT:
    group = Wiimote::GetClassicGroup(pad_id, WiimoteEmu::ClassicGroup::RightStick);
    break;
  default:
    ASSERT(false);
    return 0;
  }

  return static_cast<ControllerEmu::ReshapableInput*>(group)->GetInputRadiusAtAngle(angle);
}

bool GamepadEvent(const std::string& dev, int button, int action)
{
  auto it = m_controllers.find(dev);
  if (it != m_controllers.end())
    return it->second->PressEvent(button, action);
  return false;
}

void GamepadAxisEvent(const std::string& dev, int axis, float value)
{
  auto it = m_controllers.find(dev);
  if (it != m_controllers.end())
    it->second->AxisEvent(axis, value);
}

void Shutdown()
{
  for (const auto& controller : m_controllers)
    delete controller.second;
  m_controllers.clear();
}

// InputDevice
bool InputDevice::PressEvent(int button, int action)
{
  bool handled = false;
  for (const auto& binding : m_input_binds)
  {
    if (binding.second->m_bind == button)
    {
      if (binding.second->m_bind_type == BIND_BUTTON)
        m_buttons[binding.second->m_button_type] = action == BUTTON_PRESSED ? true : false;
      else
        m_axes[binding.second->m_button_type] = action == BUTTON_PRESSED ? 1.0f : 0.0f;
      handled = true;
    }
  }
  return handled;
}

void InputDevice::AxisEvent(int axis, float value)
{
  for (const auto& binding : m_input_binds)
  {
    if (binding.second->m_bind == axis)
    {
      if (binding.second->m_bind_type == BIND_AXIS)
        m_axes[binding.second->m_button_type] = value;
      else
        m_buttons[binding.second->m_button_type] = value > 0.5f ? true : false;
    }
  }
}

bool InputDevice::ButtonValue(int pad_id, ButtonType button) const
{
  const auto binding = m_input_binds.find(std::make_pair(pad_id, button));
  if (binding == m_input_binds.end())
    return false;

  if (binding->second->m_bind_type == BIND_BUTTON)
  {
    const auto button = m_buttons.find(binding->second->m_button_type);
    if (button == m_buttons.end())
      return false;
    return button->second;
  }
  else
  {
    const auto axis = m_axes.find(binding->second->m_button_type);
    if (axis == m_axes.end())
      return false;
    return (axis->second * binding->second->m_neg) > 0.5f;
  }
}

float InputDevice::AxisValue(int pad_id, ButtonType axis) const
{
  const auto binding = m_input_binds.find(std::make_pair(pad_id, axis));
  if (binding == m_input_binds.end())
    return 0.0f;

  if (binding->second->m_bind_type == BIND_AXIS)
  {
    const auto axis = m_axes.find(binding->second->m_button_type);
    if (axis == m_axes.end())
      return 0.0f;
    return axis->second * binding->second->m_neg;
  }
  else
  {
    const auto button = m_buttons.find(binding->second->m_button_type);
    if (button == m_buttons.end())
      return 0.0f;
    return button->second == BUTTON_PRESSED ? 1.0f : 0.0f;
  }
}
}  // namespace ButtonManager
