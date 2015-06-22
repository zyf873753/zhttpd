#include<pthread.h>

typedef struct zh_task_s
{
	void (*func)(void *);
	void * arg;
	struct zh_task_s *next;
} zh_task_s;

typedef struct
{
	zh_task_s *head;
	zh_task_s *tail;
} zh_task_queue_s;

typedef struct
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t  *threads;
	int thread_count;
	zh_task_queue_s *task_queue;
	int queue_length;
} zh_threadpool_s;

//初始化线程池，内部出错返回NULL
zh_threadpool_s *threadpool_init(int);

int threadpool_add(zh_threadpool_s *, void (*)(void *), void *);
