#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "MHDPreviewHandler.h"
#include "libmhd.h"

#pragma comment (lib, "shlwapi.lib")

static const CLSID CLSID_PreviewHandlerSample =
{ 0x82a02ea0, 0x8766, 0x4a02, { 0xbd, 0x8d, 0x91, 0x7, 0xa, 0x2b, 0x85, 0x6b } };
const TCHAR g_szClsid[] = TEXT("{82A02EA0-8766-4A02-BD8D-91070A2B856B}");
const TCHAR g_szHandlerName[] = TEXT("MHD Preview Handler");
const TCHAR* g_szExts[] = { TEXT(".mhd"), TEXT(".mha") };

LONG      g_lLocks = 0;
HINSTANCE g_hinstDll = NULL;

void LockModule(BOOL bLock);
BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue, LPTSTR lpszData);


// CPreviewHandler


CPreviewHandler::CPreviewHandler()
{
	m_cRef = 1;
	m_hwndParent = NULL;
	m_hwndPreview = NULL;
	m_pSite = NULL;
  m_pStream = NULL;

	LockModule(TRUE);
}

CPreviewHandler::~CPreviewHandler()
{

	LockModule(FALSE);
}

STDMETHODIMP CPreviewHandler::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IPreviewHandler))
		*ppvObject = static_cast<IPreviewHandler *>(this);
	else if (IsEqualIID(riid, IID_IObjectWithSite))
		*ppvObject = static_cast<IObjectWithSite *>(this);
	else if (IsEqualIID(riid, IID_IOleWindow))
		*ppvObject = static_cast<IOleWindow *>(this);
	else if (IsEqualIID(riid, IID_IInitializeWithStream))
		*ppvObject = static_cast<IInitializeWithStream *>(this);
	else
		return E_NOINTERFACE;

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CPreviewHandler::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CPreviewHandler::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0) {
		delete this;
		return 0;
	}

	return m_cRef;
}

STDMETHODIMP CPreviewHandler::SetWindow(HWND hwnd, const RECT *prc)
{
  if (hwnd && prc) {
    m_hwndParent = hwnd;
    m_rc = *prc;

    if (m_hwndPreview != NULL) {
      SetParent(m_hwndPreview, m_hwndParent);
      SetWindowPos(m_hwndPreview, NULL, m_rc.left, m_rc.top, m_rc.right - m_rc.left, m_rc.bottom - m_rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
  }

	return S_OK;
}

STDMETHODIMP CPreviewHandler::SetRect(const RECT *prc)
{
	m_rc = *prc;

	if (m_hwndPreview != NULL)
		SetWindowPos(m_hwndPreview, NULL, m_rc.left, m_rc.top, m_rc.right - m_rc.left, m_rc.bottom - m_rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	return S_OK;
}
std::string correct_newline(const std::string &lines)
{
  constexpr int buf_size = 2048;
  std::vector<char> buf(buf_size);
  char *p_buf = buf.data();
  for (int i = 0; i < buf_size && i < lines.size(); ++i) {
    if (lines[i] == '\n' && i > 0 && lines[i-1] != '\r') {
      *p_buf = '\r';
      ++p_buf;
      *p_buf = '\n';
      ++p_buf;
    } else {
      *p_buf = lines[i];
      ++p_buf;
    }
  }
  *p_buf = '\0';
  return buf.data();
}
STDMETHODIMP CPreviewHandler::DoPreview(VOID)
{
  if (m_hwndPreview != NULL || m_pStream == NULL)
    return E_FAIL;

  constexpr int buf_size = 2048;
  auto header = read_header(m_pStream);
  header = correct_newline(header);
  std::vector<wchar_t> w_header(buf_size);
  if (header.empty()) {
    mbstowcs(w_header.data(), "Empty file.", buf_size);
  } else {
    mbstowcs(w_header.data(), header.c_str(), buf_size);
  }
  m_hwndPreview = CreateWindowEx(0, TEXT("EDIT"), w_header.data(), WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL, m_rc.left, m_rc.top, m_rc.right - m_rc.left, m_rc.bottom - m_rc.top, m_hwndParent, 0, 0, NULL);

	if (m_hwndPreview == NULL)
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

STDMETHODIMP CPreviewHandler::Unload(VOID)
{
	if (m_hwndPreview != NULL) {
		DestroyWindow(m_hwndPreview);
		m_hwndPreview = NULL;
	}
  if (m_pStream) {
    m_pStream->Release();
    m_pStream = NULL;
  }
  if (m_pSite) {
    m_pSite->Release();
    m_pSite = NULL;
  }


	return S_OK;
}

STDMETHODIMP CPreviewHandler::SetFocus(VOID)
{
	if (m_hwndPreview == NULL)
		return S_FALSE;

	::SetFocus(m_hwndPreview);

	return S_OK;
}

STDMETHODIMP CPreviewHandler::QueryFocus(HWND *phwnd)
{
	HRESULT hr = E_INVALIDARG;

	if (phwnd != NULL) {
		*phwnd = GetFocus();
		if (*phwnd != NULL)
			hr = S_OK;
		else
			hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}

STDMETHODIMP CPreviewHandler::TranslateAccelerator(MSG *pmsg)
{
  HRESULT hr = S_FALSE;
  IPreviewHandlerFrame *pFrame = NULL;
  if (m_pSite && SUCCEEDED(m_pSite->QueryInterface(&pFrame)))
  {
    // If your previewer has multiple tab stops, you will need to do 
    // appropriate first/last child checking. This sample previewer has 
    // no tabstops, so we want to just forward this message out.
    hr = pFrame->TranslateAccelerator(pmsg);

    pFrame->Release();
  }
  return hr;
}

STDMETHODIMP CPreviewHandler::SetSite(IUnknown *pUnkSite)
{
	if (m_pSite != NULL) {
		m_pSite->Release();
		m_pSite = NULL;
	}

  return pUnkSite ? pUnkSite->QueryInterface(&m_pSite) : S_OK;
}

STDMETHODIMP CPreviewHandler::GetSite(REFIID riid, void **ppvSite)
{
	*ppvSite = m_pSite;

	return S_OK;
}

STDMETHODIMP CPreviewHandler::GetWindow(HWND *phwnd)
{
	if (phwnd == NULL)
		return E_INVALIDARG;

	*phwnd = m_hwndParent;

	return S_OK;
}

STDMETHODIMP CPreviewHandler::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

STDMETHODIMP CPreviewHandler::Initialize(IStream *pstream, DWORD grfMode)
{

  HRESULT hr = E_INVALIDARG;
  if (pstream)
  {
    // Initialize can be called more than once, so release existing valid 
    // m_pStream.
    if (m_pStream)
    {
      m_pStream->Release();
      m_pStream = NULL;
    }

    m_pStream = pstream;
    m_pStream->AddRef();
    hr = S_OK;
  }
  return hr;
}


// CClassFactory


STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
		*ppvObject = static_cast<IClassFactory *>(this);
	else
		return E_NOINTERFACE;

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
	LockModule(TRUE);

	return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
	LockModule(FALSE);

	return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	CPreviewHandler *p;
	HRESULT         hr;

	*ppvObject = NULL;

	if (pUnkOuter != NULL)
		return CLASS_E_NOAGGREGATION;

	p = new CPreviewHandler();
	if (p == NULL)
		return E_OUTOFMEMORY;

	hr = p->QueryInterface(riid, ppvObject);
	p->Release();

	return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
	LockModule(fLock);

	return S_OK;
}


// DLL Export


STDAPI DllCanUnloadNow()
{
	return g_lLocks == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	static CClassFactory serverFactory;
	HRESULT hr;

	*ppv = NULL;

	if (IsEqualCLSID(rclsid, CLSID_PreviewHandlerSample))
		hr = serverFactory.QueryInterface(riid, ppv);
	else
		hr = CLASS_E_CLASSNOTAVAILABLE;

	return hr;
}

STDAPI DllRegisterServer(void)
{
	TCHAR szModulePath[MAX_PATH];
	TCHAR szKey[256];

	wsprintf(szKey, TEXT("CLSID\\%s"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, TEXT("MHD Shell Extension")))
		return E_FAIL;

	GetModuleFileName(g_hinstDll, szModulePath, sizeof(szModulePath) / sizeof(TCHAR));
	wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, szModulePath))
		return E_FAIL;

	wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, TEXT("ThreadingModel"), TEXT("Apartment")))
		return E_FAIL;

	wsprintf(szKey, TEXT("CLSID\\%s"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, TEXT("AppID"), TEXT("{6d2b5079-2f0b-48dd-ab7f-97cec514d30b}")))
		return E_FAIL;

  for (const auto &g_szExt : g_szExts) {
    wsprintf(szKey, TEXT("%s\\shellex\\{8895b1c6-b41f-4c1c-a562-0d564250836f}"), g_szExt);
    if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, (LPTSTR)g_szClsid))
      return E_FAIL;
  }

	wsprintf(szKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers"));
	if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, szKey, (LPTSTR)g_szClsid, (LPTSTR)g_szHandlerName))
		return E_FAIL;

	return S_OK;
}

STDAPI DllUnregisterServer(void)
{
	TCHAR szKey[256];

	wsprintf(szKey, TEXT("CLSID\\%s"), g_szClsid);
	SHDeleteKey(HKEY_CLASSES_ROOT, szKey);

  for (const auto &g_szExt : g_szExts) {
    SHDeleteKey(HKEY_CLASSES_ROOT, g_szExt);
  }

	SHDeleteValue(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PreviewHandlers"), (LPTSTR)g_szClsid);

	return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {

	case DLL_PROCESS_ATTACH:
		g_hinstDll = hinstDll;
		DisableThreadLibraryCalls(hinstDll);
		return TRUE;

	}

	return TRUE;
}


// Function


void LockModule(BOOL bLock)
{
	if (bLock)
		InterlockedIncrement(&g_lLocks);
	else
		InterlockedDecrement(&g_lLocks);
}

BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue, LPTSTR lpszData)
{
	HKEY  hKey;
	LONG  lResult;
	DWORD dwSize;

	lResult = RegCreateKeyEx(hKeyRoot, lpszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
	if (lResult != ERROR_SUCCESS)
		return FALSE;

	if (lpszData != NULL)
		dwSize = (lstrlen(lpszData) + 1) * sizeof(TCHAR);
	else
		dwSize = 0;

	RegSetValueEx(hKey, lpszValue, 0, REG_SZ, (LPBYTE)lpszData, dwSize);
	RegCloseKey(hKey);

	return TRUE;
}
