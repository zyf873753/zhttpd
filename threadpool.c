#include"threadpool.h"
#include"util.h"
#include"debug.h"

static void *__threadpool_worker(void *);

zh_threadpool_s *threadpool_init(int threadnum)
{
	int i, n;
	zh_threadpool_s *tp = (zh_threadpool_s *)malloc(sizeof(zh_threadpool_s));
	if(tp == NULL)
		return NULL;

	n = pthread_mutex_init(&tp->mutex, NULL);
	if(n < 0)
	{
		free(tp);
		return NULL;
	}

	n = pthread_cond_init(&tp->cond, NULL);
	if(n < 0)
	{
		free(tp);
		return NULL;
	}

	tp->threads = (pthread_t *)malloc(threadnum * sizeof(pthread_t));
	if(tp->threads == NULL)
	{
		free(tp);
		return NULL;
	}

	tp->thread_count = 0;
	for(i = 0; i < threadnum; i++)
	{
		n = pthread_create(&tp->threads[i], NULL, __threadpool_worker, (void *)tp);
		//???
		if(n == 0)
		{
//			fprintf(stderr, "create thread %d\n", tp->thread_count);
			tp->thread_count++;
		}
	}
	if(tp->thread_count == 0)
	{
		free(tp->threads);
        free(tp);
		return NULL;
	}

	tp->task_queue = (zh_task_queue_s *)malloc(sizeof(zh_task_queue_s));
	if(tp->task_queue == NULL)
	{
		free(tp->threads);
		free(tp);
		return NULL;
	}
	tp->task_queue->head = NULL;
	tp->task_queue->tail = NULL;

	tp->queue_length = 0;
	return tp;
}

int threadpool_add(zh_threadpool_s *tp, void (*task)(void *), void *arg)
{

	if(task == NULL || arg == NULL || tp == NULL)
		return -1;

	int n;
	n = pthread_mutex_lock(&tp->mutex);	
	if(n < 0)
		return -1;

	zh_task_queue_s *task_queue = tp->task_queue;
	zh_task_s *new_task = (zh_task_s *)malloc(sizeof(zh_task_s));
	if(new_task == NULL)
		return -1;

	new_task->func = task;
	new_task->arg = arg;
	new_task->next = NULL;
	if(task_queue->head == NULL)
		task_queue->head = new_task;
	else
		task_queue->tail->next = new_task;	
	task_queue->tail = new_task;
	new_task->next = NULL;
	tp->queue_length++;
	//fprintf(stderr, "queue_length:%d\n", tp->queue_length);


	n = pthread_cond_signal(&tp->cond);
	if(n < 0)
		return -1;

	n = pthread_mutex_unlock(&tp->mutex);
	if(n < 0)
		return -1;
	
	return 0;
}

//未做出错处理
void *__threadpool_worker(void *arg)
{
	zh_threadpool_s *tp = (zh_threadpool_s *)arg;
	zh_task_queue_s *task_queue;
	
	while(1)	
	{
		pthread_mutex_lock(&tp->mutex);	

		while(tp->queue_length == 0)
			pthread_cond_wait(&tp->cond, &tp->mutex);

		task_queue = tp->task_queue;

		zh_task_s *task = task_queue->head;
		if(task == NULL)
			fprintf(stderr, "task is NULL\n");
		task_queue->head = task_queue->head->next;
		if(task_queue->head == NULL)
			task_queue->tail = NULL;

		tp->queue_length--; //开始把这个放在unlock后，调试了好久才发现

		pthread_mutex_unlock(&tp->mutex);
		
		//fprintf(stderr, "fd:%d\n", *(int *)task->arg);
		(*task->func)(task->arg);

		free(task->arg);
		free(task); //当一个连接断开后，它的task->arg应该也释放掉，在哪释放呢？未实现
	}
	return (void *)0;
}
