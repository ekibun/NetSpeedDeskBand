#include <windows.h>
#include <Wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include "DeskBand.h"
#include "resource.h"

#include <tchar.h>
#include <stdio.h>

//#include <winsock2.h>
//#include <ws2tcpip.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <Iphlpapi.h>

#include <time.h>      
int getCurrentTime()
{
	SYSTEMTIME utc_time = { 0 };
	GetSystemTime(&utc_time);
	return utc_time.wMilliseconds + 1000 * utc_time.wSecond;
}


#define RECTWIDTH(x)   ((x).right - (x).left)
#define RECTHEIGHT(x)  ((x).bottom - (x).top)

extern ULONG        g_cDllRef;
extern HINSTANCE    g_hInst;
extern bool         g_canUnload;


extern CLSID CLSID_NetSpeed;
static const WCHAR g_szDeskBandSampleClass[] = L"DeskBandSampleClass";

CDeskBand::CDeskBand() :
	m_cRef(1), m_pSite(NULL), m_fHasFocus(FALSE), m_fIsDirty(FALSE), m_dwBandID(0), m_hwnd(NULL), m_hwndParent(NULL)
{
}

CDeskBand::~CDeskBand()
{
	if (m_pSite)
	{
		m_pSite->Release();
	}
}

//
// IUnknown
//
STDMETHODIMP CDeskBand::QueryInterface(REFIID riid, void **ppv)
{
	HRESULT hr = S_OK;

	if (IsEqualIID(IID_IUnknown, riid) ||
		IsEqualIID(IID_IOleWindow, riid) ||
		IsEqualIID(IID_IDockingWindow, riid) ||
		IsEqualIID(IID_IDeskBand, riid) ||
		IsEqualIID(IID_IDeskBand2, riid))
	{
		*ppv = static_cast<IOleWindow *>(this);
	}
	else if (IsEqualIID(IID_IPersist, riid) ||
		IsEqualIID(IID_IPersistStream, riid))
	{
		*ppv = static_cast<IPersist *>(this);
	}
	else if (IsEqualIID(IID_IObjectWithSite, riid))
	{
		*ppv = static_cast<IObjectWithSite *>(this);
	}
	else if (IsEqualIID(IID_IInputObject, riid))
	{
		*ppv = static_cast<IInputObject *>(this);
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
	}

	if (*ppv)
	{
		AddRef();
	}

	return hr;
}

STDMETHODIMP_(ULONG) CDeskBand::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDeskBand::Release()
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
	{
		delete this;
	}
	return cRef;
}

//
// IOleWindow
//
STDMETHODIMP CDeskBand::GetWindow(HWND *phwnd)
{
	*phwnd = m_hwnd;
	return S_OK;
}

STDMETHODIMP CDeskBand::ContextSensitiveHelp(BOOL)
{
	return E_NOTIMPL;
}

//
// IDockingWindow
//
STDMETHODIMP CDeskBand::ShowDW(BOOL fShow)
{
	if (m_hwnd)
	{
		ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);
	}

	return S_OK;
}

STDMETHODIMP CDeskBand::CloseDW(DWORD)
{
	if (m_hwnd)
	{
		ShowWindow(m_hwnd, SW_HIDE);
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}

	return S_OK;
}

STDMETHODIMP CDeskBand::ResizeBorderDW(const RECT *, IUnknown *, BOOL)
{
	return E_NOTIMPL;
}

//
// IDeskBand
//
STDMETHODIMP CDeskBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
	HRESULT hr = E_INVALIDARG;

	if (pdbi)
	{
		m_dwBandID = dwBandID;

		POINT pt;

		pt.x = dwViewMode == DBIF_VIEWMODE_NORMAL ? 70 : 35;
		pt.y = dwViewMode == DBIF_VIEWMODE_NORMAL ? 35 : 70;

		if (pdbi->dwMask & DBIM_MINSIZE)
		{
			pdbi->ptMinSize.x = pt.x;
			pdbi->ptMinSize.y = pt.y;
		}

		if (pdbi->dwMask & DBIM_MAXSIZE)
		{
			pdbi->ptMaxSize.y = -1;
		}

		if (pdbi->dwMask & DBIM_INTEGRAL)
		{
			pdbi->ptIntegral.y = 1;
		}

		if (pdbi->dwMask & DBIM_ACTUAL)
		{
			pdbi->ptActual.x = pt.x;
			pdbi->ptActual.y = pt.y;
		}

		if (pdbi->dwMask & DBIM_TITLE)
		{
			// Don't show title by removing this flag.
			pdbi->dwMask &= ~DBIM_TITLE;
		}

		if (pdbi->dwMask & DBIM_MODEFLAGS)
		{
			pdbi->dwModeFlags = DBIMF_NORMAL | DBIMF_VARIABLEHEIGHT;
		}

		if (pdbi->dwMask & DBIM_BKCOLOR)
		{
			// Use the default background color by removing this flag.
			pdbi->dwMask &= ~DBIM_BKCOLOR;
		}

		hr = S_OK;
	}

	return hr;
}

//
// IDeskBand2
//
STDMETHODIMP CDeskBand::CanRenderComposited(BOOL *pfCanRenderComposited)
{
	*pfCanRenderComposited = TRUE;

	return S_OK;
}

STDMETHODIMP CDeskBand::SetCompositionState(BOOL fCompositionEnabled)
{
	m_fCompositionEnabled = fCompositionEnabled;

	//InvalidateRect(m_hwnd, NULL, TRUE);
	//UpdateWindow(m_hwnd);

	return S_OK;
}

STDMETHODIMP CDeskBand::GetCompositionState(BOOL *pfCompositionEnabled)
{
	*pfCompositionEnabled = m_fCompositionEnabled;

	return S_OK;
}

//
// IPersist
//
STDMETHODIMP CDeskBand::GetClassID(CLSID *pclsid)
{
	*pclsid = CLSID_NetSpeed;
	return S_OK;
}

//
// IPersistStream
//
STDMETHODIMP CDeskBand::IsDirty()
{
	return m_fIsDirty ? S_OK : S_FALSE;
}

STDMETHODIMP CDeskBand::Load(IStream * /*pStm*/)
{
	return S_OK;
}

STDMETHODIMP CDeskBand::Save(IStream * /*pStm*/, BOOL fClearDirty)
{
	if (fClearDirty)
	{
		m_fIsDirty = FALSE;
	}

	return S_OK;
}

STDMETHODIMP CDeskBand::GetSizeMax(ULARGE_INTEGER * /*pcbSize*/)
{
	return E_NOTIMPL;
}

//
// IObjectWithSite
//
STDMETHODIMP CDeskBand::SetSite(IUnknown *pUnkSite)
{
	HRESULT hr = S_OK;

	m_hwndParent = NULL;

	if (m_pSite)
	{
		m_pSite->Release();
	}

	if (pUnkSite)
	{
		IOleWindow *pow;
		hr = pUnkSite->QueryInterface(IID_IOleWindow, reinterpret_cast<void **>(&pow));
		if (SUCCEEDED(hr))
		{
			hr = pow->GetWindow(&m_hwndParent);
			if (SUCCEEDED(hr))
			{
				WNDCLASSW wc = { 0 };
				wc.style = CS_HREDRAW | CS_VREDRAW;
				wc.hCursor = LoadCursor(NULL, IDC_ARROW);
				wc.hInstance = g_hInst;
				wc.lpfnWndProc = WndProc;
				wc.lpszClassName = g_szDeskBandSampleClass;
				wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 0));

				RegisterClassW(&wc);

				CreateWindowExW(0,
					g_szDeskBandSampleClass,
					NULL,
					WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
					0,
					0,
					0,
					0,
					m_hwndParent,
					NULL,
					g_hInst,
					this);

				if (!m_hwnd)
				{
					hr = E_FAIL;
				}
			}

			pow->Release();
		}

		hr = pUnkSite->QueryInterface(IID_IInputObjectSite, reinterpret_cast<void **>(&m_pSite));
	}

	return hr;
}

STDMETHODIMP CDeskBand::GetSite(REFIID riid, void **ppv)
{
	HRESULT hr = E_FAIL;

	if (m_pSite)
	{
		hr = m_pSite->QueryInterface(riid, ppv);
	}
	else
	{
		*ppv = NULL;
	}

	return hr;
}

//
// IInputObject
//
STDMETHODIMP CDeskBand::UIActivateIO(BOOL fActivate, MSG *)
{
	if (fActivate)
	{
		SetFocus(m_hwnd);
	}

	return S_OK;
}

STDMETHODIMP CDeskBand::HasFocusIO()
{
	return m_fHasFocus ? S_OK : S_FALSE;
}

STDMETHODIMP CDeskBand::TranslateAcceleratorIO(MSG *)
{
	return S_FALSE;
};

void CDeskBand::OnFocus(const BOOL fFocus)
{
	m_fHasFocus = fFocus;

	if (m_pSite)
	{
		m_pSite->OnFocusChangeIS(static_cast<IOleWindow*>(this), m_fHasFocus);
	}
}











BOOL Ping(LPCTSTR szTarget, int &dwTime)
{
	BOOL bSuccess = FALSE;
	//dwTime = INFINITE;

	if (szTarget == NULL)
	{
		//Debug("Target Is NULL\n");
		return FALSE;
	}

	TCHAR szCmdLine[80];
	if (_sntprintf_s(szCmdLine, sizeof(szCmdLine) / sizeof(TCHAR),
		_T("ping -n 1 %s"), szTarget) == sizeof(szCmdLine) / sizeof(TCHAR))
	{
		//Debug("Target Is Too Long\n");
		return FALSE;
	}

	HANDLE hWritePipe = NULL;
	HANDLE hReadPipe = NULL;
	HANDLE hWriteShell = NULL;
	HANDLE hReadShell = NULL;

	SECURITY_ATTRIBUTES  sa;
	memset(&sa, 0, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (CreatePipe(&hReadPipe, &hReadShell, &sa, 0)
		&& CreatePipe(&hWriteShell, &hWritePipe, &sa, 0))
	{
		STARTUPINFO            si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.hStdInput = hWriteShell;
		si.hStdOutput = hReadShell;
		si.hStdError = hReadShell;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION    pi;
		memset(&pi, 0, sizeof(pi));

		int nAvg = -1;
		if (CreateProcess(NULL, szCmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
		{
			if (WaitForSingleObject(pi.hProcess, 10000) == WAIT_OBJECT_0)
			{
				char szBuffer[1024];
				DWORD dwBytes;
				if (ReadFile(hReadPipe, szBuffer, sizeof(szBuffer), &dwBytes, NULL))
				{
					szBuffer[dwBytes] = '\0';
					//Debug("%s\n", szBuffer);
					char* ptStr = strrchr(szBuffer, '=');
					//Debug("%s\n", ptStr);
					//LPTSTR lpszTime = NULL;
					if (ptStr > 0) {
						sscanf_s(ptStr + 2, "%d", &nAvg);
						//Debug("%d %d\n", dwTime, c);
						if (!strchr(ptStr, '%')) {
							dwTime = nAvg;
							bSuccess = TRUE;
						}
					}

				}
			}
			else
			{
				//Debug("Process(%d) is Time Out\n", pi.dwProcessId);
				TerminateProcess(pi.hProcess, 0);
			}

			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	}

	if (!bSuccess)
		dwTime = -1;

	if (hWritePipe != NULL)
		CloseHandle(hWritePipe);
	if (hReadPipe != NULL)
		CloseHandle(hReadPipe);
	if (hWriteShell != NULL)
		CloseHandle(hWriteShell);
	if (hReadShell != NULL)
		CloseHandle(hReadShell);

	return bSuccess;
}

int up, down, ping;

DWORD WINAPI pingThread(LPVOID lpParamter)
{
	while (IsWindow((HWND)lpParamter)) {

		//Debug("pingThread-begin\n");
		//try {
		Ping(L"baidu.com", ping);

		//Debug("pingThread:%d\n",ping);

		InvalidateRect((HWND)lpParamter, NULL, false);
		//}
		//catch (EXCEPINFO & e) {
		//	//Debug("pingThread - %s\n", e.bstrDescription);
		//}
		Sleep(2000);
	}
	return 0;
}

DWORD WINAPI speedThread(LPVOID lpParamter)
{
	//TODO
	MIB_IFTABLE *m_pMIT = NULL, *o_pMIT = NULL;

	int otime = 0;
	DWORD dwIfBufSize = sizeof(MIB_IFTABLE);
	BOOL bRet;

	while (IsWindow((HWND)lpParamter)) {

		//Debug("speedThread-begin\n");

		m_pMIT = new MIB_IFTABLE[dwIfBufSize];

		bRet = ::GetIfTable(m_pMIT, &dwIfBufSize, 0);
		if (bRet == ERROR_INSUFFICIENT_BUFFER)//如果内存不足，则重新分配内存大小  
		{
			if (dwIfBufSize != NULL)
				delete[] m_pMIT;
			m_pMIT = new MIB_IFTABLE[dwIfBufSize];
		}
		bRet = ::GetIfTable(m_pMIT, &dwIfBufSize, 0);

		if (o_pMIT) {
			if (bRet == NO_ERROR && m_pMIT->dwNumEntries > 1 && o_pMIT->dwNumEntries > 1)
			{
				//system("cls");

				int time = getCurrentTime();
				//printf_s("%d-%d---------\n", otime, time);

				int span = otime > time ? 60000 + time - otime : time - otime;
				//printf_s("Time Span=%d\n", span);
				otime = time;

				int maxUpSpeed = 0;
				int maxDownSpeed = 0;

				for (int i = 0; i < (int)(m_pMIT->dwNumEntries); i++)
				{
					if (m_pMIT->table[i].dwOperStatus > 2) {
						for (int j = 0; j < (int)(o_pMIT->dwNumEntries); j++)
						{
							if (o_pMIT->table[j].dwIndex == m_pMIT->table[i].dwIndex) {
								int up = 0, down = 0;
								if (m_pMIT->table[i].dwOutOctets > o_pMIT->table[j].dwOutOctets)
									up = m_pMIT->table[i].dwOutOctets - o_pMIT->table[j].dwOutOctets;
								if (m_pMIT->table[i].dwInOctets > o_pMIT->table[j].dwInOctets)
									down = m_pMIT->table[i].dwInOctets - o_pMIT->table[j].dwInOctets;
								//printf_s("%d-%d\n", m_pMIT->table[i].dwOutOctets, o_pMIT->table[j].dwOutOctets);

								maxUpSpeed = max(maxUpSpeed, up);
								maxDownSpeed = max(maxDownSpeed, down);
								break;
							}
						}
					}
				}
				if (span) {
					up = maxUpSpeed / 1024 * 1000 / span;
					down = maxDownSpeed / 1024 * 1000 / span;
				}
				//ping = Ping(L"baidu.com ");

				//printf_s("Upload:%dKB/s\nDownload:%dKB/s\n", maxUpSpeed / 1024 * 1000 / span,
				//	maxDownSpeed / 1024 * 1000 / span);
				//printf_s("ping:%dms\n", ping);
			}
		}
		delete[] o_pMIT;
		o_pMIT = m_pMIT;

		//Debug("speedThread-up:%d,down:%d\n", up, down);

		InvalidateRect((HWND)lpParamter, NULL, false);

		Sleep(1000);
	}
	delete[] o_pMIT;

	return 0;
}













bool ImageFromIDResource(UINT nID, LPCTSTR sTR, Image *&pImg)
{
	HINSTANCE hInst = g_hInst;
	HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(nID), sTR); // type
	if (!hRsrc)
		return FALSE;
	// load resource into memory
	DWORD len = SizeofResource(hInst, hRsrc);
	BYTE* lpRsrc = (BYTE*)LoadResource(hInst, hRsrc);
	if (!lpRsrc)
		return FALSE;
	// Allocate global memory on which to create stream
	HGLOBAL m_hMem = GlobalAlloc(GMEM_FIXED, len);
	BYTE* pmem = (BYTE*)GlobalLock(m_hMem);
	memcpy(pmem, lpRsrc, len);
	GlobalUnlock(m_hMem);
	IStream* pstm;
	CreateStreamOnHGlobal(m_hMem, FALSE, &pstm);
	// load from stream
	pImg = Gdiplus::Image::FromStream(pstm);
	// free/release stuff
	pstm->Release();
	FreeResource(lpRsrc);
	GlobalFree(m_hMem);
	return TRUE;
}

void DrawString(Graphics& g, PWCHAR string, RectF rect, int size, StringAlignment sta)
{
	LOGFONTW lf;
	SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(LOGFONTW), &lf, 0);
	SolidBrush  solidBrush(Color::White);
	FontFamily  fontFamily(lf.lfFaceName);
	Font        font(&fontFamily, (REAL)size, FontStyleRegular, UnitPixel);

	StringFormat sf(StringFormatFlags::StringFormatFlagsNoWrap);

	sf.SetAlignment(sta);
	sf.SetLineAlignment(StringAlignmentCenter);

	g.DrawString((WCHAR*)string, (INT)wcslen(string), &font, rect, &sf, &solidBrush);
}

//IStream * CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType)
//{
//	// initialize return value
//	IStream * ipStream = NULL;
//
//	// find the resource
//	HRSRC hrsrc = FindResource(NULL, lpName, lpType);
//	if (hrsrc == NULL)
//		goto Return;
//
//	// load the resource
//	DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
//	HGLOBAL hglbImage = LoadResource(NULL, hrsrc);
//	if (hglbImage == NULL)
//		goto Return;
//
//	// lock the resource, getting a pointer to its data
//	LPVOID pvSourceResourceData = LockResource(hglbImage);
//	if (pvSourceResourceData == NULL)
//		goto Return;
//
//	// allocate memory to hold the resource data
//	HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
//	if (hgblResourceData == NULL)
//		goto Return;
//
//	// get a pointer to the allocated memory
//	LPVOID pvResourceData = GlobalLock(hgblResourceData);
//	if (pvResourceData == NULL)
//		goto FreeData;
//
//	// copy the data from the resource to the new memory block
//	CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
//	GlobalUnlock(hgblResourceData);
//
//	// create a stream on the HGLOBAL containing the data
//	if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
//		goto Return;
//
//FreeData:
//	// couldn't create stream; free the memory
//	GlobalFree(hgblResourceData);
//
//Return:
//	// no need to unlock or free the resource
//	return ipStream;
//}
//
//IWICBitmapSource * LoadBitmapFromStream(IStream * ipImageStream)
//{
//	// initialize return value
//	IWICBitmapSource * ipBitmap = NULL;
//
//	// load WIC's PNG decoder
//	IWICBitmapDecoder * ipDecoder = NULL;
//	if (FAILED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder))))
//		goto Return;
//
//	// load the PNG
//	if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
//		goto ReleaseDecoder;
//
//	// check for the presence of the first frame in the bitmap
//	UINT nFrameCount = 0;
//	if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
//		goto ReleaseDecoder;
//
//	// load the first frame (i.e., the image)
//	IWICBitmapFrameDecode * ipFrame = NULL;
//	if (FAILED(ipDecoder->GetFrame(0, &ipFrame)))
//		goto ReleaseDecoder;
//
//	// convert the image to 32bpp BGRA format with pre-multiplied alpha
//	//   (it may not be stored in that format natively in the PNG resource,
//	//   but we need this format to create the DIB to use on-screen)
//	WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
//	ipFrame->Release();
//
//ReleaseDecoder:
//	ipDecoder->Release();
//Return:
//	return ipBitmap;
//}
//
//HBITMAP CreateHBITMAP(IWICBitmapSource * ipBitmap)
//{
//	// initialize return value
//	HBITMAP hbmp = NULL;
//
//	// get image attributes and check for valid image
//	UINT width = 0;
//	UINT height = 0;
//	if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
//		goto Return;
//
//	// prepare structure giving bitmap information (negative height indicates a top-down DIB)
//	BITMAPINFO bminfo;
//	ZeroMemory(&bminfo, sizeof(bminfo));
//	bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//	bminfo.bmiHeader.biWidth = width;
//	bminfo.bmiHeader.biHeight = -((LONG)height);
//	bminfo.bmiHeader.biPlanes = 1;
//	bminfo.bmiHeader.biBitCount = 32;
//	bminfo.bmiHeader.biCompression = BI_RGB;
//
//	// create a DIB section that can hold the image
//	void * pvImageBits = NULL;
//	HDC hdcScreen = GetDC(NULL);
//	hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
//	ReleaseDC(NULL, hdcScreen);
//	if (hbmp == NULL)
//		goto Return;
//
//	// extract the image into the HBITMAP
//	const UINT cbStride = width * 4;
//	const UINT cbImage = cbStride * height;
//	if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE *>(pvImageBits))))
//	{
//		// couldn't extract image; delete HBITMAP
//		DeleteObject(hbmp);
//		hbmp = NULL;
//	}
//
//Return:
//	return hbmp;
//}
//
//HBITMAP LoadSplashImage()
//{
//	HBITMAP hbmpSplash = NULL;
//
//	// load the PNG image data into a stream
//	IStream * ipImageStream = CreateStreamOnResource(MAKEINTRESOURCE(IDB_PNG1), _T("PNG"));
//	if (ipImageStream == NULL)
//		goto Return;
//
//	// load the bitmap with WIC
//	IWICBitmapSource * ipBitmap = LoadBitmapFromStream(ipImageStream);
//	if (ipBitmap == NULL)
//		goto ReleaseStream;
//
//	// create a HBITMAP containing the image
//	hbmpSplash = CreateHBITMAP(ipBitmap);
//	ipBitmap->Release();
//
//ReleaseStream:
//	ipImageStream->Release();
//Return:
//	return hbmpSplash;
//}


void CDeskBand::OnPaint(const HDC hdcIn)
{
	if (g_canUnload)
		return;

	HDC hdc = hdcIn;
	PAINTSTRUCT ps;
	//static WCHAR szContent[] = L"DeskBand Sample";
	//static WCHAR szContentGlass[] = L"DeskBand Sample (Glass)";

	if (!hdc)
	{
		hdc = BeginPaint(m_hwnd, &ps);
	}

	if (hdc)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		SIZE size;

		if (m_fCompositionEnabled)
		{

			HTHEME hTheme = OpenThemeData(NULL, L"BUTTON");
			if (hTheme)
			{

				HDC hdcPaint = NULL;
				HPAINTBUFFER hBufferedPaint = BeginBufferedPaint(hdc, &rc, BPBF_TOPDOWNDIB, NULL, &hdcPaint);

				Rect rect = Rect(rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc));

				Graphics g(hdcPaint);

				g.Clear(Color::Transparent);
				WCHAR str[100];
				swprintf_s(str, L"%dkB/s\n%dkB/s", up, down);
				DrawString(g, str, RectF(0, 0, rect.Width - (REAL)15, rect.Height - (REAL)5), 10, StringAlignmentFar);

				swprintf_s(str, L"%dms", ping);
				DrawString(g, str, RectF(0, rect.Height - (REAL)8, (REAL)rect.Width, (REAL)8), 8, StringAlignmentNear);

				if(!pimage)
					ImageFromIDResource(IDB_PNG1, _T("PNG"), pimage);

				if (pimage)
					g.DrawImage(pimage, rect.Width - 25, (rect.Height - 5 - 32) / 2, 32, 32);
				
				EndBufferedPaint(hBufferedPaint, TRUE);

				CloseThemeData(hTheme);
			}
		}
		else
		{
			//SetBkColor(hdc, RGB(255, 255, 0));
			//GetTextExtentPointW(hdc, szContent, ARRAYSIZE(szContent), &size);
			//TextOutW(hdc,
			//	(RECTWIDTH(rc) - size.cx) / 2,
			//	(RECTHEIGHT(rc) - size.cy) / 2,
			//	szContent,
			//	ARRAYSIZE(szContent));
		}
	}

	if (!hdcIn)
	{
		EndPaint(m_hwnd, &ps);
	}
}

LRESULT CALLBACK CDeskBand::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult = 0;

	CDeskBand *pDeskBand = reinterpret_cast<CDeskBand *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (uMsg)
	{
	case WM_CREATE:
		g_canUnload = false;
		{
			static ULONG_PTR gdiplusToken;
			if (!gdiplusToken) {
				GdiplusStartupInput gdiplusStartupInput;
				GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
			}

			//创建ping线程
			HANDLE hThread = CreateThread(NULL, 0, pingThread, hwnd, 0, NULL);
			CloseHandle(hThread);
			//创建NetSpeed线程
			hThread = CreateThread(NULL, 0, speedThread, hwnd, 0, NULL);
			CloseHandle(hThread);
		}
		pDeskBand = reinterpret_cast<CDeskBand *>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
		pDeskBand->m_hwnd = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pDeskBand));
		break;
	case WM_DESTROY:
		g_canUnload = true;
		//GdiplusShutdown(gdiplusToken);
		break;
	case WM_SETFOCUS:
		pDeskBand->OnFocus(TRUE);
		break;

	case WM_KILLFOCUS:
		pDeskBand->OnFocus(FALSE);
		break;

	case WM_PAINT:
		pDeskBand->OnPaint(NULL);
		break;

	case WM_PRINTCLIENT:
		pDeskBand->OnPaint(reinterpret_cast<HDC>(wParam));
		break;

	case WM_ERASEBKGND:
		if (pDeskBand->m_fCompositionEnabled)
		{
			lResult = 1;
		}
		break;
	}

	if (uMsg != WM_ERASEBKGND)
	{
		lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return lResult;
}
