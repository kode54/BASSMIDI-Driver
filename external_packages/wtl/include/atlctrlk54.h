#ifndef __ATLCTRLK54_H__
#define __ATLCTRLK54_H__

#pragma once

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error atlctrlk54.h requires atlapp.h to be included first
#endif

#ifndef __ATLWIN_H__
	#error atlctrlk54.h requires atlwin.h to be included first
#endif

#ifndef __ATLCTRLS_H__
	#error atlctrlk54.h requires atlctrls.h to be included first
#endif

/*#include <uxtheme.h>
#include <tmschema.h>
#pragma comment( lib, "uxtheme.lib" )
#pragma comment( lib, "delayimp.lib" )*/
// #pragma comment( linker, "/DELAYLOAD:uxtheme.dll" )

template <class TBase>
class CTrackBarCtrlXT : public CTrackBarCtrlT< TBase >
{
	HWND hWndParent;
	WNDPROC orig_wndproc;
	//UINT last_message, last_wp, last_lp;
	bool dragging;

	//HMODULE hUxTheme;

public:
	/*CTrackBarCtrlXT() : hUxTheme( 0 ) {}
	~CTrackBarCtrlXT()
	{
		if ( hUxTheme ) FreeLibrary( hUxTheme );
	}*/

	HWND Create(HWND hWndParent, ATL::_U_RECT rect = NULL, LPCTSTR szWindowName = NULL,
			DWORD dwStyle = 0, DWORD dwExStyle = 0,
			ATL::_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
	{
		HWND hWnd = CTrackBarCtrlT< TBase >::Create( hWndParent, rect.m_lpRect, szWindowName, dwStyle, dwExStyle, MenuOrID.m_hMenu, lpCreateParam );
		if ( hWnd )
		{
			this->hWndParent = hWndParent;
			/*last_message = 0;
			last_wp = 0;
			last_lp = 0;*/
			dragging = false;
			//hUxTheme = LoadLibrary( _T( "uxtheme" ) );
			SetProp( hWnd, _T( "trackbar_hook_data" ), this );
			orig_wndproc = ( WNDPROC ) ::SetWindowLong( hWnd, GWL_WNDPROC, ( LPARAM ) & CTrackBarCtrlXT< TBase >::g_hook_proc );
		}
		return hWnd;
	}

private:
	static LRESULT WINAPI g_hook_proc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		CTrackBarCtrlXT< TBase > * p_this = reinterpret_cast< CTrackBarCtrlXT< TBase > * > ( GetProp( hWnd, _T( "trackbar_hook_data" ) ) );
		if ( p_this )
		{
			return p_this->hook_proc( hWnd, uMsg, wParam, lParam );
		}
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

#if 0
	void calc_track_rects( HWND hWnd, RECT & rcChannel, RECT & rcThumb )
	{
		CallWindowProc( orig_wndproc, hWnd, TBM_GETCHANNELRECT, 0, ( LPARAM ) & rcChannel );
		CallWindowProc( orig_wndproc, hWnd, TBM_GETTHUMBRECT, 0, ( LPARAM ) & rcThumb );

		if ( hUxTheme )
		{
			if ( IsAppThemed() )
			{
				HTHEME hTheme = GetWindowTheme( hWnd );
				if ( hTheme && 
					IsThemePartDefined( hTheme, TKP_TRACK, TRS_NORMAL ) &&
					IsThemePartDefined( hTheme, TKP_THUMB, TUS_NORMAL ) )
				{
					POINT ptChannel = { rcChannel.left, rcChannel.top };
					POINT ptThumb = { rcThumb.left, rcThumb.top };
					if ( GetThemeRect( hTheme, TKP_TRACK, TRS_NORMAL, TMT_DEFAULTPANESIZE, & rcChannel ) == S_OK &&
						GetThemeRect( hTheme, TKP_THUMB, TUS_NORMAL, TMT_DEFAULTPANESIZE, & rcThumb ) == S_OK )
					{
						OffsetRect( & rcChannel, ptChannel.x, ptChannel.y );
						OffsetRect( & rcThumb, ptThumb.x, ptThumb.y );
					}
				}
			}
		}
	}

	void update_position( HWND hWnd, int x /*, bool check = false */ )
	{
		RECT rcChannel, rcThumb;
		calc_track_rects( hWnd, rcChannel, rcThumb );
		int temp = ( rcThumb.right - rcThumb.left ) / 2;
		rcChannel.left += temp;
		rcChannel.right -= temp;

		if ( x >= rcChannel.right ) x = rcChannel.right - 1;
		else if ( x < rcChannel.left ) x = rcChannel.left;

		int range_min = CallWindowProc( orig_wndproc, hWnd, TBM_GETRANGEMIN, 0, 0 );
		temp = MulDiv( x - rcChannel.left, CallWindowProc( orig_wndproc, hWnd, TBM_GETRANGEMAX, 0, 0 ) - range_min, rcChannel.right - rcChannel.left ) + range_min;
		/* if ( ! check ) */ CallWindowProc( orig_wndproc, hWnd, TBM_SETPOS, 1, temp );
		/* else if ( temp != CallWindowProc( orig_wndproc, hWnd, TBM_GETPOS, 0, 0 ) ) CallWindowProc( orig_wndproc, hWnd, TBM_SETPOS, 1, temp ); */
	}
#endif

public:
	LRESULT hook_proc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		switch ( uMsg )
		{
		/*case WM_MOUSEMOVE:
			if ( dragging )
			{
				if ( last_message == WM_MOUSEMOVE )
				{
					if ( ( ( UINT ) wParam == last_wp ) && ( ( UINT ) lParam == last_lp ) ) break;
				}
				last_wp = ( UINT ) wParam;
				last_lp = ( UINT ) lParam;
				last_message = WM_MOUSEMOVE;

				if ( ::IsWindowEnabled( hWnd ) )
				{
					update_position( hWnd, GET_X_LPARAM( lParam ), true );
				}
				return 0;
			}
			break;*/

		case WM_LBUTTONDOWN:
			//last_message = WM_LBUTTONDOWN;
			if ( ::IsWindowEnabled( hWnd ) )
			{
				POINT pt = { GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) };
				RECT rc_client;
				::GetClientRect( hWnd, & rc_client );
				if ( PtInRect( & rc_client, pt ) )
				{
					//update_position( hWnd, pt.x );

					dragging = true;

					// YAY, this works! no more prediction bs
					RECT rcThumb;
					CallWindowProc( orig_wndproc, hWnd, TBM_GETTHUMBRECT, 0, ( LPARAM ) & rcThumb );
					CallWindowProc( orig_wndproc, hWnd, WM_LBUTTONDOWN, wParam, MAKELONG( ( rcThumb.left + rcThumb.right ) / 2, ( rcThumb.top + rcThumb.bottom ) / 2 ) );
					return CallWindowProc( orig_wndproc, hWnd, WM_MOUSEMOVE, wParam, lParam );

					/*::SetCapture( hWnd );
					::SetFocus( hWndParent );*/

					//::SendMessage( hWndParent, WM_HSCROLL, MAKELONG( TB_THUMBTRACK, temp ), ( LPARAM ) hWnd );
				}
			}
			//return 0;
			break;

		case WM_LBUTTONUP:
			if (dragging)
			{
				/*::SendMessage( hWndParent, WM_HSCROLL, MAKELONG( TB_THUMBPOSITION, CallWindowProc( orig_wndproc, hWnd, TBM_GETPOS, 0, 0) ), ( LPARAM ) hWnd );
				::SendMessage( hWndParent, WM_HSCROLL, MAKELONG( TB_ENDTRACK, 0 ), ( LPARAM ) hWnd );*/
				if ( GetCapture() == hWnd ) ReleaseCapture();
				dragging = false;
				//last_message = WM_LBUTTONUP;
				::SetFocus( hWndParent );
			}
			return 0;
			break;

		case WM_MBUTTONDOWN:
			return 0;
			break;

		case WM_DESTROY:
			RemoveProp( hWnd, _T( "trackbar_hook_data" ) );
			break;
		}

		return CallWindowProc( orig_wndproc, hWnd, uMsg, wParam, lParam );
	}
};

typedef CTrackBarCtrlXT<ATL::CWindow>   CTrackBarCtrlX;

#endif