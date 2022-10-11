#ifndef WIN_KEYCODE_HPP
#define WIN_KEYCODE_HPP

#include "System/Input/KeyCodes.hpp"
#include "pch.hpp"

KeyCode ConvertToKeyCode(unsigned char winKeyCode)
{
    switch (winKeyCode)
    {
    case VK_LBUTTON:
        return KeyCodeMouseL;
    case VK_RBUTTON:
        return KeyCodeMouseR;
    case VK_CANCEL:
        return KeyCodeControlBreak;
    case VK_MBUTTON:
        return KeyCodeMouseMiddle;
    case VK_XBUTTON1:
        return KeyCodeMouseX1;
    case VK_XBUTTON2:
        return KeyCodeMouseX2;
    case VK_BACK:
        return KeyCodeBackspace;
    case VK_TAB:
        return KeyCodeTab;
    case VK_CLEAR:
        return KeyCodeClear;
    case VK_RETURN:
        return KeyCodeReturn;
    case VK_SHIFT:
        return KeyCodeShift;
    case VK_CONTROL:
        return KeyCodeControl;
    case VK_MENU:
        return KeyCodeALT;
    case VK_PAUSE:
        return KeyCodePause;
    case VK_CAPITAL:
        return KeyCodeCapital;
    case VK_ESCAPE:
        return KeyCodeEscape;
    case VK_SPACE:
        return KeyCodeSpace;
    case VK_PRIOR:
        return KeyCodePageUp;
    case VK_NEXT:
        return KeyCodePageDown;
    case VK_END:
        return KeyCodeEnd;
    case VK_HOME:
        return KeyCodeHome;
    case VK_LEFT:
        return KeyCodeLeftArrow;
    case VK_UP:
        return KeyCodeUpArrow;
    case VK_RIGHT:
        return KeyCodeRightArrow;
    case VK_DOWN:
        return KeyCodeDownArrow;
    case VK_SELECT:
        return KeyCodeSelect;
    case VK_PRINT:
        return KeyCodePrint;
    case VK_EXECUTE:
        return KeyCodeExecute;
    case VK_SNAPSHOT:
        return KeyCodePrintScreen;
    case VK_INSERT:
        return KeyCodeInsert;
    case VK_DELETE:
        return KeyCodeDelete;
    case VK_HELP:
        return KeyCodeHelp;
    case 0x30:
        return KeyCodeZero; // 0 key
    case 0x31:
        return KeyCodeOne; // 1 key
    case 0x32:
        return KeyCodeTwo; // 2 key
    case 0x33:
        return KeyCodeThree; // 3 key
    case 0x34:
        return KeyCodeFour; // 4 key
    case 0x35:
        return KeyCodeFive; // 5 key
    case 0x36:
        return KeyCodeSix; // 6 key
    case 0x37:
        return KeyCodeSeven; // 7 key
    case 0x38:
        return KeyCodeEight; // 8 key
    case 0x39:
        return KeyCodeNine; // 9 key
    case 0x41:
        return KeyCodeA; // A key
    case 0x42:
        return KeyCodeB; // B key
    case 0x43:
        return KeyCodeC; // C key
    case 0x44:
        return KeyCodeD; // D key
    case 0x45:
        return KeyCodeE; // E key
    case 0x46:
        return KeyCodeF; // F key
    case 0x47:
        return KeyCodeG; // G key
    case 0x48:
        return KeyCodeH; // H key
    case 0x49:
        return KeyCodeI; // I key
    case 0x4A:
        return KeyCodeJ; // J key
    case 0x4B:
        return KeyCodeK; // K key
    case 0x4C:
        return KeyCodeL; // L key
    case 0x4D:
        return KeyCodeM; // M key
    case 0x4E:
        return KeyCodeN; // N key
    case 0x4F:
        return KeyCodeO; // O key
    case 0x50:
        return KeyCodeP; // P key
    case 0x51:
        return KeyCodeQ; // Q key
    case 0x52:
        return KeyCodeR; // R key
    case 0x53:
        return KeyCodeS; // S key
    case 0x54:
        return KeyCodeT; // T key
    case 0x55:
        return KeyCodeU; // U key
    case 0x56:
        return KeyCodeV; // V key
    case 0x57:
        return KeyCodeW; // W key
    case 0x58:
        return KeyCodeX; // X key
    case 0x59:
        return KeyCodeY; // Y key
    case 0x5A:
        return KeyCodeZ; // Z key
    case VK_LWIN:
        return KeyCodeLeftWindows;
    case VK_RWIN:
        return KeyCodeRightWindows;
    case VK_APPS:
        return KeyCodeApplications;
    case VK_SLEEP:
        return KeyCodeSleep;
    case VK_NUMPAD0:
        return KeyCodeNumPad0;
    case VK_NUMPAD1:
        return KeyCodeNumPad1;
    case VK_NUMPAD2:
        return KeyCodeNumPad1;
    case VK_NUMPAD3:
        return KeyCodeNumPad1;
    case VK_NUMPAD4:
        return KeyCodeNumPad4;
    case VK_NUMPAD5:
        return KeyCodeNumPad5;
    case VK_NUMPAD6:
        return KeyCodeNumPad6;
    case VK_NUMPAD7:
        return KeyCodeNumPad7;
    case VK_NUMPAD8:
        return KeyCodeNumPad8;
    case VK_NUMPAD9:
        return KeyCodeNumPad9;
    case VK_MULTIPLY:
        return KeyCodeMultiply;
    case VK_ADD:
        return KeyCodeAdd;
    case VK_SUBTRACT:
        return KeyCodeSubtract;
    case VK_DECIMAL:
        return KeyCodeDecimal;
    case VK_DIVIDE:
        return KeyCodeDivide;
    case VK_F1:
        return KeyCodeF1;
    case VK_F2:
        return KeyCodeF2;
    case VK_F3:
        return KeyCodeF3;
    case VK_F4:
        return KeyCodeF4;
    case VK_F5:
        return KeyCodeF5;
    case VK_F6:
        return KeyCodeF6;
    case VK_F7:
        return KeyCodeF7;
    case VK_F8:
        return KeyCodeF8;
    case VK_F9:
        return KeyCodeF9;
    case VK_F10:
        return KeyCodeF10;
    case VK_F11:
        return KeyCodeF11;
    case VK_F12:
        return KeyCodeF12;
    case VK_F13:
        return KeyCodeF13;
    case VK_F14:
        return KeyCodeF14;
    case VK_F15:
        return KeyCodeF15;
    case VK_F16:
        return KeyCodeF16;
    case VK_F17:
        return KeyCodeF17;
    case VK_F18:
        return KeyCodeF18;
    case VK_F19:
        return KeyCodeF19;
    case VK_F20:
        return KeyCodeF20;
    case VK_F21:
        return KeyCodeF21;
    case VK_F22:
        return KeyCodeF22;
    case VK_F23:
        return KeyCodeF23;
    case VK_F24:
        return KeyCodeF24;
    case VK_NUMLOCK:
        return KeyCodeNumLock;
    case VK_SCROLL:
        return KeyCodeScrollLock;
    case VK_LSHIFT:
        return KeyCodeLeftShift;
    case VK_RSHIFT:
        return KeyCodeRightShift;
    case VK_LCONTROL:
        return KeyCodeLeftControl;
    case VK_RCONTROL:
        return KeyCodeRightControl;
    case VK_LMENU:
        return KeyCodeLeftALT;
    case VK_RMENU:
        return KeyCodeRightALT;
    case VK_BROWSER_BACK:
        return KeyCodeBrowserBack;
    case VK_BROWSER_FORWARD:
        return KeyCodeBrowserForward;
    case VK_BROWSER_REFRESH:
        return KeyCodeBrowserRefresh;
    case VK_BROWSER_STOP:
        return KeyCodeBrowserStop;
    case VK_BROWSER_SEARCH:
        return KeyCodeBrowserSearch;
    case VK_BROWSER_FAVORITES:
        return KeyCodeBrowserFavorites;
    case VK_BROWSER_HOME:
        return KeyCodeBrowserHome;
    case VK_VOLUME_MUTE:
        return KeyCodeVolumeMute;
    case VK_VOLUME_DOWN:
        return KeyCodeVolumeDown;
    case VK_VOLUME_UP:
        return KeyCodeVolumeUp;
    case VK_MEDIA_NEXT_TRACK:
        return KeyCodeNextTrack;
    case VK_MEDIA_PREV_TRACK:
        return KeyCodePreviousTrack;
    case VK_MEDIA_STOP:
        return KeyCodeStopMedia;
    case VK_MEDIA_PLAY_PAUSE:
        return KeyCodePlayPause;
    case VK_LAUNCH_MAIL:
        return KeyCodeMail;
    case VK_LAUNCH_MEDIA_SELECT:
        return KeyCodeSelectMedia;
    case VK_LAUNCH_APP1:
        return KeyCodeApplication1;
    case VK_LAUNCH_APP2:
        return KeyCodeApplication2;
    case VK_PROCESSKEY:
        return KeyCodeIMEProcess;
    case VK_PACKET:
        return KeyCodeIMEInput;
    case VK_PLAY:
        return KeyCodePlay;
    case VK_ZOOM:
        return KeyCodeZoom;
    }
    return KeyCode::KeyCodeUndefined;
}

#endif
