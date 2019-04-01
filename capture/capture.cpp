#include "business.h"

int main(int argc, char** argv)
{
	if (argc < 4)
	{
		std::cout << "usage: capture <rtmp_url> <camera_idx> <audio_idx>" << std::endl;
		return 0;
	}

	socket_lib_helper helper;
	business b;
	std::string rtmp_url = argv[1];
	int camera_idx = atoi(argv[2]);
	int audio_idx = atoi(argv[3]);
	b.start(rtmp_url, camera_idx, audio_idx);
	return 0;
}



