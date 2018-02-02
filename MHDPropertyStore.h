#ifndef MHDPROPERTYSTORE_H
#define MHDPROPERTYSTORE_H

class CPropertyStore : public IPropertyStore, public IPropertyStoreCapabilities, public IInitializeWithStream
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	STDMETHODIMP GetCount(DWORD *cProps);
	STDMETHODIMP GetAt(DWORD iProp, PROPERTYKEY *pkey);
	STDMETHODIMP GetValue(REFPROPERTYKEY key, PROPVARIANT *pv);
	STDMETHODIMP SetValue(REFPROPERTYKEY key, REFPROPVARIANT propvar);
	STDMETHODIMP Commit(VOID);

	STDMETHODIMP IsPropertyWritable(REFPROPERTYKEY key);

	STDMETHODIMP Initialize(IStream *pstream, DWORD grfMode);

	CPropertyStore();
	~CPropertyStore();

private:
	LONG                m_cRef;
	IStream             *m_pStream;
	IPropertyStoreCache *m_pCache;
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

#endif /* MHDPROPERTYSTORE_H */
