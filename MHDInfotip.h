#ifndef MHDINFOTIP_H
#define MHDINFOTIP_H

class CQueryInfo : public IQueryInfo, public IPersistFile
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP GetInfoTip(DWORD dwFlags, LPWSTR *ppwszTip);
	STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);

	STDMETHODIMP GetClassID(CLSID *pClassID);
	STDMETHODIMP IsDirty();
	STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
	STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
	STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
	STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

	CQueryInfo();
	~CQueryInfo();

private:
	LONG  m_cRef;
	WCHAR m_szInfotip[2048];
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

#endif /* MHDINFOTIP_H */
