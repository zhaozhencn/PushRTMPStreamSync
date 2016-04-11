#include "collect_sound.h"
#include "sound_handler.h"
#include <process.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)

#define SAFE_DELETE_KERNEL_OBJECT(p) \
	if (NULL != p) \
		CloseHandle(p);	


collect_sound::collect_sound(void)
{
//	::CoInitialize(0);
	this->p_sound_data_handler_ = NULL;
	this->is_running_ = false;
	this->dwNotifySize_ = 0;
	this->dwCaptureBufferSize_ = 0;
	this->dwNextCaptureOffset_ = 0;
	ZeroMemory( this->aPosNotify_, sizeof(aPosNotify_) );
	hNotificationEvent_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	hCollectThread_ = INVALID_HANDLE_VALUE;
	bNotifyThreadExitEvent_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}


collect_sound::~collect_sound(void)
{
	stop( );
	SAFE_DELETE_KERNEL_OBJECT( this->hNotificationEvent_ );
	SAFE_DELETE_KERNEL_OBJECT( this->bNotifyThreadExitEvent_ );

//	::CoUninitialize();
}


bool collect_sound::start(const void* pSoundCaptureIn, const void* pwfxInputIn,
				  sound_handler* p_sound_data_handler)
{
	LPCGUID pSoundCapture = reinterpret_cast<LPCGUID>(pSoundCaptureIn);
	const WAVEFORMATEX *pwfxInput = reinterpret_cast<const WAVEFORMATEX*>(pwfxInputIn);

	if ( this->is_running() )
	{
		return false;
	}

	HRESULT hr = S_FALSE;
	hr = initSoundCapture(pSoundCapture);
	if ( S_OK != hr)
	{
		return false;
	}

	hr = initSoundCaptureBuffer(pwfxInput);
	if (S_OK != hr)
	{
		this->directSoundCapture8Ptr_.Release();
		return false;
	}

	hr = initSoundCaptureNotify(pwfxInput);
	if ( S_OK != hr)
	{
		this->directSoundCaptureBufer8Ptr_.Release();
		this->directSoundCapture8Ptr_.Release();
		return false;
	}

	this->p_sound_data_handler_ = p_sound_data_handler;
	this->hCollectThread_ = (HANDLE)_beginthreadex(NULL, 0, &collect_sound::collectThreadProc, this, 0, NULL);
	if (NULL == this->hCollectThread_)
	{
		this->directSoundNotify8Ptr_.Release();
		this->directSoundCaptureBufer8Ptr_.Release();
		this->directSoundCapture8Ptr_.Release();
		return S_FALSE;
	}

	this->is_running_ = true;
	hr = this->directSoundCaptureBufer8Ptr_->Start(DSCBSTART_LOOPING);
	if ( S_OK != hr)
	{
		stop();
		return false;
	}

	return true;

}


void collect_sound::stop( )
{
	if ( !this->is_running() )
	{
		return;
	}

	HRESULT hr = this->directSoundCaptureBufer8Ptr_->Stop();

	::SetEvent(this->bNotifyThreadExitEvent_);
	DWORD dRet = ::WaitForSingleObject(this->hCollectThread_, MAX_SYNC_WAIT_THREAD_SEC * 1000);
	if (WAIT_TIMEOUT == dRet)
	{
		::TerminateThread(this->hCollectThread_, 0);
	}

	::CloseHandle(this->hCollectThread_);
	hCollectThread_ = INVALID_HANDLE_VALUE;
	::ResetEvent(this->bNotifyThreadExitEvent_);

	finiSoundCaptureNotify( );
	finiSoundCaptureBuffer( );
	finiSoundCapture( );

	this->is_running_ = false;
}	


bool collect_sound::is_running( ) const
{
	return this->is_running_;
}


HRESULT collect_sound::initSoundCapture(LPCGUID pSoundCapture)
{
	LPDIRECTSOUNDCAPTURE8 p = NULL;
	HRESULT hr = DirectSoundCaptureCreate8(pSoundCapture, &p, NULL);
	if ( S_OK != hr )
	{
		return S_FALSE;
	}

	this->directSoundCapture8Ptr_ = p;
	return S_OK;
}


HRESULT collect_sound::initSoundCaptureBuffer(const WAVEFORMATEX* pwfxInput)
{
	LPDIRECTSOUNDCAPTUREBUFFER pDSCB = NULL;
	LPDIRECTSOUNDCAPTUREBUFFER8 pDSCB8 = NULL;

	// Set the notification size
	dwNotifySize_ = MAX(1024, 4096); //  pwfxInput->nAvgBytesPerSec / 5);

	// 8k, 16bit, 1channel => 2048
	// 44.1, 16bit, 2channel =>4096
	// dwNotifySize_ = MAX(1024, 1024 * 1 * 2); // MAX(1024, pwfxInput->nAvgBytesPerSec / 5);
	dwNotifySize_ -= dwNotifySize_ % pwfxInput->nBlockAlign;   

	// Set the buffer sizes 
	dwCaptureBufferSize_ = dwNotifySize_ * NUM_REC_NOTIFICATIONS;

	// Create the capture buffer
	DSCBUFFERDESC dscbd;
	ZeroMemory( &dscbd, sizeof(dscbd) );
	dscbd.dwSize        = sizeof(dscbd);
	dscbd.dwBufferBytes = dwCaptureBufferSize_;
	dscbd.lpwfxFormat   = const_cast<LPWAVEFORMATEX>(pwfxInput); // Set the format during creatation

	// init dwNextCaptureOffset_
	dwNextCaptureOffset_ = 0;

	HRESULT hr = directSoundCapture8Ptr_->CreateCaptureBuffer(&dscbd, &pDSCB, NULL);
	if (SUCCEEDED(hr))
	{
		hr = pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&pDSCB8);
		if ( SUCCEEDED(hr))
		{
			pDSCB->Release(); 
			this->directSoundCaptureBufer8Ptr_ = pDSCB8;
			return S_OK;
		}
	}

	return S_FALSE;
}


HRESULT collect_sound::initSoundCaptureNotify(const WAVEFORMATEX* pwfxInput)
{
	LPDIRECTSOUNDNOTIFY8 pNotify = NULL;
	HRESULT hr = this->directSoundCaptureBufer8Ptr_->QueryInterface(IID_IDirectSoundNotify,
																	(VOID**)&pNotify);
	if ( S_OK != hr )
	{
		return S_FALSE;
	}

	// Setup the notification positions
	for(int i = 0; i < NUM_REC_NOTIFICATIONS; i++ )
	{
		aPosNotify_[i].dwOffset = (dwNotifySize_ * i) + dwNotifySize_ - 1;
		aPosNotify_[i].hEventNotify = hNotificationEvent_;
	}

	// Tell DirectSound when to notify us. the notification will come in the from 
	// of signaled events that are handled in Our Thread
	if( FAILED( hr = pNotify->SetNotificationPositions( NUM_REC_NOTIFICATIONS, 
		aPosNotify_) ) )
	{
		return hr;
	}

	this->directSoundNotify8Ptr_ = pNotify;
	return S_OK;
}


void collect_sound::finiSoundCaptureNotify( )
{
	this->directSoundNotify8Ptr_.Release();
}


void collect_sound::finiSoundCaptureBuffer( )
{
	this->directSoundCaptureBufer8Ptr_.Release();
}


void collect_sound::finiSoundCapture( )
{
	this->directSoundCapture8Ptr_.Release();
}


unsigned __stdcall collect_sound::collectThreadProc(void* lpParam)
{
	collect_sound* pCollectSound = reinterpret_cast<collect_sound*>(lpParam);
	return pCollectSound->threadEntry();
}


unsigned collect_sound::threadEntry()
{
	HANDLE hHandleArray[2] = {0};
	hHandleArray[0] = this->hNotificationEvent_;
	hHandleArray[1] = this->bNotifyThreadExitEvent_;
	HRESULT hr = S_FALSE;

	for (;;)
	{
		DWORD dRet = ::WaitForMultipleObjects(sizeof(hHandleArray)/sizeof(HANDLE),
											hHandleArray, FALSE, INFINITE);
		switch(dRet)
		{
		case WAIT_OBJECT_0:
			{
				hr = recordCapturedData( );
				if ( FAILED(hr) )
				{
					::SetEvent(this->bNotifyThreadExitEvent_);
				}
				break;
			}

		case WAIT_OBJECT_0 + 1:
			{
				return 0;
			}

		default:
			{
				return 0;
			}

		}
	}

	return 0;
}


HRESULT collect_sound::recordCapturedData( )
{
	HRESULT hr = S_FALSE;
	VOID*   pbCaptureData = NULL;
	DWORD   dwCaptureLength = 0;
	VOID*   pbCaptureData2 = NULL;
	DWORD   dwCaptureLength2 = 0;
	DWORD   dwReadPos = 0;
	DWORD   dwCapturePos = 0;
	LONG	lLockSize = 0;

	if( NULL == directSoundCaptureBufer8Ptr_ )
	{
		return S_FALSE;
	}

	if( FAILED( hr = directSoundCaptureBufer8Ptr_->GetCurrentPosition( &dwCapturePos,
																		&dwReadPos ) ) )
	{
		return hr;
	}

	lLockSize = dwReadPos - dwNextCaptureOffset_;
	if( lLockSize < 0 )
	{
		lLockSize += dwCaptureBufferSize_;
	}

	// Block align lock size so that we are always write on a boundary
	lLockSize -= (lLockSize % dwNotifySize_);

	if( lLockSize == 0 )
	{
		return S_FALSE;
	}

	// Lock the capture buffer down
	if( FAILED( hr = directSoundCaptureBufer8Ptr_->Lock( dwNextCaptureOffset_, lLockSize, 
		&pbCaptureData, &dwCaptureLength, 
		&pbCaptureData2, &dwCaptureLength2, 0L ) ) )
	{
		return hr;
	}

	// call p_sound_data_handler_::handle function
	if ( this->p_sound_data_handler_ )
	{
		this->p_sound_data_handler_->handle((unsigned char*)pbCaptureData, dwCaptureLength);
	}

	// Move the capture offset along
	dwNextCaptureOffset_ += dwCaptureLength; 
	dwNextCaptureOffset_ %= dwCaptureBufferSize_; // Circular buffer

	if( pbCaptureData2 != NULL )
	{
		// call p_sound_data_handler_::handle function
		if ( this->p_sound_data_handler_ )
		{
			this->p_sound_data_handler_->handle((unsigned char*)pbCaptureData2, dwCaptureLength2);
		}
	
		// Move the capture offset along
		dwNextCaptureOffset_ += dwCaptureLength2; 
		dwNextCaptureOffset_ %= dwCaptureBufferSize_; // Circular buffer
	}

	// Unlock the capture buffer
	directSoundCaptureBufer8Ptr_->Unlock( pbCaptureData,  dwCaptureLength, 
		pbCaptureData2, dwCaptureLength2 );


	return S_OK;
}



