/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
** WIN_GLIMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_LogComment
** GLimp_Shutdown
**
** Note that the GLW_xxx functions are Windows specific GL-subsystem
** related functions that are relevant ONLY to win_glimp.c
*/


#include <assert.h>
#include "tr_local.h"
#include "qcommon.h"

//BBi
#if defined RTCW_SP
    #include "rtcw_sp_resource.h"
#elif defined RTCW_MP
    #include "rtcw_mp_resource.h"
#elif defined RTCW_ET
    #include "rtcw_et_resource.h"
#endif // RTCW_XX
//BBi

#include "glw_win.h"
#include "win_local.h"

//BBi
#include <GL/wglew.h>
//BBi


extern void WG_CheckHardwareGamma( void );
extern void WG_RestoreGamma( void );

typedef enum {
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
} rserr_t;

#define TRY_PFD_SUCCESS     0
#define TRY_PFD_FAIL_SOFT   1
#define TRY_PFD_FAIL_HARD   2

#if !defined RTCW_ET
#define WINDOW_CLASS_NAME   "Wolfenstein"
#else
//#define	WINDOW_CLASS_NAME	"Wolfenstein"
#define WINDOW_CLASS_NAME   "Enemy Territory"
#endif // RTCW_XX

static void     GLW_InitExtensions( void );

//BBi
//static rserr_t  GLW_SetMode( const char *drivername,
//							 int mode,
//							 int colorbits,
//							 qboolean cdsFullscreen );
static rserr_t  GLW_SetMode (
    int mode,
    bool isFullscreen);
//BBi

//BBi
//static qboolean s_classRegistered = qfalse;
static bool s_classRegistered = false;
//BBi

//
// function declaration
//

//BBi
//void     QGL_EnableLogging( qboolean enable );
//BBi

qboolean QGL_Init( const char *dllname );
void     QGL_Shutdown( void );

//
// variable declarations
//
glwstate_t glw_state;

cvar_t  *r_allowSoftwareGL;     // don't abort out if the pixelformat claims software
cvar_t  *r_maskMinidriver;      // allow a different dll name to be treated as if it were opengl32.dll

#if defined RTCW_ET
int gl_NormalFontBase = 0;
static qboolean fontbase_init = qfalse;

//BBi
///*
//* Find the first occurrence of find in s.
//*/
//// bk001130 - from cvs1.17 (mkv), const
//// bk001130 - made first argument const
//// fretn - comes from linux_glimp.c msvc really needs this (well ... mine does \o/ )
//static const char *Q_stristr( const char *s, const char *find ) {
//	register char c, sc;
//	register size_t len;
//
//	if ( ( c = *find++ ) != 0 ) {
//		if ( c >= 'a' && c <= 'z' ) {
//			c -= ( 'a' - 'A' );
//		}
//		len = strlen( find );
//		do
//		{
//			do
//			{
//				if ( ( sc = *s++ ) == 0 ) {
//					return NULL;
//				}
//				if ( sc >= 'a' && sc <= 'z' ) {
//					sc -= ( 'a' - 'A' );
//				}
//			} while ( sc != c );
//		} while ( Q_stricmpn( s, find, len ) != 0 );
//		s--;
//	}
//	return s;
//}
//BBi
#endif // RTCW_XX

/*
** GLW_StartDriverAndSetMode
*/

//BBi
//static qboolean GLW_StartDriverAndSetMode( const char *drivername,
//										   int mode,
//										   int colorbits,
//										   qboolean cdsFullscreen ) {
//	rserr_t err;
//
//	err = GLW_SetMode( drivername, r_mode->integer, colorbits, cdsFullscreen );
//
//	switch ( err )
//	{
//	case RSERR_INVALID_FULLSCREEN:
//		ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
//		return qfalse;
//	case RSERR_INVALID_MODE:
//		ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
//		return qfalse;
//	default:
//		break;
//	}
//	return qtrue;
//}

static bool GLW_StartDriverAndSetMode (
    int mode,
    bool isFullscreen)
{
    rserr_t err = ::GLW_SetMode (r_mode->integer, isFullscreen);

    switch (err) {
    case RSERR_INVALID_FULLSCREEN:
        ::ri.Printf (PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n");
        return false;

    case RSERR_INVALID_MODE:
        ::ri.Printf (PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode);
        return false;
    }


    GLenum glewResult = ::glewInit ();

    if (glewResult != GLEW_OK) {
        ::ri.Printf (PRINT_ALL, "...WARNING: failed to initialized GLEW (%s)\n",
            ::glewGetErrorString (glewResult));
        return false;
    }

    return true;
}
//BBi

/*
** ChoosePFD
**
** Helper function that replaces ChoosePixelFormat.
*/

//BBi
//#define MAX_PFDS 256
//
//static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD ) {
//	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS + 1];
//	int maxPFD = 0;
//	int i;
//	int bestMatch = 0;
//
//	ri.Printf( PRINT_ALL, "...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits );
//
//	// count number of PFDs
//	if ( glConfig.driverType > GLDRV_ICD ) {
//		maxPFD = qwglDescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );
//	} else
//	{
//		maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );
//	}
//	if ( maxPFD > MAX_PFDS ) {
//		ri.Printf( PRINT_WARNING, "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS );
//		maxPFD = MAX_PFDS;
//	}
//
//	ri.Printf( PRINT_ALL, "...%d PFDs found\n", maxPFD - 1 );
//
//	// grab information
//	for ( i = 1; i <= maxPFD; i++ )
//	{
//		if ( glConfig.driverType > GLDRV_ICD ) {
//			qwglDescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );
//		} else
//		{
//			DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );
//		}
//	}
//
//	// look for a best match
//	for ( i = 1; i <= maxPFD; i++ )
//	{
//		//
//		// make sure this has hardware acceleration
//		//
//		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
//			if ( !r_allowSoftwareGL->integer ) {
//				if ( r_verbose->integer ) {
//					ri.Printf( PRINT_ALL, "...PFD %d rejected, software acceleration\n", i );
//				}
//				continue;
//			}
//		}
//
//		// verify pixel type
//		if ( pfds[i].iPixelType != PFD_TYPE_RGBA ) {
//			if ( r_verbose->integer ) {
//				ri.Printf( PRINT_ALL, "...PFD %d rejected, not RGBA\n", i );
//			}
//			continue;
//		}
//
//		// verify proper flags
//		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) {
//			if ( r_verbose->integer ) {
//				ri.Printf( PRINT_ALL, "...PFD %d rejected, improper flags (%x instead of %x)\n", i, pfds[i].dwFlags, pPFD->dwFlags );
//			}
//			continue;
//		}
//
//		// verify enough bits
//		if ( pfds[i].cDepthBits < 15 ) {
//			continue;
//		}
//		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) ) {
//			continue;
//		}
//
//		//
//		// selection criteria (in order of priority):
//		//
//		//  PFD_STEREO
//		//  colorBits
//		//  depthBits
//		//  stencilBits
//		//
//		if ( bestMatch ) {
//			// check stereo
//			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) ) {
//				bestMatch = i;
//				continue;
//			}
//
//			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) ) {
//				bestMatch = i;
//				continue;
//			}
//
//			// check color
//			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits ) {
//				// prefer perfect match
//				if ( pfds[i].cColorBits == pPFD->cColorBits ) {
//					bestMatch = i;
//					continue;
//				}
//				// otherwise if this PFD has more bits than our best, use it
//				else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits ) {
//					bestMatch = i;
//					continue;
//				}
//			}
//
//			// check depth
//			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits ) {
//				// prefer perfect match
//				if ( pfds[i].cDepthBits == pPFD->cDepthBits ) {
//					bestMatch = i;
//					continue;
//				}
//				// otherwise if this PFD has more bits than our best, use it
//				else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits ) {
//					bestMatch = i;
//					continue;
//				}
//			}
//
//			// check stencil
//			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits ) {
//				// prefer perfect match
//				if ( pfds[i].cStencilBits == pPFD->cStencilBits ) {
//					bestMatch = i;
//					continue;
//				}
//				// otherwise if this PFD has more bits than our best, use it
//				else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) &&
//						  ( pPFD->cStencilBits > 0 ) ) {
//					bestMatch = i;
//					continue;
//				}
//			}
//		} else
//		{
//			bestMatch = i;
//		}
//	}
//
//	if ( !bestMatch ) {
//		return 0;
//	}
//
//	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) {
//		if ( !r_allowSoftwareGL->integer ) {
//			ri.Printf( PRINT_ALL, "...no hardware acceleration found\n" );
//			return 0;
//		} else
//		{
//			ri.Printf( PRINT_ALL, "...using software emulation\n" );
//		}
//	} else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )   {
//		ri.Printf( PRINT_ALL, "...MCD acceleration found\n" );
//	} else
//	{
//		ri.Printf( PRINT_ALL, "...hardware acceleration found\n" );
//	}
//
//	*pPFD = pfds[bestMatch];
//
//	return bestMatch;
//}

static int GLW_ChoosePFD (
    HDC hDC,
    PIXELFORMATDESCRIPTOR* pPFD)
{
    ::ri.Printf (PRINT_ALL, "...choosing pixel format (%d, %d, %d)\n",
        static_cast<int> (pPFD->cColorBits),
        static_cast<int> (pPFD->cDepthBits),
        static_cast<int> (pPFD->cStencilBits));

    int pixelFormatIndex = ::ChoosePixelFormat (hDC, pPFD);

    if (pixelFormatIndex == 0) {
        ::ri.Printf (PRINT_ALL, "failed\n");
        return 0;
    }

    PIXELFORMATDESCRIPTOR newPfd;
    ::DescribePixelFormat (
        hDC,
        pixelFormatIndex,
        sizeof (PIXELFORMATDESCRIPTOR),
        &newPfd);

    DWORD oldFlags = pPFD->dwFlags & (~PFD_STEREO);
    DWORD newFlags = newPfd.dwFlags;

    if ((newFlags & oldFlags) != oldFlags) {
        ::ri.Printf (PRINT_ALL, "mismatch flags\n");
        return 0;
    }

    if (newPfd.cColorBits != pPFD->cColorBits) {
        ::ri.Printf (PRINT_ALL, "mismatch color bits\n");
        return 0;
    }

    if (newPfd.cDepthBits != pPFD->cDepthBits) {
        ::ri.Printf (PRINT_ALL, "mismatch depth bits\n");
        return 0;
    }

    if (newPfd.cStencilBits != pPFD->cStencilBits) {
        ::ri.Printf (PRINT_ALL, "mismatch stencil bits\n");
        return 0;
    }

    return pixelFormatIndex;
}
//BBi

/*
** void GLW_CreatePFD
**
** Helper function zeros out then fills in a PFD
*/

//BBi
//static void GLW_CreatePFD( PIXELFORMATDESCRIPTOR *pPFD, int colorbits, int depthbits, int stencilbits, qboolean stereo ) {
//	PIXELFORMATDESCRIPTOR src =
//	{
//		sizeof( PIXELFORMATDESCRIPTOR ),  // size of this pfd
//		1,                              // version number
//		PFD_DRAW_TO_WINDOW |            // support window
//		PFD_SUPPORT_OPENGL |            // support OpenGL
//		PFD_DOUBLEBUFFER,               // double buffered
//		PFD_TYPE_RGBA,                  // RGBA type
//		24,                             // 24-bit color depth
//		0, 0, 0, 0, 0, 0,               // color bits ignored
//		0,                              // no alpha buffer
//		0,                              // shift bit ignored
//		0,                              // no accumulation buffer
//		0, 0, 0, 0,                     // accum bits ignored
//		24,                             // 24-bit z-buffer
//		8,                              // 8-bit stencil buffer
//		0,                              // no auxiliary buffer
//		PFD_MAIN_PLANE,                 // main layer
//		0,                              // reserved
//		0, 0, 0                         // layer masks ignored
//	};
//
//	src.cColorBits = colorbits;
//	src.cDepthBits = depthbits;
//	src.cStencilBits = stencilbits;
//
//	if ( stereo ) {
//		ri.Printf( PRINT_ALL, "...attempting to use stereo\n" );
//		src.dwFlags |= PFD_STEREO;
//		glConfig.stereoEnabled = qtrue;
//	} else
//	{
//		glConfig.stereoEnabled = qfalse;
//	}
//
//	*pPFD = src;
//}

static void GLW_CreatePFD (
    PIXELFORMATDESCRIPTOR* pPFD,
    bool isStereo)
{
    PIXELFORMATDESCRIPTOR result;
    ::memset (&result, 0, sizeof (PIXELFORMATDESCRIPTOR));
    result.nSize = sizeof (PIXELFORMATDESCRIPTOR);
    result.nVersion = 1;
    result.dwFlags =
        PFD_DRAW_TO_WINDOW |
        PFD_SUPPORT_OPENGL |
        PFD_DOUBLEBUFFER;
    result.iPixelType = PFD_TYPE_RGBA;
    result.cColorBits = 32;
    result.cDepthBits = 24;
    result.cStencilBits = 8;

    if (isStereo) {
        ::ri.Printf (PRINT_ALL, "...attempting to use stereo\n");
        result.dwFlags |= PFD_STEREO;
        glConfig.stereoEnabled = true;
    } else
        glConfig.stereoEnabled = false;

    *pPFD = result;
}
//BBi

/*
** GLW_MakeContext
*/

//BBi
//static int GLW_MakeContext( PIXELFORMATDESCRIPTOR *pPFD ) {
//	int pixelformat;
//
//	//
//	// don't putz around with pixelformat if it's already set (e.g. this is a soft
//	// reset of the graphics system)
//	//
//	if ( !glw_state.pixelFormatSet ) {
//		//
//		// choose, set, and describe our desired pixel format.  If we're
//		// using a minidriver then we need to bypass the GDI functions,
//		// otherwise use the GDI functions.
//		//
//		if ( ( pixelformat = GLW_ChoosePFD( glw_state.hDC, pPFD ) ) == 0 ) {
//			ri.Printf( PRINT_ALL, "...GLW_ChoosePFD failed\n" );
//			return TRY_PFD_FAIL_SOFT;
//		}
//		ri.Printf( PRINT_ALL, "...PIXELFORMAT %d selected\n", pixelformat );
//
//		if ( glConfig.driverType > GLDRV_ICD ) {
//			qwglDescribePixelFormat( glw_state.hDC, pixelformat, sizeof( *pPFD ), pPFD );
//			if ( qwglSetPixelFormat( glw_state.hDC, pixelformat, pPFD ) == FALSE ) {
//				ri.Printf( PRINT_ALL, "...qwglSetPixelFormat failed\n" );
//				return TRY_PFD_FAIL_SOFT;
//			}
//		} else
//		{
//			DescribePixelFormat( glw_state.hDC, pixelformat, sizeof( *pPFD ), pPFD );
//
//			if ( SetPixelFormat( glw_state.hDC, pixelformat, pPFD ) == FALSE ) {
//				ri.Printf( PRINT_ALL, "...SetPixelFormat failed\n", glw_state.hDC );
//				return TRY_PFD_FAIL_SOFT;
//			}
//		}
//
//		glw_state.pixelFormatSet = qtrue;
//	}
//
//	//
//	// startup the OpenGL subsystem by creating a context and making it current
//	//
//	if ( !glw_state.hGLRC ) {
//		ri.Printf( PRINT_ALL, "...creating GL context: " );
//		if ( ( glw_state.hGLRC = qwglCreateContext( glw_state.hDC ) ) == 0 ) {
//			ri.Printf( PRINT_ALL, "failed\n" );
//
//			return TRY_PFD_FAIL_HARD;
//		}
//		ri.Printf( PRINT_ALL, "succeeded\n" );
//
//		ri.Printf( PRINT_ALL, "...making context current: " );
//		if ( !qwglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
//			qwglDeleteContext( glw_state.hGLRC );
//			glw_state.hGLRC = NULL;
//			ri.Printf( PRINT_ALL, "failed\n" );
//			return TRY_PFD_FAIL_HARD;
//		}
//		ri.Printf( PRINT_ALL, "succeeded\n" );
//	}
//
//	return TRY_PFD_SUCCESS;
//}

static int GLW_MakeContext (
    PIXELFORMATDESCRIPTOR* pPFD)
{
    int pixelFormat;

    if (!glw_state.pixelFormatSet) {
        pixelFormat = ::GLW_ChoosePFD (glw_state.hDC, pPFD);

        if (pixelFormat == 0) {
            ::ri.Printf (PRINT_ALL, "...GLW_ChoosePFD failed\n");
            return TRY_PFD_FAIL_SOFT;
        }

        ::ri.Printf (PRINT_ALL, "...PIXELFORMAT %d selected\n", pixelFormat);

        ::DescribePixelFormat (glw_state.hDC, pixelFormat, sizeof (*pPFD), pPFD);

        if (::SetPixelFormat (glw_state.hDC, pixelFormat, pPFD) == FALSE) {
            ::ri.Printf (PRINT_ALL, "...SetPixelFormat failed\n", glw_state.hDC);
            return TRY_PFD_FAIL_SOFT;
        }

        glw_state.pixelFormatSet = qtrue;
    }

    //
    // startup the OpenGL subsystem by creating a context and making it current
    //
    if (glw_state.hGLRC == 0) {
        ::ri.Printf (PRINT_ALL, "...creating GL context: ");

        glw_state.hGLRC = ::wglCreateContext (glw_state.hDC);

        if (glw_state.hGLRC == 0) {
            ::ri.Printf (PRINT_ALL, "failed\n");
            return TRY_PFD_FAIL_HARD;
        }

        ::ri.Printf (PRINT_ALL, "succeeded\n");
        ::ri.Printf (PRINT_ALL, "...making context current: ");

        if (::wglMakeCurrent (glw_state.hDC, glw_state.hGLRC) == FALSE) {
            ::wglDeleteContext (glw_state.hGLRC);
            glw_state.hGLRC = 0;
            ::ri.Printf (PRINT_ALL, "failed\n");
            return TRY_PFD_FAIL_HARD;
        }

        ::ri.Printf (PRINT_ALL, "succeeded\n");
    }

    return TRY_PFD_SUCCESS;
}
//BBi

/*
** GLW_InitDriver
**
** - get a DC if one doesn't exist
** - create an HGLRC if one doesn't exist
*/

//BBi
//static qboolean GLW_InitDriver( const char *drivername, int colorbits ) {
//	int tpfd;
//	int depthbits, stencilbits;
//	static PIXELFORMATDESCRIPTOR pfd;       // save between frames since 'tr' gets cleared
//
//	ri.Printf( PRINT_ALL, "Initializing OpenGL driver\n" );
//
//	//
//	// get a DC for our window if we don't already have one allocated
//	//
//	if ( glw_state.hDC == NULL ) {
//		ri.Printf( PRINT_ALL, "...getting DC: " );
//
//		if ( ( glw_state.hDC = GetDC( g_wv.hWnd ) ) == NULL ) {
//			ri.Printf( PRINT_ALL, "failed\n" );
//			return qfalse;
//		}
//		ri.Printf( PRINT_ALL, "succeeded\n" );
//	}
//
//	if ( colorbits == 0 ) {
//		colorbits = glw_state.desktopBitsPixel;
//	}
//
//	//
//	// implicitly assume Z-buffer depth == desktop color depth
//	//
//	if ( r_depthbits->integer == 0 ) {
//		if ( colorbits > 16 ) {
//			depthbits = 24;
//		} else {
//			depthbits = 16;
//		}
//	} else {
//		depthbits = r_depthbits->integer;
//	}
//
//	//
//	// do not allow stencil if Z-buffer depth likely won't contain it
//	//
//	stencilbits = r_stencilbits->integer;
//	if ( depthbits < 24 ) {
//		stencilbits = 0;
//	}
//
//	//
//	// make two attempts to set the PIXELFORMAT
//	//
//
//	//
//	// first attempt: r_colorbits, depthbits, and r_stencilbits
//	//
//	if ( !glw_state.pixelFormatSet ) {
//		GLW_CreatePFD( &pfd, colorbits, depthbits, stencilbits, r_stereo->integer );
//		if ( ( tpfd = GLW_MakeContext( &pfd ) ) != TRY_PFD_SUCCESS ) {
//			if ( tpfd == TRY_PFD_FAIL_HARD ) {
//				ri.Printf( PRINT_WARNING, "...failed hard\n" );
//				return qfalse;
//			}
//
//			//
//			// punt if we've already tried the desktop bit depth and no stencil bits
//			//
//			if ( ( r_colorbits->integer == glw_state.desktopBitsPixel ) &&
//				 ( stencilbits == 0 ) ) {
//				ReleaseDC( g_wv.hWnd, glw_state.hDC );
//				glw_state.hDC = NULL;
//
//				ri.Printf( PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );
//
//				return qfalse;
//			}
//
//			//
//			// second attempt: desktop's color bits and no stencil
//			//
//			if ( colorbits > glw_state.desktopBitsPixel ) {
//				colorbits = glw_state.desktopBitsPixel;
//			}
//			GLW_CreatePFD( &pfd, colorbits, depthbits, 0, r_stereo->integer );
//			if ( GLW_MakeContext( &pfd ) != TRY_PFD_SUCCESS ) {
//				if ( glw_state.hDC ) {
//					ReleaseDC( g_wv.hWnd, glw_state.hDC );
//					glw_state.hDC = NULL;
//				}
//
//				ri.Printf( PRINT_ALL, "...failed to find an appropriate PIXELFORMAT\n" );
//
//				return qfalse;
//			}
//		}
//
//		/*
//		** report if stereo is desired but unavailable
//		*/
//		if ( !( pfd.dwFlags & PFD_STEREO ) && ( r_stereo->integer != 0 ) ) {
//			ri.Printf( PRINT_ALL, "...failed to select stereo pixel format\n" );
//			glConfig.stereoEnabled = qfalse;
//		}
//	}
//
//	/*
//	** store PFD specifics
//	*/
//	glConfig.colorBits = ( int ) pfd.cColorBits;
//	glConfig.depthBits = ( int ) pfd.cDepthBits;
//	glConfig.stencilBits = ( int ) pfd.cStencilBits;
//
//	return qtrue;
//}

static bool GLW_InitDriver ()
{
    ::ri.Printf (PRINT_ALL, "Initializing OpenGL driver\n");


    int colorBits = r_colorbits->integer;

    if ((colorBits != 0) && (colorBits != 32)) {
        ::ri.Printf (PRINT_WARNING, "...invalid/deprecated color bits: %i\n", colorBits);
        ::ri.Printf (PRINT_WARNING, "...using 32 color bits\n");
    }

    colorBits = 32;


    //
    // get a DC for our window if we don't already have one allocated
    //
    if (glw_state.hDC == 0) {
        ::ri.Printf (PRINT_ALL, "...getting DC: ");

        glw_state.hDC = ::GetDC (g_wv.hWnd);

        if (glw_state.hDC == 0) {
            ::ri.Printf (PRINT_ALL, "failed\n");
            return false;
        }

        ri.Printf (PRINT_ALL, "succeeded\n");
    }


    int depthBits = r_depthbits->integer;

    if ((depthBits != 0) && (depthBits != 24)) {
        ::ri.Printf (PRINT_WARNING, "...invalid/deprecated depth bits: %i\n", depthBits);
        ::ri.Printf (PRINT_WARNING, "...using 24 depth bits\n");
    }

    depthBits = 24;


    int stencilBits = r_stencilbits->integer;

    if ((stencilBits != 0) && (stencilBits != 8)) {
        ::ri.Printf (PRINT_WARNING, "...invalid/deprecated stencil bits: %i\n", stencilBits);
        ::ri.Printf (PRINT_WARNING, "...using 8 stencil bits\n");
    }

    stencilBits = 8;


    static PIXELFORMATDESCRIPTOR pfd; // save between frames since 'tr' gets cleared

    if (!glw_state.pixelFormatSet) {
        ::GLW_CreatePFD (&pfd, (r_stereo->integer != 0));

        int tpfd = ::GLW_MakeContext (&pfd);

        if (tpfd != TRY_PFD_SUCCESS ) {
            ::ri.Printf (PRINT_WARNING, "...failed hard\n");
            return false;
        }

        //
        // report if stereo is desired but unavailable
        //
        if ((r_stereo->integer != 0) && ((pfd.dwFlags & PFD_STEREO) == 0)) {
            ::ri.Printf (PRINT_ALL, "...failed to select stereo pixel format\n");
            glConfig.stereoEnabled = qfalse;
        }
    }

    //
    // store PFD specifics
    //
    glConfig.colorBits = pfd.cColorBits;
    glConfig.depthBits = pfd.cDepthBits;
    glConfig.stencilBits = pfd.cStencilBits;

    return true;
}
//BBi

/*
** GLW_CreateWindow
**
** Responsible for creating the Win32 window and initializing the OpenGL driver.
*/

//BBi
//#define WINDOW_STYLE    ( WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_VISIBLE )
//static qboolean GLW_CreateWindow( const char *drivername, int width, int height, int colorbits, qboolean cdsFullscreen ) {
//	RECT r;
//	cvar_t          *vid_xpos, *vid_ypos;
//	int stylebits;
//	int x, y, w, h;
//	int exstyle;
//
//	//
//	// register the window class if necessary
//	//
//	if ( !s_classRegistered ) {
//		WNDCLASS wc;
//
//		memset( &wc, 0, sizeof( wc ) );
//
//		wc.style         = 0;
//		wc.lpfnWndProc   = (WNDPROC) glw_state.wndproc;
//		wc.cbClsExtra    = 0;
//		wc.cbWndExtra    = 0;
//		wc.hInstance     = g_wv.hInstance;
//		wc.hIcon         = LoadIcon( g_wv.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
//		wc.hCursor       = LoadCursor( NULL,IDC_ARROW );
//		wc.hbrBackground = reinterpret_cast<HBRUSH> (COLOR_GRAYTEXT);
//		wc.lpszMenuName  = 0;
//		wc.lpszClassName = WINDOW_CLASS_NAME;
//
//		if ( !RegisterClass( &wc ) ) {
//
//#if !defined RTCW_ET
//			ri.Error( ERR_FATAL, "GLW_CreateWindow: could not register window class" );
//#else
//			ri.Error( ERR_VID_FATAL, "GLW_CreateWindow: could not register window class" );
//#endif // RTCW_XX
//
//		}
//		s_classRegistered = qtrue;
//		ri.Printf( PRINT_ALL, "...registered window class\n" );
//	}
//
//	//
//	// create the HWND if one does not already exist
//	//
//	if ( !g_wv.hWnd ) {
//		//
//		// compute width and height
//		//
//		r.left = 0;
//		r.top = 0;
//		r.right  = width;
//		r.bottom = height;
//
//		if ( cdsFullscreen || !Q_stricmp( _3DFX_DRIVER_NAME, drivername ) ) {
//			exstyle = WS_EX_TOPMOST;
//			stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
//		} else
//		{
//			exstyle = 0;
//			stylebits = WINDOW_STYLE | WS_SYSMENU;
//			AdjustWindowRect( &r, stylebits, FALSE );
//		}
//
//		w = r.right - r.left;
//		h = r.bottom - r.top;
//
//		if ( cdsFullscreen || !Q_stricmp( _3DFX_DRIVER_NAME, drivername ) ) {
//			x = 0;
//			y = 0;
//		} else
//		{
//			vid_xpos = ri.Cvar_Get( "vid_xpos", "", 0 );
//			vid_ypos = ri.Cvar_Get( "vid_ypos", "", 0 );
//			x = vid_xpos->integer;
//			y = vid_ypos->integer;
//
//			// adjust window coordinates if necessary
//			// so that the window is completely on screen
//			if ( x < 0 ) {
//				x = 0;
//			}
//			if ( y < 0 ) {
//				y = 0;
//			}
//
//			if ( w < glw_state.desktopWidth &&
//				 h < glw_state.desktopHeight ) {
//				if ( x + w > glw_state.desktopWidth ) {
//					x = ( glw_state.desktopWidth - w );
//				}
//				if ( y + h > glw_state.desktopHeight ) {
//					y = ( glw_state.desktopHeight - h );
//				}
//			}
//		}
//
//		g_wv.hWnd = CreateWindowEx(
//			exstyle,
//			WINDOW_CLASS_NAME,
//
//#if !defined RTCW_ET
//			"Wolfenstein",
//#else
//			//"Wolfenstein",
//			"Enemy Territory",
//#endif // RTCW_XX
//
//			stylebits,
//			x, y, w, h,
//			NULL,
//			NULL,
//			g_wv.hInstance,
//			NULL );
//
//		if ( !g_wv.hWnd ) {
//
//#if !defined RTCW_ET
//			ri.Error( ERR_FATAL, "GLW_CreateWindow() - Couldn't create window" );
//#else
//			ri.Error( ERR_VID_FATAL, "GLW_CreateWindow() - Couldn't create window" );
//#endif // RTCW_XX
//
//		}
//
//		ShowWindow( g_wv.hWnd, SW_SHOW );
//		UpdateWindow( g_wv.hWnd );
//		ri.Printf( PRINT_ALL, "...created window@%d,%d (%dx%d)\n", x, y, w, h );
//	} else
//	{
//		ri.Printf( PRINT_ALL, "...window already present, CreateWindowEx skipped\n" );
//	}
//
//	if ( !GLW_InitDriver( drivername, colorbits ) ) {
//		ShowWindow( g_wv.hWnd, SW_HIDE );
//		DestroyWindow( g_wv.hWnd );
//		g_wv.hWnd = NULL;
//
//		return qfalse;
//	}
//
//	SetForegroundWindow( g_wv.hWnd );
//	SetFocus( g_wv.hWnd );
//
//	return qtrue;
//}

static const UINT WINDOW_STYLE = WS_OVERLAPPED | WS_BORDER | WS_CAPTION;

static bool GLW_CreateWindow (
    int width,
    int height,
    bool isFullscreen)
{
    RECT r;
    cvar_t* vid_xpos;
    cvar_t* vid_ypos;
    int stylebits;
    int x, y, w, h;
    int exstyle;

    //
    // register the window class if necessary
    //
    if (!s_classRegistered ) {
        WNDCLASS wc;

        ::memset (&wc, 0, sizeof (wc));

        wc.style         = 0;
        wc.lpfnWndProc   = glw_state.wndproc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = g_wv.hInstance;
        wc.hIcon         = ::LoadIcon (g_wv.hInstance, MAKEINTRESOURCE (IDI_ICON1));
        wc.hCursor       = ::LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH> (COLOR_GRAYTEXT);
        wc.lpszMenuName  = 0;
        wc.lpszClassName = WINDOW_CLASS_NAME;

        if (!::RegisterClass (&wc)) {

#if !defined RTCW_ET
            ::ri.Error (ERR_FATAL, "GLW_CreateWindow: could not register window class");
#else
            ::ri.Error (ERR_VID_FATAL, "GLW_CreateWindow: could not register window class");
#endif // RTCW_XX

        }

        s_classRegistered = true;
        ::ri.Printf (PRINT_ALL, "...registered window class\n");
    }

    //
    // create the HWND if one does not already exist
    //
    if (g_wv.hWnd == 0) {
        //
        // compute width and height
        //
        r.left = 0;
        r.top = 0;
        r.right  = width;
        r.bottom = height;

        if (isFullscreen) {
            exstyle = WS_EX_TOPMOST;
            stylebits = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
        } else {
            exstyle = 0;
            stylebits = WINDOW_STYLE | WS_SYSMENU;
            ::AdjustWindowRect (&r, stylebits, FALSE);
        }

        w = r.right - r.left;
        h = r.bottom - r.top;

        if (isFullscreen) {
            x = 0;
            y = 0;
        } else {
            vid_xpos = ri.Cvar_Get ("vid_xpos", "", 0);
            vid_ypos = ri.Cvar_Get ("vid_ypos", "", 0);
            x = vid_xpos->integer;
            y = vid_ypos->integer;

            // adjust window coordinates if necessary
            // so that the window is completely on screen
            if (x < 0)
                x = 0;

            if (y < 0)
                y = 0;

            if ((w < glw_state.desktopWidth) && (h < glw_state.desktopHeight)) {
                    if (x + w > glw_state.desktopWidth)
                        x = glw_state.desktopWidth - w;

                    if (y + h > glw_state.desktopHeight)
                        y = glw_state.desktopHeight - h;
            }
        }

        g_wv.hWnd = ::CreateWindowEx (
            exstyle,
            WINDOW_CLASS_NAME,

#if defined RTCW_SP
            "Return to Castle Wolfenstein: Single Player",
#elif defined RTCW_MP
            "Return to Castle Wolfenstein: Multi Player",
#else
            "Return to Castle Wolfenstein: Enemy Territory",
#endif // RTCW_XX

            stylebits,
            x, y, w, h,
            0,
            0,
            g_wv.hInstance,
            0);

        if (g_wv.hWnd == 0) {

#if !defined RTCW_ET
            ::ri.Error (ERR_FATAL, "GLW_CreateWindow() - Couldn't create window");
#else
            ::ri.Error (ERR_VID_FATAL, "GLW_CreateWindow() - Couldn't create window");
#endif // RTCW_XX

        }

        ::ShowWindow (g_wv.hWnd, SW_SHOW);
        ::UpdateWindow (g_wv.hWnd);
        ::ri.Printf (PRINT_ALL, "...created window@%d,%d (%dx%d)\n", x, y, w, h);
    } else
        ::ri.Printf (PRINT_ALL, "...window already present, CreateWindowEx skipped\n");

    if (!::GLW_InitDriver ()) {
        ::ShowWindow (g_wv.hWnd, SW_HIDE);
        ::DestroyWindow (g_wv.hWnd);
        g_wv.hWnd = 0;

        return false;
    }

    ::SetForegroundWindow (g_wv.hWnd);
    ::SetFocus (g_wv.hWnd);

    return true;
}
//BBi

static void PrintCDSError( int value ) {
	switch ( value )
	{
	case DISP_CHANGE_RESTART:
		ri.Printf( PRINT_ALL, "restart required\n" );
		break;
	case DISP_CHANGE_BADPARAM:
		ri.Printf( PRINT_ALL, "bad param\n" );
		break;
	case DISP_CHANGE_BADFLAGS:
		ri.Printf( PRINT_ALL, "bad flags\n" );
		break;
	case DISP_CHANGE_FAILED:
		ri.Printf( PRINT_ALL, "DISP_CHANGE_FAILED\n" );
		break;
	case DISP_CHANGE_BADMODE:
		ri.Printf( PRINT_ALL, "bad mode\n" );
		break;
	case DISP_CHANGE_NOTUPDATED:
		ri.Printf( PRINT_ALL, "not updated\n" );
		break;
	default:
		ri.Printf( PRINT_ALL, "unknown error %d\n", value );
		break;
	}
}

/*
** GLW_SetMode
*/

//BBi
//static rserr_t GLW_SetMode( const char *drivername,
//							int mode,
//							int colorbits,
//							qboolean cdsFullscreen ) {
//	HDC hDC;
//	const char *win_fs[] = { "W", "FS" };
//	int cdsRet;
//	DEVMODE dm;
//
//	//
//	// print out informational messages
//	//
//	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );
//	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) ) {
//		ri.Printf( PRINT_ALL, " invalid mode\n" );
//		return RSERR_INVALID_MODE;
//	}
//	ri.Printf( PRINT_ALL, " %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, win_fs[cdsFullscreen] );
//
//	//
//	// check our desktop attributes
//	//
//	hDC = GetDC( GetDesktopWindow() );
//	glw_state.desktopBitsPixel = GetDeviceCaps( hDC, BITSPIXEL );
//	glw_state.desktopWidth = GetDeviceCaps( hDC, HORZRES );
//	glw_state.desktopHeight = GetDeviceCaps( hDC, VERTRES );
//	ReleaseDC( GetDesktopWindow(), hDC );
//
//	//
//	// verify desktop bit depth
//	//
//	if ( glConfig.driverType != GLDRV_VOODOO ) {
//		if ( glw_state.desktopBitsPixel < 15 || glw_state.desktopBitsPixel == 24 ) {
//			if ( colorbits == 0 || ( !cdsFullscreen && colorbits >= 15 ) ) {
//				if ( MessageBox( NULL,
//								 "It is highly unlikely that a correct\n"
//								 "windowed display can be initialized with\n"
//								 "the current desktop display depth.  Select\n"
//								 "'OK' to try anyway.  Press 'Cancel' if you\n"
//								 "have a 3Dfx Voodoo, Voodoo-2, or Voodoo Rush\n"
//								 "3D accelerator installed, or if you otherwise\n"
//								 "wish to quit.",
//								 "Low Desktop Color Depth",
//								 MB_OKCANCEL | MB_ICONEXCLAMATION ) != IDOK ) {
//					return RSERR_INVALID_MODE;
//				}
//			}
//		}
//	}
//
//	// do a CDS if needed
//	if ( cdsFullscreen ) {
//		memset( &dm, 0, sizeof( dm ) );
//
//		dm.dmSize = sizeof( dm );
//
//		dm.dmPelsWidth  = glConfig.vidWidth;
//		dm.dmPelsHeight = glConfig.vidHeight;
//		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;
//
//		if ( r_displayRefresh->integer != 0 ) {
//			dm.dmDisplayFrequency = r_displayRefresh->integer;
//			dm.dmFields |= DM_DISPLAYFREQUENCY;
//
//#if defined RTCW_ET
//		} else {
//			DEVMODE dmode;
//
//			if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dmode ) ) {
//				dm.dmDisplayFrequency = dmode.dmDisplayFrequency;
//				dm.dmFields |= DM_DISPLAYFREQUENCY;
//			}
//#endif // RTCW_XX
//
//		}
//
//		// try to change color depth if possible
//		if ( colorbits != 0 ) {
//			if ( glw_state.allowdisplaydepthchange ) {
//				dm.dmBitsPerPel = colorbits;
//				dm.dmFields |= DM_BITSPERPEL;
//				ri.Printf( PRINT_ALL, "...using colorsbits of %d\n", colorbits );
//			} else
//			{
//				ri.Printf( PRINT_ALL, "WARNING:...changing depth not supported on Win95 < pre-OSR 2.x\n" );
//			}
//		} else
//		{
//			ri.Printf( PRINT_ALL, "...using desktop display depth of %d\n", glw_state.desktopBitsPixel );
//		}
//
//		//
//		// if we're already in fullscreen then just create the window
//		//
//		if ( glw_state.cdsFullscreen ) {
//			ri.Printf( PRINT_ALL, "...already fullscreen, avoiding redundant CDS\n" );
//
//			if ( !GLW_CreateWindow( drivername, glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) ) {
//				ri.Printf( PRINT_ALL, "...restoring display settings\n" );
//				ChangeDisplaySettings( 0, 0 );
//				return RSERR_INVALID_MODE;
//			}
//		}
//		//
//		// need to call CDS
//		//
//		else
//		{
//			ri.Printf( PRINT_ALL, "...calling CDS: " );
//
//			// try setting the exact mode requested, because some drivers don't report
//			// the low res modes in EnumDisplaySettings, but still work
//			if ( ( cdsRet = ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
//				ri.Printf( PRINT_ALL, "ok\n" );
//
//				if ( !GLW_CreateWindow( drivername, glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) ) {
//					ri.Printf( PRINT_ALL, "...restoring display settings\n" );
//					ChangeDisplaySettings( 0, 0 );
//					return RSERR_INVALID_MODE;
//				}
//
//				glw_state.cdsFullscreen = qtrue;
//			} else
//			{
//				//
//				// the exact mode failed, so scan EnumDisplaySettings for the next largest mode
//				//
//				DEVMODE devmode;
//				int modeNum;
//
//				ri.Printf( PRINT_ALL, "failed, " );
//
//				PrintCDSError( cdsRet );
//
//				ri.Printf( PRINT_ALL, "...trying next higher resolution:" );
//
//				// we could do a better matching job here...
//				for ( modeNum = 0 ; ; modeNum++ ) {
//					if ( !EnumDisplaySettings( NULL, modeNum, &devmode ) ) {
//						modeNum = -1;
//						break;
//					}
//					if ( devmode.dmPelsWidth >= glConfig.vidWidth
//						 && devmode.dmPelsHeight >= glConfig.vidHeight
//						 && devmode.dmBitsPerPel >= 15 ) {
//						break;
//					}
//				}
//
//				if ( modeNum != -1 && ( cdsRet = ChangeDisplaySettings( &devmode, CDS_FULLSCREEN ) ) == DISP_CHANGE_SUCCESSFUL ) {
//					ri.Printf( PRINT_ALL, " ok\n" );
//					if ( !GLW_CreateWindow( drivername, glConfig.vidWidth, glConfig.vidHeight, colorbits, qtrue ) ) {
//						ri.Printf( PRINT_ALL, "...restoring display settings\n" );
//						ChangeDisplaySettings( 0, 0 );
//						return RSERR_INVALID_MODE;
//					}
//
//					glw_state.cdsFullscreen = qtrue;
//				} else
//				{
//					ri.Printf( PRINT_ALL, " failed, " );
//
//					PrintCDSError( cdsRet );
//
//					ri.Printf( PRINT_ALL, "...restoring display settings\n" );
//					ChangeDisplaySettings( 0, 0 );
//
//					glw_state.cdsFullscreen = qfalse;
//					glConfig.isFullscreen = qfalse;
//					if ( !GLW_CreateWindow( drivername, glConfig.vidWidth, glConfig.vidHeight, colorbits, qfalse ) ) {
//						return RSERR_INVALID_MODE;
//					}
//					return RSERR_INVALID_FULLSCREEN;
//				}
//			}
//		}
//	} else
//	{
//		if ( glw_state.cdsFullscreen ) {
//			ChangeDisplaySettings( 0, 0 );
//		}
//
//		glw_state.cdsFullscreen = qfalse;
//		if ( !GLW_CreateWindow( drivername, glConfig.vidWidth, glConfig.vidHeight, colorbits, qfalse ) ) {
//			return RSERR_INVALID_MODE;
//		}
//	}
//
//	//
//	// success, now check display frequency, although this won't be valid on Voodoo(2)
//	//
//	memset( &dm, 0, sizeof( dm ) );
//	dm.dmSize = sizeof( dm );
//	if ( EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ) ) {
//		glConfig.displayFrequency = dm.dmDisplayFrequency;
//	}
//
//	// NOTE: this is overridden later on standalone 3Dfx drivers
//	glConfig.isFullscreen = cdsFullscreen;
//
//	return RSERR_OK;
//}

static rserr_t GLW_SetMode (
    int mode,
    bool isFullscreen)
{
    const char *win_fs[] = { "W", "FS" };
    int cdsRet;
    DEVMODE dm;

    glConfigEx.isNativeResolution = false;

    //
    // print out informational messages
    //
    ::ri.Printf (PRINT_ALL, "...setting mode %d:", mode);
    if (!R_GetModeInfo (&glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode)) {
        ::ri.Printf (PRINT_ALL, " invalid mode\n");
        return RSERR_INVALID_MODE;
    }

    ::ri.Printf (PRINT_ALL, " %d %d %s\n", glConfig.vidWidth, glConfig.vidHeight, win_fs[isFullscreen]);

    //
    // check our desktop attributes
    //
    HDC hDC = ::GetDC (::GetDesktopWindow ());
    glw_state.desktopBitsPixel = ::GetDeviceCaps (hDC, BITSPIXEL);
    glw_state.desktopWidth = ::GetDeviceCaps (hDC, HORZRES);
    glw_state.desktopHeight = ::GetDeviceCaps (hDC, VERTRES);
    ::ReleaseDC (::GetDesktopWindow (), hDC);

    //BBi
    glConfigEx.isNativeResolution =
        (glConfig.vidWidth == glw_state.desktopWidth) &&
        (glConfig.vidHeight == glw_state.desktopHeight);
    //BBi

    // do a CDS if needed
    if (isFullscreen) {
        if (glConfigEx.isNativeResolution) {
            glw_state.cdsFullscreen = true;
            ::ChangeDisplaySettings (0, 0);
        } else {
            ::memset (&dm, 0, sizeof (dm));

            dm.dmSize = sizeof (dm);
            dm.dmPelsWidth  = glConfig.vidWidth;
            dm.dmPelsHeight = glConfig.vidHeight;
            dm.dmBitsPerPel = 32;
            dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        }

        //
        // if we're already in fullscreen then just create the window
        //
        if (glw_state.cdsFullscreen) {
            ::ri.Printf (PRINT_ALL, "...already fullscreen, avoiding redundant CDS\n");

            if (!::GLW_CreateWindow (glConfig.vidWidth, glConfig.vidHeight, true)) {
                ::ri.Printf (PRINT_ALL, "...restoring display settings\n");
                ::ChangeDisplaySettings (0, 0);
                return RSERR_INVALID_MODE;
            }
        }
        //
        // need to call CDS
        //
        else
        {
            ::ri.Printf (PRINT_ALL, "...calling CDS: ");

            // try setting the exact mode requested, because some drivers don't report
            // the low res modes in EnumDisplaySettings, but still work
            if ((cdsRet = ::ChangeDisplaySettings (&dm, CDS_FULLSCREEN)) == DISP_CHANGE_SUCCESSFUL) {
                ::ri.Printf (PRINT_ALL, "ok\n");

                if (!::GLW_CreateWindow (glConfig.vidWidth, glConfig.vidHeight, true)) {
                    ::ri.Printf (PRINT_ALL, "...restoring display settings\n");
                    ::ChangeDisplaySettings (0, 0);
                    return RSERR_INVALID_MODE;
                }

                glw_state.cdsFullscreen = true;
            } else {
                ::ri.Printf (PRINT_ALL, " failed, ");

                ::PrintCDSError (cdsRet);

                ::ri.Printf (PRINT_ALL, "...restoring display settings\n");
                ::ChangeDisplaySettings (0, 0);

                glw_state.cdsFullscreen = false;
                glConfig.isFullscreen = false;

                if (!::GLW_CreateWindow (glConfig.vidWidth, glConfig.vidHeight, false))
                    return RSERR_INVALID_MODE;

                return RSERR_INVALID_FULLSCREEN;
            }
        }
    } else {
        if (glw_state.cdsFullscreen)
            ::ChangeDisplaySettings (0, 0);

        glw_state.cdsFullscreen = false;

        if (!::GLW_CreateWindow (glConfig.vidWidth, glConfig.vidHeight, false))
            return RSERR_INVALID_MODE;
    }

    //
    // success, now check display frequency, although this won't be valid on Voodoo(2)
    //
    ::memset (&dm, 0, sizeof (dm));
    dm.dmSize = sizeof (dm);

    if (::EnumDisplaySettings (NULL, ENUM_CURRENT_SETTINGS, &dm))
        glConfig.displayFrequency = dm.dmDisplayFrequency;

    // NOTE: this is overridden later on standalone 3Dfx drivers
    glConfig.isFullscreen = isFullscreen;

    return RSERR_OK;
}
//BBi

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void ) {

//BBi
//#if defined RTCW_SP
//BBi
//----(SA)	moved these up
	glConfig.textureCompression = TC_NONE;
	glConfig.textureEnvAddAvailable = qfalse;

    //BBi
	//qglMultiTexCoord2fARB = NULL;
	//qglActiveTextureARB = NULL;
	//qglClientActiveTextureARB = NULL;
	//qglLockArraysEXT = NULL;
	//qglUnlockArraysEXT = NULL;
	//qwglGetDeviceGammaRamp3DFX = NULL;
	//qwglSetDeviceGammaRamp3DFX = NULL;
	//qglPNTrianglesiATI = NULL;
	//qglPNTrianglesfATI = NULL;
    //BBi

	glConfig.anisotropicAvailable = qfalse;
	glConfig.NVFogAvailable = qfalse;
	glConfig.NVFogMode = 0;
//----(SA)	end

//BBi
//#endif // RTCW_XX
//BBi

    //BBi
    glConfigEx.reset ();
    //BBi

	if ( !r_allowExtensions->integer ) {
		ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
		return;
	}

	ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

	// GL_S3_s3tc

//BBi
//#if !defined RTCW_SP
//	glConfig.textureCompression = TC_NONE;
//#endif // RTCW_XX
//
//	// RF, check for GL_EXT_texture_compression_s3tc
//	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_compression_s3tc" ) ) {
//		if ( r_ext_compressed_textures->integer ) {
//			glConfig.textureCompression = TC_EXT_COMP_S3TC;
//			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_compression_s3tc\n" );
//		} else
//		{
//			glConfig.textureCompression = TC_NONE;
//			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_compression_s3tc\n" );
//		}
//	}
//	/* RF, disabled this section, since this method produces very ugly results on nvidia hardware
//	else if ( strstr( glConfig.extensions_string, "GL_S3_s3tc" ) )
//	{
//		if ( r_ext_compressed_textures->integer )
//		{
//			glConfig.textureCompression = TC_S3TC;
//			ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
//		}
//		else
//		{
//			glConfig.textureCompression = TC_NONE;
//			ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
//		}
//	}
//	*/
//	else
//	{
//		ri.Printf( PRINT_ALL, "...GL_EXT_texture_compression_s3tc not found\n" );
//	}

    if (GLEW_ARB_texture_compression != 0) {
        if (r_ext_compressed_textures->integer != 0) {
            glConfig.textureCompression = TC_ARB;
            ::ri.Printf (PRINT_ALL, "...using %s\n",
                "GL_ARB_texture_compression");
        } else {
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n",
                "GL_ARB_texture_compression");
        }
    } else if (GLEW_EXT_texture_compression_s3tc != 0) {
        if (r_ext_compressed_textures->integer != 0) {
            glConfig.textureCompression = TC_EXT_COMP_S3TC;
            ::ri.Printf (PRINT_ALL, "...using %s\n",
                "GL_EXT_texture_compression_s3tc");
        } else {
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n",
                "GL_EXT_texture_compression_s3tc");
        }
    } else if (GLEW_S3_s3tc != 0) {
        if (r_ext_compressed_textures->integer != 0) {
            glConfig.textureCompression = TC_S3TC;
            ::ri.Printf (PRINT_ALL, "...using %s\n", "GL_S3_s3tc");
        } else
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n", "GL_S3_s3tc");
    } else
        ::ri.Printf (PRINT_ALL, "...texture compression not found\n");
//BBi

	// GL_EXT_texture_env_add

//BBi
//#if !defined RTCW_SP
//	glConfig.textureEnvAddAvailable = qfalse;
//#endif // RTCW_XX
//BBi

    // BBi
	//if ( strstr( glConfig.extensions_string, "EXT_texture_env_add" ) ) {
	//	if ( r_ext_texture_env_add->integer ) {
	//		glConfig.textureEnvAddAvailable = qtrue;
	//		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
	//	} else
	//	{
	//		glConfig.textureEnvAddAvailable = qfalse;
	//		ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
	//	}
	//} else
	//{
	//	ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
	//}

    if (GLEW_EXT_texture_env_add != 0) {
        if (r_ext_texture_env_add->integer != 0) {
            glConfig.textureEnvAddAvailable = true;
            ::ri.Printf (PRINT_ALL, "...using %s\n", "GL_EXT_texture_env_add");
        } else {
            glConfig.textureEnvAddAvailable = false;
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n",
                "GL_EXT_texture_env_add");
        }
    } else {
        ::ri.Printf (PRINT_ALL, "...%s not found\n",
            "GL_EXT_texture_env_add");
    }
    // BBi

	// WGL_EXT_swap_control

    //BBi
	//qwglSwapIntervalEXT = ( BOOL ( WINAPI * )(int) )qwglGetProcAddress( "wglSwapIntervalEXT" );
	//if ( qwglSwapIntervalEXT ) {
	//	ri.Printf( PRINT_ALL, "...using WGL_EXT_swap_control\n" );
	//	r_swapInterval->modified = qtrue;   // force a set next frame
	//} else
	//{
	//	ri.Printf( PRINT_ALL, "...WGL_EXT_swap_control not found\n" );
	//}

    if (WGLEW_EXT_swap_control != 0) {
        ::ri.Printf (PRINT_ALL, "...using %s\n", "WGL_EXT_swap_control");
        r_swapInterval->modified = true;   // force a set next frame
    } else
        ::ri.Printf (PRINT_ALL, "...%s not found\n", "WGL_EXT_swap_control");
    //BBi

	// GL_ARB_multitexture

//BBi
//#if !defined RTCW_SP
//	qglMultiTexCoord2fARB = NULL;
//	qglActiveTextureARB = NULL;
//	qglClientActiveTextureARB = NULL;
//#endif // RTCW_XX
//
//	if ( strstr( glConfig.extensions_string, "GL_ARB_multitexture" )  ) {
//		if ( r_ext_multitexture->integer ) {
//			qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
//			qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glActiveTextureARB" );
//			qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) qwglGetProcAddress( "glClientActiveTextureARB" );
//
//			if ( qglActiveTextureARB ) {
//				qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );
//
//				if ( glConfig.maxActiveTextures > 1 ) {
//					ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
//				} else
//				{
//					qglMultiTexCoord2fARB = NULL;
//					qglActiveTextureARB = NULL;
//					qglClientActiveTextureARB = NULL;
//					ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
//				}
//			}
//		} else
//		{
//			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
//		}
//	} else
//	{
//		ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
//	}

    if (GLEW_ARB_multitexture != 0) {
        if (r_ext_multitexture->integer != 0) {
            ::glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &glConfig.maxActiveTextures);

            if (glConfig.maxActiveTextures > 1) {
                glConfigEx.useArbMultitexture = true;
                ri.Printf (PRINT_ALL, "...using %s\n", "GL_ARB_multitexture");
            } else {
                ri.Printf (PRINT_ALL, "...not using %s, < 2 texture units\n",
                "GL_ARB_multitexture");
            }
        } else
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n", "GL_ARB_multitexture");
    } else
        ::ri.Printf (PRINT_ALL, "...%s not found\n", "GL_ARB_multitexture");
//BBi

	// GL_EXT_compiled_vertex_array


//BBi
//#if !defined RTCW_SP
//	qglLockArraysEXT = NULL;
//	qglUnlockArraysEXT = NULL;
//#endif // RTCW_XX
//
//	if ( strstr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) && ( glConfig.hardwareType != GLHW_RIVA128 ) ) {
//		if ( r_ext_compiled_vertex_array->integer ) {
//			ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
//			qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) )qwglGetProcAddress( "glLockArraysEXT" );
//			qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) )qwglGetProcAddress( "glUnlockArraysEXT" );
//			if ( !qglLockArraysEXT || !qglUnlockArraysEXT ) {
//
//#if !defined RTCW_ET
//				ri.Error( ERR_FATAL, "bad getprocaddress" );
//#else
//				ri.Error( ERR_VID_FATAL, "bad getprocaddress" );
//#endif // RTCW_XX
//
//			}
//		} else
//		{
//			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
//		}
//	} else
//	{
//		ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
//	}

    if (GLEW_EXT_compiled_vertex_array != 0) {
        if (r_ext_compiled_vertex_array->integer != 0) {
            glConfigEx.useExtCompiledVertexArray = true;
            ::ri.Printf (PRINT_ALL, "...using %s\n",
                "GL_EXT_compiled_vertex_array");
        } else {
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n",
                "GL_EXT_compiled_vertex_array");
        }
    } else {
        ::ri.Printf (PRINT_ALL, "...%s not found\n",
            "GL_EXT_compiled_vertex_array");
    }
//BBi

//BBi
//	// WGL_3DFX_gamma_control
//
//#if !defined RTCW_SP
//	qwglGetDeviceGammaRamp3DFX = NULL;
//	qwglSetDeviceGammaRamp3DFX = NULL;
//#endif // RTCW_XX
//
//	if ( strstr( glConfig.extensions_string, "WGL_3DFX_gamma_control" ) ) {
//		if ( !r_ignorehwgamma->integer && r_ext_gamma_control->integer ) {
//			qwglGetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) )qwglGetProcAddress( "wglGetDeviceGammaRamp3DFX" );
//			qwglSetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) )qwglGetProcAddress( "wglSetDeviceGammaRamp3DFX" );
//
//			if ( qwglGetDeviceGammaRamp3DFX && qwglSetDeviceGammaRamp3DFX ) {
//				ri.Printf( PRINT_ALL, "...using WGL_3DFX_gamma_control\n" );
//			} else
//			{
//				qwglGetDeviceGammaRamp3DFX = NULL;
//				qwglSetDeviceGammaRamp3DFX = NULL;
//			}
//		} else
//		{
//			ri.Printf( PRINT_ALL, "...ignoring WGL_3DFX_gamma_control\n" );
//		}
//	} else
//	{
//		ri.Printf( PRINT_ALL, "...WGL_3DFX_gamma_control not found\n" );
//	}
//BBi

//BBi
//#if defined RTCW_SP
////----(SA)	added
//
//
//	// GL_ATI_pn_triangles - ATI PN-Triangles
//	if ( strstr( glConfig.extensions_string, "GL_ATI_pn_triangles" ) ) {
//		if ( r_ext_ATI_pntriangles->integer ) {
//			ri.Printf( PRINT_ALL, "...using GL_ATI_pn_triangles\n" );
//
//			qglPNTrianglesiATI = ( PFNGLPNTRIANGLESIATIPROC ) qwglGetProcAddress( "glPNTrianglesiATI" );
//			qglPNTrianglesfATI = ( PFNGLPNTRIANGLESFATIPROC ) qwglGetProcAddress( "glPNTrianglesfATI" );
//
//			if ( !qglPNTrianglesiATI || !qglPNTrianglesfATI ) {
//				ri.Error( ERR_FATAL, "bad getprocaddress 0" );
//			}
//		} else {
//			ri.Printf( PRINT_ALL, "...ignoring GL_ATI_pn_triangles\n" );
//			ri.Cvar_Set( "r_ext_ATI_pntriangles", "0" );
//		}
//	} else {
//		ri.Printf( PRINT_ALL, "...GL_ATI_pn_triangles not found\n" );
//		ri.Cvar_Set( "r_ext_ATI_pntriangles", "0" );
//	}
//
//
//
//	// GL_EXT_texture_filter_anisotropic
//	if ( strstr( glConfig.extensions_string, "GL_EXT_texture_filter_anisotropic" ) ) {
//		if ( r_ext_texture_filter_anisotropic->integer ) {
////			glConfig.anisotropicAvailable = qtrue;
////			ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
//
//			// always ignored.  unsupported.
//			glConfig.anisotropicAvailable = qfalse;
//			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
//			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
//
//		} else {
//			glConfig.anisotropicAvailable = qfalse;
//			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
//			ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
//		}
//	} else {
////		ri.Printf( PRINT_ALL, "...GL_EXT_texture_filter_anisotropic not found\n" );
//		ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
//	}
//#endif // RTCW_XX
//BBi

    //BBi
	//// GL_NV_fog_distance
	//if ( strstr( glConfig.extensions_string, "GL_NV_fog_distance" ) ) {
	//	if ( r_ext_NV_fog_dist->integer ) {
	//		glConfig.NVFogAvailable = qtrue;
	//		ri.Printf( PRINT_ALL, "...using GL_NV_fog_distance\n" );
	//	} else {
	//		ri.Printf( PRINT_ALL, "...ignoring GL_NV_fog_distance\n" );
	//		ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
	//	}
	//} else {
	//	ri.Printf( PRINT_ALL, "...GL_NV_fog_distance not found\n" );
	//	ri.Cvar_Set( "r_ext_NV_fog_dist", "0" );
	//}
    if (GLEW_NV_fog_distance != 0) {
        if (r_ext_NV_fog_dist->integer != 0) {
            ::glConfig.NVFogAvailable = true;
            ::ri.Printf (PRINT_ALL, "...using %s\n", "GL_NV_fog_distance");
        } else {
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n", "GL_NV_fog_distance");
            ::ri.Cvar_Set ("r_ext_NV_fog_dist", "0");
        }
    } else {
        ::ri.Printf (PRINT_ALL, "...%s not found\n", "GL_NV_fog_distance");
        ::ri.Cvar_Set ("r_ext_NV_fog_dist", "0");
    }
    //BBi

//BBi
//#if defined RTCW_SP
////----(SA)	end
//
//	// support?
////	SGIS_generate_mipmap
////	ARB_multisample
//#endif // RTCW_XX
//BBi

//BBi
//#if defined RTCW_ET
//BBi

    //BBi
	//// GL_EXT_texture_filter_anisotropic
	//if ( Q_stristr( glConfig.extensions_string, "GL_EXT_texture_filter_anisotropic" ) ) {
	//	if ( r_ext_texture_filter_anisotropic->integer ) {
	//		glConfig.anisotropicAvailable = qtrue;
	//		ri.Printf( PRINT_ALL, "...using GL_EXT_texture_filter_anisotropic\n" );
	//	} else {
	//		ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_filter_anisotropic\n" );
	//		ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
	//	}
	//} else {
	//	ri.Printf( PRINT_ALL, "... GL_EXT_texture_filter_anisotropic not found\n" );
	//	ri.Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
	//}

    // GL_EXT_texture_filter_anisotropic
    if (GLEW_EXT_texture_filter_anisotropic != 0) {
        if (r_ext_texture_filter_anisotropic->integer != 0) {
            glConfig.anisotropicAvailable = true;
            ::ri.Printf (PRINT_ALL, "...using %s\n",
                "GL_EXT_texture_filter_anisotropic");
        } else {
            ::ri.Printf (PRINT_ALL, "...ignoring %s\n",
                "GL_EXT_texture_filter_anisotropic");
            ::ri.Cvar_Set ("r_ext_texture_filter_anisotropic", "0");
        }
    } else {
        ::ri.Printf (PRINT_ALL, "... %s not found\n",
            "GL_EXT_texture_filter_anisotropic");
        ::ri.Cvar_Set ("r_ext_texture_filter_anisotropic", "0");
    }
    //BBi

//BBi
//#endif // RTCW_XX
//BBi

    //BBi
    if (GLEW_ARB_framebuffer_object != 0) {
        glConfigEx.useArbFramebufferObject = true;
        ::ri.Printf (PRINT_ALL, "...using %s\n",
            "GL_ARB_framebuffer_object");
    } else {
        ::ri.Printf (PRINT_ALL, "...%s not found\n",
            "GL_ARB_framebuffer_object");
    }


    if (GLEW_ARB_texture_non_power_of_two != 0) {
        glConfigEx.useArbTextureNonPowerOfTwo = true;
        ::ri.Printf (PRINT_ALL, "...using %s\n",
            "GL_ARB_texture_non_power_of_two");
    } else {
        ::ri.Printf (PRINT_ALL, "...%s not found\n",
            "GL_ARB_texture_non_power_of_two");
    }

    if (GLEW_ARB_draw_elements_base_vertex != 0) {
        glConfigEx.use_arb_draw_elements_base_vertex = true;
        ::ri.Printf (PRINT_ALL, "...using %s\n",
            "GL_ARB_draw_elements_base_vertex");
    } else {
        ::ri.Printf (PRINT_ALL, "...%s not found\n",
            "GL_ARB_draw_elements_base_vertex");
    }
}

#if defined RTCW_ET
/*
** GLW_GenDefaultLists
*/
static void GLW_GenDefaultLists( void ) {
	HFONT hfont, oldhfont;

	// keep going, we'll probably just leak some stuff
	if ( fontbase_init ) {
		Com_DPrintf( "ERROR: GLW_GenDefaultLists: font base is already marked initialized\n" );
	}

	// create font display lists
	gl_NormalFontBase = ::glGenLists( 256 );

	if ( gl_NormalFontBase == 0 ) {
		Com_Printf( "ERROR: couldn't create font (glGenLists)\n" );
		return;
	}

	hfont = CreateFont(
		12, // logical height of font
		6,  // logical average character width
		0,  // angle of escapement
		0,  // base-line orientation angle
		0,  // font weight
		0,  // italic attribute flag
		0,  // underline attribute flag
		0,  // strikeout attribute flag
		0,  // character set identifier
		0,  // output precision
		0,  // clipping precision
		0,  // output quality
		0,  // pitch and family
		"" ); // pointer to typeface name string

	if ( !hfont ) {
		Com_Printf( "ERROR: couldn't create font (CreateFont)\n" );
		return;
	}

	oldhfont = reinterpret_cast<HFONT> (SelectObject( glw_state.hDC, hfont ));
	::wglUseFontBitmaps( glw_state.hDC, 0, 255, gl_NormalFontBase );

	SelectObject( glw_state.hDC, oldhfont );
	DeleteObject( hfont );

	fontbase_init = qtrue;
}

/*
** GLW_DeleteDefaultLists
*/
static void GLW_DeleteDefaultLists( void ) {
	if ( !fontbase_init ) {
		Com_DPrintf( "ERROR: GLW_DeleteDefaultLists: no font list initialized\n" );
		return;
	}

	::glDeleteLists( gl_NormalFontBase, 256 );
	fontbase_init = qfalse;
}
#endif // RTCW_XX

//BBi
///*
//** GLW_CheckOSVersion
//*/
//static qboolean GLW_CheckOSVersion( void ) {
//#define OSR2_BUILD_NUMBER 1111
//
//	OSVERSIONINFO vinfo;
//
//	vinfo.dwOSVersionInfoSize = sizeof( vinfo );
//
//	glw_state.allowdisplaydepthchange = qfalse;
//
//	if ( GetVersionEx( &vinfo ) ) {
//		if ( vinfo.dwMajorVersion > 4 ) {
//			glw_state.allowdisplaydepthchange = qtrue;
//		} else if ( vinfo.dwMajorVersion == 4 )   {
//			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) {
//				glw_state.allowdisplaydepthchange = qtrue;
//			} else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )   {
//				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER ) {
//					glw_state.allowdisplaydepthchange = qtrue;
//				}
//			}
//		}
//	} else
//	{
//		ri.Printf( PRINT_ALL, "GLW_CheckOSVersion() - GetVersionEx failed\n" );
//		return qfalse;
//	}
//
//	return qtrue;
//}
//BBi

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that attempts to load and use
** a specific OpenGL DLL.
*/

//BBi
//static qboolean GLW_LoadOpenGL( const char *drivername ) {
//	char buffer[1024];
//	qboolean cdsFullscreen;
//
//	Q_strncpyz( buffer, drivername, sizeof( buffer ) );
//	Q_strlwr( buffer );
//
//	//
//	// determine if we're on a standalone driver
//	//
//	if ( strstr( buffer, "opengl32" ) != 0 || r_maskMinidriver->integer ) {
//		glConfig.driverType = GLDRV_ICD;
//	} else
//	{
//		glConfig.driverType = GLDRV_STANDALONE;
//
//		ri.Printf( PRINT_ALL, "...assuming '%s' is a standalone driver\n", drivername );
//
//		if ( strstr( buffer, _3DFX_DRIVER_NAME ) ) {
//			glConfig.driverType = GLDRV_VOODOO;
//		}
//	}
//
//	// disable the 3Dfx splash screen
//	_putenv( "FX_GLIDE_NO_SPLASH=0" );
//
//	//
//	// load the driver and bind our function pointers to it
//	//
//	if ( QGL_Init( buffer ) ) {
//#if 0
//// FIXME: newer 3Dfx drivers means this can go away
//		if ( !Q_stricmp( buffer, _3DFX_DRIVER_NAME ) ) {
//			cdsFullscreen = qfalse;
//		} else
//#endif
//		{
//			cdsFullscreen = r_fullscreen->integer;
//		}
//
//		// create the window and set up the context
//		if ( !GLW_StartDriverAndSetMode( drivername, r_mode->integer, r_colorbits->integer, cdsFullscreen ) ) {
//			// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
//			// try it again but with a 16-bit desktop
//			if ( glConfig.driverType == GLDRV_ICD ) {
//				if ( r_colorbits->integer != 16 ||
//					 cdsFullscreen != qtrue ||
//					 r_mode->integer != 3 ) {
//					if ( !GLW_StartDriverAndSetMode( drivername, 3, 16, qtrue ) ) {
//						goto fail;
//					}
//				}
//			} else
//			{
//				goto fail;
//			}
//		}
//
//		if ( glConfig.driverType == GLDRV_VOODOO ) {
//			glConfig.isFullscreen = qtrue;
//		}
//
//		return qtrue;
//	}
//fail:
//
//	QGL_Shutdown();
//
//	return qfalse;
//}
//BBi

/*
** GLimp_EndFrame
*/

//BBi
//void GLimp_EndFrame( void ) {
//	//
//	// swapinterval stuff
//	//
//	if ( r_swapInterval->modified ) {
//		r_swapInterval->modified = qfalse;
//
//		if ( !glConfig.stereoEnabled ) {    // why?
//			if ( qwglSwapIntervalEXT ) {
//				qwglSwapIntervalEXT( r_swapInterval->integer );
//			}
//		}
//	}
//
//
//	// don't flip if drawing to front buffer
//	if ( Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
//		if ( glConfig.driverType > GLDRV_ICD ) {
//			if ( !qwglSwapBuffers( glw_state.hDC ) ) {
//
//#if !defined RTCW_ET
//				ri.Error( ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n" );
//#else
//				ri.Error( ERR_VID_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n" );
//#endif // RTCW_XX
//
//			}
//		} else
//		{
//			SwapBuffers( glw_state.hDC );
//		}
//	}
//
//	// check logging
//	QGL_EnableLogging( r_logFile->integer );
//}

void GLimp_EndFrame ()
{
    //
    // swapinterval stuff
    //
    if (r_swapInterval->modified) {
        r_swapInterval->modified = qfalse;

        if (!glConfig.stereoEnabled) { // why?
            if (WGLEW_EXT_swap_control != 0)
                ::wglSwapIntervalEXT (r_swapInterval->integer);
        }
    }


    // don't flip if drawing to front buffer
    if (::Q_stricmp (r_drawBuffer->string, "GL_FRONT") != 0)
        ::SwapBuffers (glw_state.hDC);
}
//BBi


//BBi
//extern qboolean GlideIsValid( void );
//static void GLW_StartOpenGL( void ) {
//	qboolean attemptedOpenGL32 = qfalse;
//	qboolean attempted3Dfx = qfalse;
//
//	// this bit will pre-detect voodoo gl and if appropriate
//	// set the r_glDriver to point at one of the wicked 3D drivers
//	if ( !r_glIgnoreWicked3D->integer && GlideIsValid() ) {
//		const char *vid = WICKED3D_V5_DRIVER_NAME;
//		HMODULE handle;
//		handle = LoadLibrary( vid );
//		if ( handle == 0 ) {
//			vid = WICKED3D_V3_DRIVER_NAME;
//			handle = LoadLibrary( vid );
//		}
//
//		if ( handle ) {
//			Cvar_Set( "r_glDriver", vid );
//			FreeLibrary( handle );
//		}
//	}
//
//	if ( r_glIgnoreWicked3D->integer ) {
//		Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
//	}
//
//	//
//	// load and initialize the specific OpenGL driver
//	//
//	if ( !GLW_LoadOpenGL( r_glDriver->string ) ) {
//		if ( !Q_stricmp( r_glDriver->string, OPENGL_DRIVER_NAME ) ) {
//			attemptedOpenGL32 = qtrue;
//		} else if ( !Q_stricmp( r_glDriver->string, _3DFX_DRIVER_NAME ) )   {
//			attempted3Dfx = qtrue;
//		}
//
//		if ( !attempted3Dfx ) {
//			attempted3Dfx = qtrue;
//			if ( GLW_LoadOpenGL( _3DFX_DRIVER_NAME ) ) {
//				ri.Cvar_Set( "r_glDriver", _3DFX_DRIVER_NAME );
//				r_glDriver->modified = qfalse;
//			} else
//			{
//				if ( !attemptedOpenGL32 ) {
//					if ( !GLW_LoadOpenGL( OPENGL_DRIVER_NAME ) ) {
//
//#if !defined RTCW_ET
//						ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
//#else
//						ri.Error( ERR_VID_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
//#endif // RTCW_XX
//
//					}
//					ri.Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
//					r_glDriver->modified = qfalse;
//				} else
//				{
//
//#if !defined RTCW_ET
//					ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
//#else
//					ri.Error( ERR_VID_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
//#endif // RTCW_XX
//
//				}
//			}
//		} else if ( !attemptedOpenGL32 )   {
//			attemptedOpenGL32 = qtrue;
//			if ( GLW_LoadOpenGL( OPENGL_DRIVER_NAME ) ) {
//				ri.Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
//				r_glDriver->modified = qfalse;
//			} else
//			{
//
//#if !defined RTCW_ET
//				ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
//#else
//				ri.Error( ERR_VID_FATAL, "GLW_StartOpenGL() - could not load OpenGL subsystem\n" );
//#endif // RTCW_XX
//
//			}
//		}
//	}
//}

static void GLW_StartOpenGL ()
{
    ::Cvar_Set ("r_glDriver", OPENGL_DRIVER_NAME);

    glConfig.driverType = GLDRV_ICD;

    bool isFullscreen = (r_fullscreen->integer != 0);

    bool modeResult = ::GLW_StartDriverAndSetMode (r_mode->integer, isFullscreen);

    if (!modeResult) {
#if !defined RTCW_ET
        ::ri.Error( ERR_FATAL, "GLW_StartOpenGL() - could not set video mode\n" );
#else
        ::ri.Error( ERR_VID_FATAL, "GLW_StartOpenGL() - could not set video mode\n" );
#endif // RTCW_XX
    }
}
//BBi

/*
** GLimp_Init
**
** This is the platform specific OpenGL initialization function.  It
** is responsible for loading OpenGL, initializing it, setting
** extensions, creating a window of the appropriate size, doing
** fullscreen manipulations, etc.  Its overall responsibility is
** to make sure that a functional OpenGL subsystem is operating
** when it returns to the ref.
*/
void GLimp_Init( void ) {
	char buf[1024];
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	cvar_t  *cv;

	ri.Printf( PRINT_ALL, "Initializing OpenGL subsystem\n" );

//BBi
//	//
//	// check OS version to see if we can do fullscreen display changes
//	//
//	if ( !GLW_CheckOSVersion() ) {
//
//#if !defined RTCW_ET
//		ri.Error( ERR_FATAL, "GLimp_Init() - incorrect operating system\n" );
//#else
//		ri.Error( ERR_VID_FATAL, "GLimp_Init() - incorrect operating system\n" );
//#endif // RTCW_XX
//
//	}
//BBi

	// save off hInstance and wndproc
	cv = ri.Cvar_Get( "win_hinstance", "", 0 );

    // BBi
    const char* const scanFormat = (sizeof (size_t) == 4) ? "%i" : "%lli";
    // BBi

    // BBi
	//sscanf( cv->string, "%i", (int *)&g_wv.hInstance );
    ::sscanf (cv->string, scanFormat, reinterpret_cast<intptr_t*> (&g_wv.hInstance));
    // BBi

	cv = ri.Cvar_Get( "win_wndproc", "", 0 );

    // BBi
	//sscanf( cv->string, "%i", (int *)&glw_state.wndproc );
    ::sscanf (cv->string, scanFormat, reinterpret_cast<intptr_t*> (&glw_state.wndproc));
    // BBi

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_maskMinidriver = ri.Cvar_Get( "r_maskMinidriver", "0", CVAR_LATCH );

	// load appropriate DLL and initialize subsystem
	GLW_StartOpenGL();

	// get our config strings
	Q_strncpyz( glConfig.vendor_string, reinterpret_cast<const char*> (::glGetString( GL_VENDOR )), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, reinterpret_cast<const char*> (::glGetString( GL_RENDERER )), sizeof( glConfig.renderer_string ) );
	Q_strncpyz( glConfig.version_string, reinterpret_cast<const char*> (::glGetString( GL_VERSION )), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, reinterpret_cast<const char*> (::glGetString( GL_EXTENSIONS )), sizeof( glConfig.extensions_string ) );

#if !defined RTCW_SP
	// TTimo - safe check
	if ( strlen( reinterpret_cast<const char*> (::glGetString( GL_EXTENSIONS ) )) >= sizeof( glConfig.extensions_string ) ) {
		Com_Printf( S_COLOR_YELLOW "WARNNING: GL extensions string too long (%d), truncated to %d\n", strlen( reinterpret_cast<const char*> (::glGetString( GL_EXTENSIONS )) ), sizeof( glConfig.extensions_string ) );
	}
#endif // RTCW_XX

	//
	// chipset specific configuration
	//
	Q_strncpyz( buf, glConfig.renderer_string, sizeof( buf ) );
	Q_strlwr( buf );

//BBi
	////
	//// NOTE: if changing cvars, do it within this block.  This allows them
	//// to be overridden when testing driver fixes, etc. but only sets
	//// them to their default state when the hardware is first installed/run.
	////
	//if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) ) {
	//	glConfig.hardwareType = GLHW_GENERIC;
//
//		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
//
//		// VOODOO GRAPHICS w/ 2MB
//		if ( strstr( buf, "voodoo graphics/1 tmu/2 mb" ) ) {
//			ri.Cvar_Set( "r_picmip", "2" );
//			ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
//
//#if defined RTCW_SP
//		} else if ( strstr( buf, "matrox" ) ) {
//			ri.Cvar_Set( "r_allowExtensions", "0" );
//		} else {
//#else
//		} else
//		{
//
////----(SA)	FIXME: RETURN TO DEFAULT  Another id build change for DK/DM
//			ri.Cvar_Set( "r_picmip", "1" );   //----(SA)	was "1" // JPW NERVE back to 1
////----(SA)
//#endif // RTCW_XX
//
//			if ( strstr( buf, "rage 128" ) || strstr( buf, "rage128" ) ) {
//				ri.Cvar_Set( "r_finish", "0" );
//			}
//			// Savage3D and Savage4 should always have trilinear enabled
//			else if ( strstr( buf, "savage3d" ) || strstr( buf, "s3 savage4" ) ) {
//				ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
//			}
//		}
//	}
//
//	//
//	// this is where hardware specific workarounds that should be
//	// detected/initialized every startup should go.
//	//
//	if ( strstr( buf, "banshee" ) || strstr( buf, "voodoo3" ) ) {
//		glConfig.hardwareType = GLHW_3DFX_2D3D;
//	}
//	// VOODOO GRAPHICS w/ 2MB
//	else if ( strstr( buf, "voodoo graphics/1 tmu/2 mb" ) ) {
//	} else if ( strstr( buf, "glzicd" ) )    {
//
//#if !defined RTCW_ET
//	} else if ( strstr( buf, "rage pro" ) || strstr( buf, "Rage Pro" ) || strstr( buf, "ragepro" ) )       {
//#else
//	} else if ( strstr( buf, "rage pro" ) /*|| strstr( buf, "Rage Pro")*/ || strstr( buf, "ragepro" ) )     {
//#endif // RTCW_XX
//
//		glConfig.hardwareType = GLHW_RAGEPRO;
//
//#if defined RTCW_ET
//		ri.Printf( PRINT_WARNING, "WARNING: Rage Pro hardware is unsupported. Rendering errors may occur.\n" );
//#endif // RTCW_XX
//
//	} else if ( strstr( buf, "rage 128" ) )    {
//	} else if ( strstr( buf, "permedia2" ) )    {
//		glConfig.hardwareType = GLHW_PERMEDIA2;
//
//#if defined RTCW_ET
//		ri.Printf( PRINT_WARNING, "WARNING: Permedia hardware is unsupported. Rendering errors may occur.\n" );
//#endif // RTCW_XX
//
//	} else if ( strstr( buf, "riva 128" ) )    {
//		glConfig.hardwareType = GLHW_RIVA128;
//
//#if defined RTCW_ET
//		ri.Printf( PRINT_WARNING, "WARNING: Riva 128 hardware is unsupported. Rendering errors may occur.\n" );
//#endif // RTCW_XX
//
//#if !defined RTCW_SP
//	} else if ( strstr( buf, "matrox" ) )     {
//#endif // RTCW_XX
//
//	} else if ( strstr( buf, "riva tnt " ) )    {
//	}
//
//#if !defined RTCW_ET
//	if ( strstr( buf, "geforce" ) || strstr( buf, "ge-force" ) || strstr( buf, "radeon" ) || strstr( buf, "nv20" ) || strstr( buf, "nv30" )
//		 || strstr( buf, "quadro" ) ) {
//#else
//	if ( strstr( buf, "geforce3" ) ||
//		 strstr( buf, "geforce4 ti" ) ||
//		 strstr( buf, "geforce fx 5600" ) ||
//		 strstr( buf, "geforce fx 5800" ) ||
//		 strstr( buf, "radeon 8500" ) ||
//		 strstr( buf, "radeon 9000" ) ||
//		 strstr( buf, "radeon 9500" ) ||
//		 strstr( buf, "radeon 9600" ) ||
//		 strstr( buf, "radeon 9700" ) ||
//		 strstr( buf, "radeon 9800" ) ||
//		 strstr( buf, "nv20" ) ||
//		 strstr( buf, "nv30" ) ) {
//#endif // RTCW_XX
//
//		ri.Cvar_Set( "r_highQualityVideo", "1" );
//	} else {
//		ri.Cvar_Set( "r_highQualityVideo", "0" );
//	}

    glConfig.hardwareType = GLHW_GENERIC;
    ::ri.Cvar_Set ("r_highQualityVideo", "1");
//BBi

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	GLW_InitExtensions();
	WG_CheckHardwareGamma();

#if defined RTCW_ET
	// initialise default lists
	GLW_GenDefaultLists();
#endif // RTCW_XX

}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.
*/

//BBi
//void GLimp_Shutdown( void ) {
////	const char *strings[] = { "soft", "hard" };
//	const char *success[] = { "failed", "success" };
//	int retVal;
//
//	// FIXME: Brian, we need better fallbacks from partially initialized failures
//	if ( !qwglMakeCurrent ) {
//		return;
//	}
//
//	ri.Printf( PRINT_ALL, "Shutting down OpenGL subsystem\n" );
//
//	// restore gamma.  We do this first because 3Dfx's extension needs a valid OGL subsystem
//	WG_RestoreGamma();
//
//#if defined RTCW_ET
//	// delete display lists
//	GLW_DeleteDefaultLists();
//#endif // RTCW_XX
//
//	// set current context to NULL
//	if ( qwglMakeCurrent ) {
//		retVal = qwglMakeCurrent( NULL, NULL ) != 0;
//
//		ri.Printf( PRINT_ALL, "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal] );
//	}
//
//	// delete HGLRC
//	if ( glw_state.hGLRC ) {
//		retVal = qwglDeleteContext( glw_state.hGLRC ) != 0;
//		ri.Printf( PRINT_ALL, "...deleting GL context: %s\n", success[retVal] );
//		glw_state.hGLRC = NULL;
//	}
//
//	// release DC
//	if ( glw_state.hDC ) {
//		retVal = ReleaseDC( g_wv.hWnd, glw_state.hDC ) != 0;
//		ri.Printf( PRINT_ALL, "...releasing DC: %s\n", success[retVal] );
//		glw_state.hDC   = NULL;
//	}
//
//	// destroy window
//	if ( g_wv.hWnd ) {
//		ri.Printf( PRINT_ALL, "...destroying window\n" );
//		ShowWindow( g_wv.hWnd, SW_HIDE );
//		DestroyWindow( g_wv.hWnd );
//		g_wv.hWnd = NULL;
//		glw_state.pixelFormatSet = qfalse;
//	}
//
//	// close the r_logFile
//	if ( glw_state.log_fp ) {
//		fclose( glw_state.log_fp );
//		glw_state.log_fp = 0;
//	}
//
//	// reset display settings
//	if ( glw_state.cdsFullscreen ) {
//		ri.Printf( PRINT_ALL, "...resetting display\n" );
//		ChangeDisplaySettings( 0, 0 );
//		glw_state.cdsFullscreen = qfalse;
//	}
//
//	// shutdown QGL subsystem
//	QGL_Shutdown();
//
//	memset( &glConfig, 0, sizeof( glConfig ) );
//	memset( &glState, 0, sizeof( glState ) );
//}

void GLimp_Shutdown () {
    bool retVal;
    const char* success[] = { "failed", "success", };

    ::ri.Printf (PRINT_ALL, "Shutting down OpenGL subsystem\n");

    // restore gamma.  We do this first because 3Dfx's extension needs a valid OGL subsystem
    WG_RestoreGamma();

#if defined RTCW_ET
    // delete display lists
    GLW_DeleteDefaultLists();
#endif // RTCW_XX

    // set current context to NULL
    retVal = (::wglMakeCurrent (0, 0) != FALSE);
    ::ri.Printf (PRINT_ALL, "...wglMakeCurrent( NULL, NULL ): %s\n", success[retVal]);

    // delete HGLRC
    if (glw_state.hGLRC != 0) {
        retVal = (::wglDeleteContext (glw_state.hGLRC) != FALSE);
        ::ri.Printf (PRINT_ALL, "...deleting GL context: %s\n", success[retVal]);
        glw_state.hGLRC = 0;
    }

    // release DC
    if (glw_state.hDC != 0) {
        retVal = (::ReleaseDC (g_wv.hWnd, glw_state.hDC) != FALSE);
        ::ri.Printf (PRINT_ALL, "...releasing DC: %s\n", success[retVal]);
        glw_state.hDC = 0;
    }

    // destroy window
    if (g_wv.hWnd != 0) {
        ::ri.Printf (PRINT_ALL, "...destroying window\n");
        ::ShowWindow (g_wv.hWnd, SW_HIDE);
        ::DestroyWindow (g_wv.hWnd);
        g_wv.hWnd = 0;
        glw_state.pixelFormatSet = qfalse;
    }

    // reset display settings
    if (glw_state.cdsFullscreen) {
        ::ri.Printf (PRINT_ALL, "...resetting display\n");
        ::ChangeDisplaySettings (0, 0);
        glw_state.cdsFullscreen = qfalse;
    }

    ::memset (&glConfig, 0, sizeof (glConfig));
    ::memset (&glState, 0, sizeof (glState));
}
//BBi

//BBi
///*
//** GLimp_LogComment
//*/
//void GLimp_LogComment( char *comment ) {
//	if ( glw_state.log_fp ) {
//		fprintf( glw_state.log_fp, "%s", comment );
//	}
//}
//BBi


/*
===========================================================

SMP acceleration

===========================================================
*/

HANDLE renderCommandsEvent;
HANDLE renderCompletedEvent;
HANDLE renderActiveEvent;

void ( *glimpRenderThread )( void );

void GLimp_RenderThreadWrapper( void ) {
	glimpRenderThread();

	// unbind the context before we die
	::wglMakeCurrent( glw_state.hDC, NULL );
}

/*
=======================
GLimp_SpawnRenderThread
=======================
*/
HANDLE renderThreadHandle;
DWORD renderThreadId;
qboolean GLimp_SpawnRenderThread( void ( *function )( void ) ) {

	renderCommandsEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderCompletedEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	renderActiveEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

	glimpRenderThread = function;

	renderThreadHandle = CreateThread(
		NULL,   // LPSECURITY_ATTRIBUTES lpsa,
		0,      // DWORD cbStack,
		(LPTHREAD_START_ROUTINE)GLimp_RenderThreadWrapper,  // LPTHREAD_START_ROUTINE lpStartAddr,
		0,          // LPVOID lpvThreadParm,
		0,          //   DWORD fdwCreate,
		&renderThreadId );

	if ( !renderThreadHandle ) {
		return qfalse;
	}

	return qtrue;
}

static void    *smpData;
static int wglErrors;

void *GLimp_RendererSleep( void ) {
	void    *data;

	if ( !::wglMakeCurrent( glw_state.hDC, NULL ) ) {
		wglErrors++;
	}

	ResetEvent( renderActiveEvent );

	// after this, the front end can exit GLimp_FrontEndSleep
	SetEvent( renderCompletedEvent );

	WaitForSingleObject( renderCommandsEvent, INFINITE );

	if ( !::wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		wglErrors++;
	}

	ResetEvent( renderCompletedEvent );
	ResetEvent( renderCommandsEvent );

	data = smpData;

	// after this, the main thread can exit GLimp_WakeRenderer
	SetEvent( renderActiveEvent );

	return data;
}


void GLimp_FrontEndSleep( void ) {
	WaitForSingleObject( renderCompletedEvent, INFINITE );

	if ( !::wglMakeCurrent( glw_state.hDC, glw_state.hGLRC ) ) {
		wglErrors++;
	}
}


void GLimp_WakeRenderer( void *data ) {
	smpData = data;

	if ( !::wglMakeCurrent( glw_state.hDC, NULL ) ) {
		wglErrors++;
	}

	// after this, the renderer can continue through GLimp_RendererSleep
	::SetEvent( renderCommandsEvent );

	::WaitForSingleObject( renderActiveEvent, INFINITE );
}


//BBi
void GLimp_Activate (
    bool isActivated,
    bool isMinimized)
{
    if (!glConfig.isFullscreen)
        return;


    if (isActivated) {
        ::ShowWindow (g_wv.hWnd, SW_SHOW);

        if ((!glw_state.cdsFullscreen) &&
            (!glConfigEx.isNativeResolution))
        {
            DEVMODE dm;
            ::memset (&dm, 0, sizeof (DEVMODE));

            dm.dmSize = sizeof (DEVMODE);
            dm.dmPelsWidth  = glConfig.vidWidth;
            dm.dmPelsHeight = glConfig.vidHeight;
            dm.dmBitsPerPel = 32;
            dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

            ::ChangeDisplaySettings (&dm, CDS_FULLSCREEN);
        }
    } else {
        ::ShowWindow (g_wv.hWnd, SW_MINIMIZE);

        if ((!glConfigEx.isNativeResolution) &&
            (glw_state.cdsFullscreen))
        {
            glw_state.cdsFullscreen = false;
            ::ChangeDisplaySettings (0, 0);
        }
    }
}
//BBi
