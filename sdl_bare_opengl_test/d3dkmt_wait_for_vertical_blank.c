#include "d3dkmt_wait_for_veritcal_blank.h"
#include <Windows.h>

typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
	HDC                                         hDc;            // in:  DC that maps to a single display
	D3DKMTWFVB__D3DKMT_HANDLE                   hAdapter;       // out: adapter handle
	LUID                                        AdapterLuid;    // out: adapter LUID
	D3DKMTWFVB__D3DDDI_VIDEO_PRESENT_SOURCE_ID  VidPnSourceId;  // out: VidPN source ID for that particular display
} D3DKMT_OPENADAPTERFROMHDC;

typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_OPENADAPTERFROMHDC)(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);
typedef _Check_return_ NTSTATUS(APIENTRY *PFND3DKMT_WAITFORVERTICALBLANKEVENT)(_In_ CONST struct D3DKMTWFVB__D3DKMT_WAITFORVERTICALBLANKEVENT* df);

NTSTATUS APIENTRY D3DKMTOpenAdapterFromHdc(_Inout_ D3DKMT_OPENADAPTERFROMHDC*);
NTSTATUS APIENTRY D3DKMTWaitForVerticalBlankEvent(_In_ CONST struct D3DKMTWFVB__D3DKMT_WAITFORVERTICALBLANKEVENT* df);

extern NTSTATUS d3dkmt_wait_for_vertical_blank_init(struct D3DKMTWFVB_STATE *state)
{
	D3DKMT_OPENADAPTERFROMHDC oa;
	oa.hDc = GetDC(NULL);  // NULL = primary display monitor; NOT tested with multiple monitor setup; tested/works with hAppWnd
	NTSTATUS result = D3DKMTOpenAdapterFromHdc(&oa);
	if (state != NULL)
	{
		if (result == S_OK)
		{
			state->wait_event.hAdapter = oa.hAdapter;
			state->wait_event.hDevice = 0;
			state->wait_event.VidPnSourceId = oa.VidPnSourceId;
			state->good = TRUE;
		}
		else
		{
			state->good = FALSE;
		}
	}

	return result;
}

extern NTSTATUS d3dkmt_wait_for_vertical_blank(struct D3DKMTWFVB_STATE *state)
{
	if (state != NULL)
	{
		if (!state->good)
		{
			// Try to reinitialize it if it broke.
			NTSTATUS init_result = d3dkmt_wait_for_vertical_blank_init(state);
			if (init_result != S_OK)
			{
				return init_result;
			}
		}
	}
	NTSTATUS result = D3DKMTWaitForVerticalBlankEvent(&state->wait_event);
	if (result != S_OK && state != NULL)
	{
		state->good = FALSE;
	}
	return result;
}
