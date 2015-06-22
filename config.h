#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_FILE_PATH "zhttpd.conf"

typedef struct
{
	int listen_port;
	int threads_num;
	int no_delay;
} config_s;
	
int make_config(config_s *);

#endif
