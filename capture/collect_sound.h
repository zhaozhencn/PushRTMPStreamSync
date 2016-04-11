#ifndef COLLECT_SOUND_H__
#define COLLECT_SOUND_H__

#include "sound_comm.h"

class sound_handler;
class collect_sound
{
public:
	collect_sound(void);
	~collect_sound(void);

public:
	bool	start(const void* pSoundCapture, const void* pwfxInput, sound_handler* p_sound_data_handler);
	void	stop();
	bool	is_running() const;

private:
	bool	is_running_;

	sound_handler* 
			p_sound_data_handler_;

protected:
	virtual HRESULT initSoundCapture(LPCGUID pSoundCapture);
	virtual HRESULT initSoundCaptureBuffer(const WAVEFORMATEX* pwfxInput);
	virtual HRESULT initSoundCaptureNotify(const WAVEFORMATEX* pwfxInput);

	virtual void finiSoundCaptureNotify();
	virtual void finiSoundCaptureBuffer();
	virtual void finiSoundCapture();

private:
	CComPtr<IDirectSoundCapture8> directSoundCapture8Ptr_; 
	CComPtr<IDirectSoundCaptureBuffer8> directSoundCaptureBufer8Ptr_;
	CComPtr<IDirectSoundNotify8> directSoundNotify8Ptr_;

private:
	DWORD	dwNotifySize_;
	DWORD   dwCaptureBufferSize_;
	DWORD   dwNextCaptureOffset_;

	enum { NUM_REC_NOTIFICATIONS = 2 };
	DSBPOSITIONNOTIFY aPosNotify_[NUM_REC_NOTIFICATIONS + 1];  

	HANDLE	hNotificationEvent_;

private:
	HANDLE	hCollectThread_;
	HANDLE	bNotifyThreadExitEvent_;
	static	unsigned __stdcall collectThreadProc(void* lpParam);
	unsigned	threadEntry( );
	enum { MAX_SYNC_WAIT_THREAD_SEC = 10 };

private:
	HRESULT	recordCapturedData();

};

#endif


