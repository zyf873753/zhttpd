#include"util.h"

void err_exit(char *info)
{
	perror(info);
	exit(errno);
}
