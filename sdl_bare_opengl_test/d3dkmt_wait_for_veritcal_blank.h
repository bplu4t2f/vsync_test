#pragma once

#include <Windows.h>

// These declarations are taken from the windows driver kit.
// We renamed them to avoid name conflicts, in case the real windows driver kit is currently installed and included.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source unique identification number descriptor type
//
typedef UINT  D3DKMTWFVB__D3DDDI_VIDEO_PRESENT_SOURCE_ID;

//
// DDI level handle that represents a kernel mode object (allocation, device, etc)
//
typedef UINT D3DKMTWFVB__D3DKMT_HANDLE;


struct D3DKMTWFVB__D3DKMT_WAITFORVERTICALBLANKEVENT
{
	D3DKMTWFVB__D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
	D3DKMTWFVB__D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
	D3DKMTWFVB__D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
};


struct D3DKMTWFVB_STATE
{
	struct D3DKMTWFVB__D3DKMT_WAITFORVERTICALBLANKEVENT wait_event;
	BOOL good;
};


typedef LONG NTSTATUS;

extern NTSTATUS d3dkmt_wait_for_vertical_blank_init(struct D3DKMTWFVB_STATE *state);
extern NTSTATUS d3dkmt_wait_for_vertical_blank(struct D3DKMTWFVB_STATE *state);
