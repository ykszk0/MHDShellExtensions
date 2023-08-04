#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <memory>
#include <clocale>
#include "MHDIconHandler.h"
#include "mhd.h"
#include "nrrd.h"
#include "nifti.h"

#pragma comment (lib, "shlwapi.lib")

static const CLSID CLSID_ExtractIconSample =
{ 0xdc2923e9, 0xa7c3, 0x49a8, { 0x99, 0x74, 0xf, 0x1a, 0x65, 0x18, 0x13, 0xbb } };
const TCHAR g_szClsid[] = TEXT("{DC2923E9-A7C3-49A8-9974-0F1A651813BB}");
const TCHAR g_szProgid[] = TEXT("MHDShellExtension");

#ifdef INCLUDE_GZ
const TCHAR* g_szExts[] = { TEXT(".mhd"), TEXT(".mha"), TEXT(".nrrd"), TEXT(".nii"), TEXT(".gz") };
#else
const TCHAR* g_szExts[] = { TEXT(".mhd"), TEXT(".mha"), TEXT(".nrrd"), TEXT(".nii") };
#endif

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

STDMETHODIMP CExtractIcon::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
  std::setlocale(LC_ALL, "");

  TCHAR szModulePath[MAX_PATH];
  GetModuleFileName(g_hinstDll,szModulePath, MAX_PATH);
  lstrcpyn(szIconFile, szModulePath, cchMax);
  *pwFlags = 0;
  std::shared_ptr<ParserBase> parsers[] = { std::make_shared<Mhd>(), std::make_shared<Nrrd>(), std::make_shared<Nifti>() };
  try {
		auto parser = ParserBase::select_parser(m_szFilename);
    if (!parser) { // no proper parser was found
      *piIndex = ParserBase::UnknownIcon;
      return S_OK;
    }
    parser->read_header(m_szFilename);
    parser->parse_header();
    *piIndex = parser->get_icon_index();
  } catch (std::exception &e) {
    *piIndex = ParserBase::UnknownIcon;
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

