/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         EventTarget.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC


    OVERVIEW
    ========
    Event Target interface class
*/

#pragma once

#include "Types.h"
#include "Geometry.h"

// +--------------------------------------------------------------------+

class EventTarget
{
public:
    static const char* TYPENAME() { return "EventTarget"; }

    virtual ~EventTarget() {}

    int operator==(const EventTarget& t) const { return this == &t; }

    virtual bool OnMouseMove(int32 x, int32 y) { return false; }
    virtual int  OnLButtonDown(int x, int y) { return 0; }
    virtual int  OnLButtonUp(int x, int y) { return 0; }
    virtual int  OnClick() { return 0; }
    virtual int  OnSelect() { return 0; }
    virtual int  OnRButtonDown(int x, int y) { return 0; }
    virtual int  OnRButtonUp(int x, int y) { return 0; }
    virtual int  OnMouseEnter(int x, int y) { return 0; }
    virtual int  OnMouseExit(int x, int y) { return 0; }
    virtual int  OnMouseWheel(int wheel) { return 0; }

    virtual int  OnKeyDown(int vk, int flags) { return 0; }

    virtual void SetFocus() {}
    virtual void KillFocus() {}
    virtual bool HasFocus() const { return false; }

    virtual bool IsEnabled() const { return true; }
    virtual bool IsVisible() const { return true; }
    virtual bool IsFormActive() const { return true; }

    virtual Rect TargetRect() const { return Rect(); }

    virtual const char* GetDescription() const { return "EventTarget"; }
};
