#include <Windows.h>
#if 0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source unique identification number descriptor type
//

typedef UINT  D3DDDI_VIDEO_PRESENT_SOURCE_ID;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Purpose: Video present source unique identification number descriptor type.
//
typedef UINT  D3DDDI_VIDEO_PRESENT_TARGET_ID;

//
// DDI level handle that represents a kernel mode object (allocation, device, etc)
//
typedef UINT D3DKMT_HANDLE;

typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
	HDC                             hDc;            // in:  DC that maps to a single display
	D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
	LUID                            AdapterLuid;    // out: adapter LUID
	D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
} D3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT
{
	D3DKMT_HANDLE                   hAdapter;      // in: adapter handle
	D3DKMT_HANDLE                   hDevice;       // in: device handle [Optional]
	D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId; // in: adapter's VidPN Source ID
} D3DKMT_WAITFORVERTICALBLANKEVENT;

typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_OPENADAPTERFROMHDC)(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);
typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_WAITFORVERTICALBLANKEVENT)(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT*);

extern "C"
{
	NTSTATUS APIENTRY D3DKMTOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);
	NTSTATUS APIENTRY D3DKMTWaitForVerticalBlankEvent(_In_ CONST D3DKMT_WAITFORVERTICALBLANKEVENT*);
}
#endif
