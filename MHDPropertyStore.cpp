#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <propkey.h>
#include <propvarutil.h>
#include "libmhd.h"
#include "MHDPropertyStore.h"
#include <propkey.h>
#include <algorithm>

#pragma comment (lib, "propsys.lib")
#pragma comment (lib, "shlwapi.lib")

static const CLSID CLSID_PropertyStoreSample = 
{ 0xc0ef8573, 0x7dcb, 0x43a1, { 0x99, 0x47, 0xd5, 0x75, 0xd5, 0x7d, 0xb0, 0xc5 } };
const TCHAR g_szClsid[] = TEXT("{C0EF8573-7DCB-43A1-9947-D575D57DB0C5}");
const TCHAR g_szHandlerName[] = TEXT("MHD Property Handler");
const TCHAR *g_szExts[] = { TEXT(".mhd"), TEXT(".mha") };

const PROPERTYKEY g_propKeySupport[] = {PKEY_Kind, PKEY_KindText,PKEY_Image_BitDepth,PKEY_Image_Dimensions};
//const PROPERTYKEY g_propKeySupport[] = {PKEY_Keywords};
const int         g_nPropKeyCount    = sizeof(g_propKeySupport);

LONG      g_lLocks = 0;
HINSTANCE g_hinstDll = NULL;

void LockModule(BOOL bLock);
BOOL CreateRegistryKey(HKEY hKeyRoot, LPTSTR lpszKey, LPTSTR lpszValue, LPTSTR lpszData);


// CPropertyStore


CPropertyStore::CPropertyStore()
{
	m_cRef = 1;
	m_pStream = NULL;
	m_pCache = NULL;

	LockModule(TRUE);
}

CPropertyStore::~CPropertyStore()
{
  if (m_pStream != NULL) {
    m_pStream->Release();
  }
  if (m_pCache != NULL) {
    m_pCache->Release();
  }
	LockModule(FALSE);
}

STDMETHODIMP CPropertyStore::QueryInterface(REFIID riid, void **ppvObject)
{
	*ppvObject = NULL;

	if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IPropertyStore))
		*ppvObject = static_cast<IPropertyStore *>(this);
	else if (IsEqualIID(riid, IID_IPropertyStoreCapabilities))
		*ppvObject = static_cast<IPropertyStoreCapabilities *>(this);
	else if (IsEqualIID(riid, IID_IInitializeWithStream))
		*ppvObject = static_cast<IInitializeWithStream *>(this);
	else
		return E_NOINTERFACE;

	AddRef();

	return S_OK;
}

STDMETHODIMP_(ULONG) CPropertyStore::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CPropertyStore::Release()
{
	if (InterlockedDecrement(&m_cRef) == 0) {
		delete this;
		return 0;
	}

	return m_cRef;
}

STDMETHODIMP CPropertyStore::GetCount(DWORD *cProps)
{
	if (m_pCache == NULL)
		return E_UNEXPECTED;

	return m_pCache->GetCount(cProps);
}

STDMETHODIMP CPropertyStore::GetAt(DWORD iProp, PROPERTYKEY *pkey)
{
	if (m_pCache == NULL)
		return E_UNEXPECTED;

	return m_pCache->GetAt(iProp, pkey);
}

STDMETHODIMP CPropertyStore::GetValue(REFPROPERTYKEY key, PROPVARIANT *pv)
{
	if (m_pCache == NULL)
		return E_UNEXPECTED;

	return m_pCache->GetValue(key, pv);
}

STDMETHODIMP CPropertyStore::SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
	int     i;
	HRESULT hr = E_FAIL;

	if (m_pCache == NULL)
		return E_UNEXPECTED;

	for (i = 0; i < g_nPropKeyCount; i++) {
		if (IsEqualPropertyKey(key, g_propKeySupport[i])) {
			hr = m_pCache->SetValueAndState(g_propKeySupport[i], &propvar, PSC_NORMAL);
			break;
		}
	}
	
	return hr;
}

STDMETHODIMP CPropertyStore::Commit(VOID)
{
	int         i;
	HRESULT     hr = E_FAIL;
	PROPVARIANT propvar;
	ULONG       uSize;
	WCHAR       szBuf[256];

	if (m_pCache == NULL)
		return E_UNEXPECTED;

	for (i = 0; i < g_nPropKeyCount; i++) {
		m_pCache->GetValue(g_propKeySupport[i], &propvar);
		PropVariantToString(propvar, szBuf, sizeof(szBuf) / sizeof(WCHAR));

		uSize = sizeof(szBuf);
		m_pStream->Write(szBuf, lstrlenW(szBuf) * sizeof(WCHAR), &uSize);

		PropVariantClear(&propvar);
	}

	m_pStream->Commit(STGC_DEFAULT);

	return S_OK;
}

STDMETHODIMP CPropertyStore::IsPropertyWritable(REFPROPERTYKEY key)
{
	return IsEqualPropertyKey(key, PKEY_Search_Contents) ? S_FALSE : S_OK;
}

namespace
{
  template <typename T>
  HRESULT InitPropVariant(T value, PROPVARIANT *ppropvar);
  template <> HRESULT InitPropVariant<double>(double value, PROPVARIANT *ppropvar) { return InitPropVariantFromDouble(value, ppropvar); };
  template <> HRESULT InitPropVariant<int>(int value, PROPVARIANT *ppropvar) { return InitPropVariantFromInt32(value, ppropvar); };
  template <> HRESULT InitPropVariant<unsigned int>(unsigned int value, PROPVARIANT *ppropvar) { return InitPropVariantFromInt32(value, ppropvar); };
  template <typename T>
  void set_prop(PROPVARIANT &propvar, IPropertyStoreCache *pCache, const T &value, PROPERTYKEY key)
  {
    InitPropVariant<T>(value, &propvar);
    pCache->SetValueAndState(key, &propvar, PSC_NORMAL);
    PropVariantClear(&propvar);
  }
void set_prop_int(PROPVARIANT &propvar, IPropertyStoreCache *pCache, int num, PROPERTYKEY key)
{
  InitPropVariantFromInt32(num, &propvar);
  pCache->SetValueAndState(key, &propvar, PSC_NORMAL);
  PropVariantClear(&propvar);
}
void set_prop_uint(PROPVARIANT &propvar, IPropertyStoreCache *pCache, unsigned int num, PROPERTYKEY key)
{
  InitPropVariantFromUInt32(num, &propvar);
  pCache->SetValueAndState(key, &propvar, PSC_NORMAL);
  PropVariantClear(&propvar);
}
void set_prop_double(PROPVARIANT &propvar, IPropertyStoreCache *pCache, double num, PROPERTYKEY key)
{
  InitPropVariantFromDouble(num, &propvar);
  pCache->SetValueAndState(key, &propvar, PSC_NORMAL);
  PropVariantClear(&propvar);
}
void set_prop_str(PROPVARIANT &propvar, IPropertyStoreCache *pCache, const wchar_t *str, PROPERTYKEY key)
{
  InitPropVariantFromString(str, &propvar);
  pCache->SetValueAndState(key, &propvar, PSC_NORMAL);
  PropVariantClear(&propvar);
}
std::map<std::string, int> ElementTypeMap = {
  {"MET_CHAR",8},
  {"MET_UCHAR",8},
  {"MET_SHORT",16},
  {"MET_USHORT",16},
  {"MET_INT",32},
  {"MET_UINT",32},
  {"MET_LONG",64},
  {"MET_ULONG",64},
  {"MET_FLOAT",32},
  {"MET_DOUBLE",64}
};
std::map<std::string, char*> ElementType2Text = {
  {"MET_CHAR","char"},
  {"MET_UCHAR","unsigned char"},
  {"MET_SHORT","short"},
  {"MET_USHORT","unsigned short"},
  {"MET_INT","int"},
  {"MET_UINT","unsigned int"},
  {"MET_LONG","long"},
  {"MET_ULONG","unsigned long"},
  {"MET_FLOAT","float"},
  {"MET_DOUBLE","double"}
};
int ElementType2bits(const std::string &str)
{
  auto it = ElementTypeMap.find(str);
  if (it != ElementTypeMap.end()) {
    return it->second;
  }
  return 0;
}
template <typename T>
T stoT(const std::string &str);
template <>
double stoT<double>(const std::string &str) {
  return std::stod(str);
}
template <>
int stoT<int>(const std::string &str) {
  return std::stoi(str);
}


template <typename T>
std::vector<T> str2vec(std::string str, char delim)
{
  std::vector<T> v;
  char *str_p = &str[0];
  try {
    int i_end = str.size();
    int start = 0;
    for (int i = 0; i < i_end; ++i) {
      if (str_p[i] == delim) {
        str_p[i] = '\0';
        v.push_back(stoT<T>(str_p + start));
        start = i + 1;
      }
    }
    if (start < i_end) {
      v.push_back(stoT<T>(str_p + start));
    }
  } catch (std::exception &e) {
    return v;
  }
  return v;
}
}
STDMETHODIMP CPropertyStore::Initialize(IStream *pstream, DWORD grfMode)
{
  PROPVARIANT propvar;

	PSCreateMemoryPropertyStore(IID_PPV_ARGS(&m_pCache));


  constexpr int buf_size = 1024;
  std::vector<wchar_t> w_buf(buf_size);

  auto header = read_header(pstream);
  auto header_map = parse_header(header);
  {
    auto it = header_map.find("DimSize");
    if (it != header_map.end()) {
      auto dim = it->second;
      std::replace(dim.begin(), dim.end(), ' ', 'x');
      mbstowcs(w_buf.data(), dim.c_str(), buf_size);
      set_prop_str(propvar, m_pCache, w_buf.data(), PKEY_Image_Dimensions);
      auto v = str2vec<int>(it->second, ' ');
      // width
      if (v.size() >= 1) {
        set_prop_int(propvar, m_pCache, v[0], PKEY_Image_HorizontalSize);
      }
      // height
      if (v.size() >= 2) {
        set_prop_int(propvar, m_pCache, v[1], PKEY_Image_VerticalSize);
      }
    }
  }
  {
    auto it = header_map.find("ElementType");
    if (it != header_map.end()) {
      auto bits = ElementType2bits(it->second);
      if (bits == 0) {
        set_prop_str(propvar, m_pCache, L"Unknown", PKEY_Image_BitDepth);
        set_prop_str(propvar, m_pCache, L"Unknown ElementType", PKEY_Comment);
      } else {
        set_prop_int(propvar, m_pCache, bits, PKEY_Image_BitDepth);
        mbstowcs(w_buf.data(), ElementType2Text[it->second], buf_size);
        set_prop_str(propvar, m_pCache, w_buf.data(), PKEY_Keywords);
      }
    }
  }
  // resolution
  {
    auto it = header_map.find("ElementSpacing");
    if (it != header_map.end()) {
      mbstowcs(w_buf.data(), it->second.c_str(), buf_size);
      set_prop_str(propvar, m_pCache, w_buf.data(), PKEY_Comment);
    }
  }

  set_prop_str(propvar, m_pCache, KIND_PICTURE, PKEY_Kind);
  set_prop_str(propvar, m_pCache, L"Meta Image", PKEY_KindText);
  set_prop_str(propvar, m_pCache, L".mhd", PKEY_ItemType);
  set_prop_str(propvar, m_pCache, L"Meta Image", PKEY_ItemTypeText);
  {
    auto it = header_map.find("CompressedData");
    if (it != header_map.end() && it->second == "True") {
      set_prop_str(propvar, m_pCache, L"Compressed", PKEY_Image_CompressionText);
      set_prop_uint(propvar, m_pCache, IMAGE_COMPRESSION_PACKBITS, PKEY_Image_Compression);
    } else {
      set_prop_str(propvar, m_pCache, L"Raw", PKEY_Image_CompressionText);
      set_prop_uint(propvar, m_pCache, IMAGE_COMPRESSION_UNCOMPRESSED, PKEY_Image_Compression);
    }
  }

	m_pStream = pstream;
	m_pStream->AddRef();

	return S_OK;
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
	CPropertyStore *p;
	HRESULT        hr;

	*ppvObject = NULL;

	if (pUnkOuter != NULL)
		return CLASS_E_NOAGGREGATION;

	p = new CPropertyStore();
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

	if (IsEqualCLSID(rclsid, CLSID_PropertyStoreSample))
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
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, TEXT("MHD ShellExtension")))
		return E_FAIL;

	GetModuleFileName(g_hinstDll, szModulePath, sizeof(szModulePath) / sizeof(TCHAR));
	wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, NULL, szModulePath))
		return E_FAIL;

	wsprintf(szKey, TEXT("CLSID\\%s\\InprocServer32"), g_szClsid);
	if (!CreateRegistryKey(HKEY_CLASSES_ROOT, szKey, TEXT("ThreadingModel"), TEXT("Apartment")))
		return E_FAIL;

    for (const auto &g_szExt : g_szExts) {
      wsprintf(szKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\%s"), g_szExt);
      if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, szKey, NULL, (LPTSTR)g_szClsid))
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
      wsprintf(szKey, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PropertySystem\\PropertyHandlers\\%s"), g_szExt);
      SHDeleteKey(HKEY_LOCAL_MACHINE, szKey);
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
	HKEY hKey;
	LONG lResult;

	lResult = RegCreateKeyEx(hKeyRoot, lpszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
	if (lResult != ERROR_SUCCESS)
		return FALSE;

	RegSetValueEx(hKey, lpszValue, 0, REG_SZ, (LPBYTE)lpszData, lstrlen(lpszData) * sizeof(TCHAR));
	RegCloseKey(hKey);

	return TRUE;
}
