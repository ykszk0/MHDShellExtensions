#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "MHDIconHandler.h"
#include "libmhd.h"

#pragma comment (lib, "shlwapi.lib")

static const CLSID CLSID_ExtractIconSample =
{ 0xdc2923e9, 0xa7c3, 0x49a8, { 0x99, 0x74, 0xf, 0x1a, 0x65, 0x18, 0x13, 0xbb } };
const TCHAR g_szClsid[] = TEXT("{DC2923E9-A7C3-49A8-9974-0F1A651813BB}");
const TCHAR g_szProgid[] = TEXT("MHDShellExtension");
const TCHAR* g_szExts[] = { TEXT(".mhd"), TEXT(".mha") };

LONG      g_lLocks = 0;
HINSTANCE g_hinstDll = NULL;

void LockModule(BOOL bLock);
BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue, LPTSTR lpszData);


// CExtractIcon


CExtractIcon::CExtractIcon()
{
	m_cRef = 1;

	LockModule(TRUE);
}

CExtractIcon::~CExtractIcon()
{
	LockModule(FALSE);
}

STDMETHODIMP CExtractIcon::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IExtractIcon))
		*ppvObject = static_cast<IExtractIcon *>(this);
	else if (IsEqualIID(riid, IID_IPersist) || IsEqualIID(riid, IID_IPersistFile))
		*ppvObject = static_cast<IPersistFile *>(this);
	else
		return E_NOINTERFACE;

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CExtractIcon::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CExtractIcon::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0) {
		delete this;
		return 0;
	}

	return m_cRef;
}

namespace
{
  std::map<std::string,int> type2index = {
    {"MET_CHAR",-101}, 
    {"MET_UCHAR",-102}, 
    {"MET_SHORT",-103}, 
    {"MET_USHORT",-104}, 
    {"MET_INT",-105}, 
    {"MET_UINT",-106}, 
    {"MET_LONG",-107}, 
    {"MET_ULONG",-108}, 
    {"MET_FLOAT",-109}, 
    {"MET_DOUBLE",-110} 
  };
  int ElementType2Index(const std::string &type)
  {
    auto it = type2index.find(type);
    if (it != type2index.end()) {
      return it->second;
    } else {
      return -100;
    }
  }
}

STDMETHODIMP CExtractIcon::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
  setlocale(LC_ALL, "");

  TCHAR szModulePath[MAX_PATH];
  GetModuleFileName(g_hinstDll,szModulePath, MAX_PATH);
  lstrcpyn(szIconFile, szModulePath, cchMax);
  *pwFlags = 0;
  auto header = parse_header(read_header(m_szFilename));
  auto it = header.find("ElementType");
  if (it != header.end()) {
    *piIndex = ElementType2Index(it->second);
  } else {
    *piIndex = 0;
  }
  return S_OK;
}

STDMETHODIMP CExtractIcon::Extract(LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
  return S_FALSE;;
}

STDMETHODIMP CExtractIcon::GetClassID(CLSID *pClassID)
{
	*pClassID = CLSID_ExtractIconSample;

	return S_OK;
}

STDMETHODIMP CExtractIcon::IsDirty()
{
	return E_NOTIMPL;
}

STDMETHODIMP CExtractIcon::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
  lstrcpyn(m_szFilename, pszFileName, MAX_PATH);
  return S_OK;
}

STDMETHODIMP CExtractIcon::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
	return E_NOTIMPL;
}

STDMETHODIMP CExtractIcon::SaveCompleted(LPCOLESTR pszFileName)
{
	return E_NOTIMPL;
}

STDMETHODIMP CExtractIcon::GetCurFile(LPOLESTR *ppszFileName)
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
	CExtractIcon *p;
	HRESULT      hr;

	*ppvObject = NULL;

	if (pUnkOuter != NULL)
		return CLASS_E_NOAGGREGATION;

	p = new CExtractIcon();
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

	if (IsEqualCLSID(rclsid, CLSID_ExtractIconSample))
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
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, TEXT("MHDShellExtension")))
		return E_FAIL;

	GetModuleFileName(g_hinstDll, szModulePath, sizeof(szModulePath) / sizeof(TCHAR));
	wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, szModulePath))
		return E_FAIL;

	wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, TEXT("ThreadingModel"), TEXT("Apartment")))
		return E_FAIL;

  for (const auto &g_szExt : g_szExts) {
    wsprintf(szKey, TEXT("%s"), g_szExt);
    if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, (LPTSTR)g_szProgid))
      return E_FAIL;
  }

	wsprintf(szKey, TEXT("%s\\shellex\\IconHandler"), g_szProgid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, (LPTSTR)g_szClsid))
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

	SHDeleteKey(HKEY_CLASSES_ROOT, g_szProgid);

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

int main()
{
  return 0;
}

