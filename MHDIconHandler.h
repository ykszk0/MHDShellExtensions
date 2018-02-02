#ifndef MHDICONHANDLER_H
#define MHDICONHANDLER_H

class CExtractIcon : public IExtractIcon, public IPersistFile
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int *piIndex, UINT *pwFlags);
	STDMETHODIMP Extract(LPCTSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

	STDMETHODIMP GetClassID(CLSID *pClassID);
	STDMETHODIMP IsDirty();
	STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
	STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
	STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
	STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

	CExtractIcon();
	~CExtractIcon();

private:
	LONG m_cRef;
    TCHAR m_szFilename[MAX_PATH];
};

class CClassFactory : public IClassFactory
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	STDMETHODIMP LockServer(BOOL fLock);
};

#endif /* MHDICONHANDLER_H */
