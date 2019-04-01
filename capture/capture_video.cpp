#include "capture_video.h"
#include "util.h"


capture_video::capture_video()
	: idx_(-1)
	, interval_(40)
	, yuv_(NULL)
	, user_data_(0)
	, flag_run_(false)
{

}

capture_video::~capture_video()
{

}

int capture_video::enum_devs()
{
	com_lib_helper helper;
	int id = 0;


	ICreateDevEnum *pCreateDevEnum;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (hr != NOERROR)
		return -1;

	CComPtr<IEnumMoniker> pEm;
	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
	if (hr != NOERROR)
		return -1;

	pEm->Reset();
	ULONG cFetched;
	IMoniker *pM;
	while (hr = pEm->Next(1, &pM, &cFetched), hr == S_OK)
	{
		IPropertyBag *pBag;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if (SUCCEEDED(hr))
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR)
			{
				id++;
				SysFreeString(var.bstrVal);
			}
			pBag->Release();
		}
		pM->Release();
	}
	return id;
}


int capture_video::start(int camera_idx, int rate, yuv_callback cb, long user_data)
{
	if (flag_run_)
		return 0;

	flag_run_ = true;
	idx_ = camera_idx;
	interval_ = 1000 / rate;
	yuv_ = cb;
	user_data_ = user_data;

	this->thr_ = std::thread(&capture_video::svc, this);

	return 0;
}

void capture_video::stop()
{
	if (this->thr_.get_id() != std::thread::id())
	{
		flag_run_ = false;
		this->thr_.join();
	}
}

#define MACRO_DEFAULT_INTERVAL 66

int capture_video::svc()
{
	printf("capture_video::svc() chan[%d] enter \n", idx_);
	CvCapture *cap = cvCreateCameraCapture(idx_);
	if (!cap)
	{
		printf("capture_video::svc() index[%d]  CameraCapture failed\n", idx_);
		return -1;
	}

	DWORD t0, t1;

	t0 = t1 = ::GetTickCount();

	IplImage *img = NULL;
	int inter_calc = 0;
	for (; flag_run_;)
	{
		t0 = ::GetTickCount();

		IplImage *img = cvQueryFrame(cap);

		if (NULL == img)
		{
			Sleep(40);
			continue;
		}

#if 0
		if (NULL == img)
			img = cvCreateImage(cvGetSize(tempImg), tempImg->depth, tempImg->nChannels);
#endif

#if 1
		// cvCopy(tempImg, img);
		if (img->origin == IPL_ORIGIN_TL)
			cvFlip(img, img, 0);
#endif

		if (NULL != yuv_)
			yuv_(img, this->user_data_, this->idx_);

		t1 = ::GetTickCount();
		DWORD diff = t1 - t0;
		if (diff < MACRO_DEFAULT_INTERVAL)
			Sleep(MACRO_DEFAULT_INTERVAL - diff);		
	}

	cvReleaseImage(&img);
	cvReleaseCapture(&cap);
	printf("capture_video::svc() chan[%d] exit! \n", idx_);
	return 0;
}

