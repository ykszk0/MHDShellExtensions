#ifndef MHDPREVIEWHANDLER_H
#define MHDPREVIEWHANDLER_H
#include <vector>

class CPreviewHandler : public IPreviewHandler, public IObjectWithSite, public IOleWindow, public IInitializeWithFile
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP SetWindow(HWND hwnd, const RECT *prc);
	STDMETHODIMP SetRect(const RECT *prc);
	STDMETHODIMP DoPreview(VOID);
	STDMETHODIMP Unload(VOID);
	STDMETHODIMP SetFocus(VOID);
	STDMETHODIMP QueryFocus(HWND *phwnd);
	STDMETHODIMP TranslateAccelerator(MSG *pmsg);

	STDMETHODIMP SetSite(IUnknown *pUnkSite);
	STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

	STDMETHODIMP GetWindow(HWND *phwnd);
	STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

	IFACEMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD grfMode);

	CPreviewHandler();
	~CPreviewHandler();

private:
	LONG                 m_cRef;
	HWND                 m_hwndParent;
	HWND                 m_hwndPreview;
	RECT                 m_rc;
	IUnknown             *m_pSite;
	LPWSTR m_pPathFile;
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

#endif /* MHDPREVIEWHANDLER_H */
