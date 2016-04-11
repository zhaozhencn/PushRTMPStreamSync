#ifndef sound_handler_H__
#define sound_handler_H__

class sound_handler 
{
public:
	virtual ~sound_handler() {};

public:
	virtual void handle(unsigned char* data, unsigned int len) = 0;
};

#endif

