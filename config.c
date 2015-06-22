#include"config.h"
#include"util.h"
#include"stdio.h"
#include"string.h"

//is the return necessary?
char *left_trim(char *str)
{
	char *p1, *p2;
	p1 = p2 = str;
	while(*p2 == ' ')
		p2++;
	while(*p2 != '\0')
		*p1++ = *p2++;
	*p1 = '\0';
	return str;
}

char *right_trim(char *str)
{
	int n = strlen(str);
	char *p = str + n;
	p--;
	while(*p == ' ' || *p == '\n' || *p == '\r')
		*p-- = '\0';
	return str;
}

int get_key_value(char **key, char **value, char *line)
{
	char *p = strchr(line, '=');
	if(p == NULL)
		return -1;
	*p = '\0';
	*key = line;
	*value = p + 1;
	right_trim(*key);
	left_trim(*value);
	right_trim(*value);
	return 0;
}

//return -1 just when fail to open configuration file 
int make_config(config_s *conf)
{
	char line[256];
	char *key, *value;
	FILE *fp = fopen(CONFIG_FILE_PATH, "r");	
	if(fp == NULL)
		return -1;

	while(fgets(line, sizeof(line), fp))
	{
		left_trim(line);
		if(line[0] == '#' || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')	
			continue;
		int n = get_key_value(&key, &value, line);
		if(n == 0)
		{
			if(strcmp(key, "listen_port") == 0)
				conf->listen_port = atoi(value);
			else if(strcmp(key, "threads_num") == 0)
				conf->threads_num = atoi(value);
			else if(strcmp(key, "no_delay") == 0)
				conf->no_delay = atoi(value);
			else
				continue;
		}
	}
	fclose(fp);
	return 0;
}

//int main()
//{
//	config_s *conf = (config_s *)malloc(sizeof(config_s));
//	int n = make_config(conf);
//	if(n < 0)
//		printf("fail to open file\n ");
//	printf("listen_port: %d\n", conf->listen_port);
//	printf("threads_num: %d\n", conf->threads_num);
//	printf("no_delay: %d\n", conf->no_delay);
//}
