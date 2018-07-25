#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include "MHDInfotip.h"
#include "libmhd.h"

#pragma comment (lib, "shlwapi.lib")

static const CLSID CLSID_Infotip =
{ 0x7d0dfea6, 0x324e, 0x4d87, { 0x98, 0x83, 0xa5, 0x2f, 0x79, 0x42, 0xb5, 0x20 } };
const TCHAR g_szClsid[] = TEXT("{7D0DFEA6-324E-4D87-9883-A52F7942B520}");
const TCHAR* g_szExts[] = { TEXT(".mhd"), TEXT(".mha") };

LONG      g_lLocks = 0;
HINSTANCE g_hinstDll = NULL;

void LockModule(BOOL bLock);
BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue, LPTSTR lpszData);


// CQueryInfo


CQueryInfo::CQueryInfo()
{
	m_cRef = 1;

	LockModule(TRUE);
}

CQueryInfo::~CQueryInfo()
{
	LockModule(FALSE);
}

STDMETHODIMP CQueryInfo::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IQueryInfo))
		*ppvObject = static_cast<IQueryInfo *>(this);
	else if (IsEqualIID(riid, IID_IPersist) || IsEqualIID(riid, IID_IPersistFile))
		*ppvObject = static_cast<IPersistFile *>(this);
	else
		return E_NOINTERFACE;

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CQueryInfo::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CQueryInfo::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0) {
		delete this;
		return 0;
	}

	return m_cRef;
}

#include <string>
STDMETHODIMP CQueryInfo::GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip)
{
	if (!ppwszTip)
		return E_POINTER;

    constexpr int buf_size = 4096;
    try {
      setlocale(LC_ALL, "");
      auto header = read_header(m_szInfotip);
      std::vector<wchar_t> w_header(buf_size);
      if (header.empty()) {
        mbstowcs(w_header.data(), "Empty file.", buf_size);
      } else {
        mbstowcs(w_header.data(), header.c_str(), buf_size);
      }
      *ppwszTip = (LPWSTR)CoTaskMemAlloc((lstrlenW(w_header.data()) + 1) * sizeof(WCHAR));
      lstrcpyW(*ppwszTip, w_header.data());
      return S_OK;
    } catch (std::exception &e) {
      // show error
      std::vector<wchar_t> w_error(buf_size);
      mbstowcs(w_error.data(), e.what(), buf_size);
      *ppwszTip = (LPWSTR)CoTaskMemAlloc((lstrlenW(w_error.data()) + 1) * sizeof(WCHAR));
      lstrcpyW(*ppwszTip, w_error.data());
      return S_OK;
    }

	return S_OK;
}

STDMETHODIMP CQueryInfo::GetInfoFlags(DWORD *pdwFlags)
{
	return E_NOTIMPL;
}

STDMETHODIMP CQueryInfo::GetClassID(CLSID *pClassID)
{
	*pClassID = CLSID_Infotip;

	return S_OK;
}

STDMETHODIMP CQueryInfo::IsDirty()
{
	return E_NOTIMPL;
}

STDMETHODIMP CQueryInfo::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
	lstrcpyW(m_szInfotip, pszFileName);
	return S_OK;
}

STDMETHODIMP CQueryInfo::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
	return E_NOTIMPL;
}

STDMETHODIMP CQueryInfo::SaveCompleted(LPCOLESTR pszFileName)
{
	return E_NOTIMPL;
}

STDMETHODIMP CQueryInfo::GetCurFile(LPOLESTR *ppszFileName)
{
	return E_NOTIMPL;
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
	CQueryInfo *p;
	HRESULT    hr;

	*ppvObject = NULL;

	if (pUnkOuter != NULL)
		return CLASS_E_NOAGGREGATION;

	p = new CQueryInfo();
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

	if (IsEqualCLSID(rclsid, CLSID_Infotip))
		hr = serverFactory.QueryInterface(riid, ppv);
	else
		hr = CLASS_E_CLASSNOTAVAILABLE;

	return hr;
}

STDAPI DllRegisterServer(void)
{
	TCHAR szModulePath[MAX_PATH];
	TCHAR szKey[1024];

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

  for (const auto &g_szExt : g_szExts) {
    wsprintf(szKey, TEXT("%s\\shellex\\{00021500-0000-0000-C000-000000000046}"), g_szExt);
    if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, (LPTSTR)g_szClsid))
      return E_FAIL;
  }

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
