/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         BaseScreen.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR AND STUDIO:
	John DiCamillo, Destroyer Studios LLC

*/

#pragma once

#include "Types.h"

// Forward declare Unreal render asset type (replacing Bitmap usage in interfaces):
class UTexture2D;

// +--------------------------------------------------------------------+

class Screen;
class Sim;
class Window;
class Font;
class NavDlg;
class MsnElemDlg;
class AudDlg;
class VidDlg;
class OptDlg;
class CtlDlg;
class KeyDlg;
class JoyDlg;
class MsgDlg;

// +--------------------------------------------------------------------+

class BaseUIScreen
{
public:
	BaseUIScreen() {}
	virtual ~BaseUIScreen() {}

	virtual void			ShowNavDlg() {}
	virtual void			HideNavDlg() {}
	virtual bool			IsNavShown() { return false; }
	virtual NavDlg*			GetNavDlg() { return 0; }

	virtual void			ShowMsnElemDlg() {}
	virtual void			HideMsnElemDlg() {}
	virtual MsnElemDlg*		GetMsnElemDlg() { return 0; }

	virtual AudDlg*			GetAudDlg() const { return 0; }
	virtual VidDlg*			GetVidDlg() const { return 0; }
	virtual OptDlg*			GetOptDlg() const { return 0; }
	virtual CtlDlg*			GetCtlDlg() const { return 0; }
	virtual JoyDlg*			GetJoyDlg() const { return 0; }
	virtual KeyDlg*			GetKeyDlg() const { return 0; }

	virtual void         ShowAudDlg() {}
	virtual void         ShowVidDlg() {}
	virtual void         ShowOptDlg() {}
	virtual void         ShowCtlDlg() {}
	virtual void         ShowJoyDlg() {}
	virtual void         ShowKeyDlg() {}

	virtual void         ShowMsgDlg() {}
	virtual void         HideMsgDlg() {}
	virtual bool         IsMsgShown() { return false; }
	virtual MsgDlg*		 GetMsgDlg() { return 0; }

	virtual void         ApplyOptions() {}
	virtual void         CancelOptions() {}
};