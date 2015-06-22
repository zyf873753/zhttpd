#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> //STDOUT_FILENO
#include<string.h>
//#include<sys/types.h>
//#include<sys/stat.h>
#include<fcntl.h> //O_RDONLY, fcntl
#include<ctype.h>
#include<sys/epoll.h>
#include<signal.h>
#include<netinet/tcp.h> //TCP_NODELAY

#include"threadpool.h"
#include"util.h"
#include"debug.h"
#include"config.h"

#define SA struct sockaddr
#define Server "Server: zhttpd-0.1\r\n"


int sock_listen(int *);
void readandprint(int);
size_t readline(int, char *, size_t);
void sendheader(int);
void sendfile(int, char *);
void handlerequest(void *);
int make_sock_non_blocking(int);

config_s conf;

int sock_listen(int *port)
{
	int listenfd, n, on;
	size_t len;
	struct sockaddr_in sockaddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd < 0)
		err_exit("socket");

	on = 1;
	n = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)); //reuse port
	if(n < 0)
		err_exit("setsockopt reuse address");

	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(*port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = bind(listenfd, (SA *)&sockaddr, sizeof(sockaddr));
	if(n < 0)
		err_exit("bind");

	n = listen(listenfd, 5);
	if(n < 0)
		err_exit("listen");

	len = sizeof(sockaddr); //如果设置端口为0，那么会随机分配一个端口，但是如果len不初始化，getsockname返回的端口为0！！！
	n = getsockname(listenfd, (SA *)&sockaddr, &len);
	if(n < 0)
		err_exit("getsockname");
	*port = ntohs(sockaddr.sin_port);
	
	return listenfd;
}

int make_sock_non_blocking(int fd)
{
	int flag, n;	
	flag = fcntl(fd, F_GETFL, 0);
	if(flag < 0)
		return -1;
	flag |= O_NONBLOCK;
	n = fcntl(fd, F_SETFL, flag);
	if(n < 0)
		return -1;
	return 0;
}

void readandprint(int conn)
{
	int buf[4096];
	int n;
	while(( n = read(conn, buf, sizeof(buf))) > 0)
		write(STDOUT_FILENO, buf, n);
//	n = recv(conn, buf, sizeof(buf), MSG_WAITALL);
	sleep(10);
}

//low efficency
size_t readline(int fd, char *buf, size_t size)
{
	char c = '\0'; //一定要赋值（其他值也可），不然最后关闭套接字时，在浏览器会返回reset错误，我也不知道为什么，调试了一个下午才找到
	size_t i;
	ssize_t n;
	for(i = 0; i < size - 1 && c != '\n'; i++)
	{
		n = recv(fd, &c, 1, 0);
		if(n < 0)
		{
			fprintf(stderr, "%d\n", fd);
			//err_exit("recv");
		}
		if(n == 0)
			break;
		if(c == '\r')
		{
			recv(fd, &c, 1, MSG_PEEK);
			if(c == '\n')
				recv(fd, &c, 1, 0);
			else
				c = '\n';
		}
		buf[i] = c;
	}
	buf[i] = '\0';
	return i;
}
int get_line(int sock, char *buf, int size)
{
 int i = 0;
 char c = '\0';
 int n;

 while ((i < size - 1) && (c != '\n'))
 {
  n = recv(sock, &c, 1, 0); //每次只读一个字符，效率不高
  /* DEBUG printf("%02X\n", c); */
  if(n < 0)
	  perror("recv");
  if (n > 0)
  {
   if (c == '\r')
   {
    n = recv(sock, &c, 1, MSG_PEEK); //较老的客户端发送的信息每行只有回车，没有换行，所以要预先读取，如果下一个是换行，则读，如果不是，则自己设置,我们不需要回车，所以回车是不写入我们的缓冲的
    /* DEBUG printf("%02X\n", c); */
    if ((n > 0) && (c == '\n'))
     recv(sock, &c, 1, 0);
    else
     c = '\n';
   }
   buf[i] = c;
   i++;
  }
  else
   c = '\n';
 }
 buf[i] = '\0';

 return(i);
}
void sendheader(int conn)
{
	char buf[1024];
	int n;
	n = sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(conn, buf, n, 0);
	n = sprintf(buf, Server);
	send(conn, buf, n, 0);
	n = sprintf(buf, "Content-type: text/html\r\n");
	send(conn, buf, n, 0);
	n = sprintf(buf, "\r\n");
	send(conn, buf, n, 0);
}

void sendfile(int conn, char *loc)
{
	int fd = open(loc, O_RDONLY);
	int n;
	char buf[4096];
	while(( n = read(fd, buf, sizeof(buf))) != 0)
		write(conn, buf, n);
    write(conn, '\0', 1);
    close(fd);
}

void handlerequest(void *arg)
{
	int n;
	char buf[4096], method[10], location[128], version[10];
	int conn = *(int *)arg;

	n = get_line(conn, buf, sizeof(buf));
			if(n < 0)
				perror("get_line");

	sscanf(buf, "%s %s %s", method, location, version);

	if(!strcasecmp(method, "GET"))
	{
		if(!strcasecmp(location, "/"))
		{
		 buf[0] = 'A'; buf[1] = '\0';
         while ((n > 0) && strcmp("\n", buf))  /* read & discard headers */
            n = get_line(conn, buf, sizeof(buf));

			char *loc = "Resource/index.html";
			sendheader(conn);
			sendfile(conn, loc);
	//	not_found(conn);
		}
	}
	close(conn);
}




int main(void)
{
	int port;
	int MAX_EVENTS = 10;
	int conn, epfd, nfds, n;
	struct sockaddr_in cliaddr;
	size_t len = sizeof(cliaddr);
	struct epoll_event ev, events[MAX_EVENTS];

	//printf("Please enter the port to listen on: ");
	//scanf("%d", &port);
	n = make_config(&conf);
	if(n < 0)
		err_exit("make_config");
	port = conf.listen_port;
	int listenfd = sock_listen(&port);
	n = make_sock_non_blocking(listenfd);
	if(n < 0)
		err_exit("make_sock_non_blocking");
	printf("Server is running on port %d\n", port);//必须加换行符，不然不会输出

//	conn = accept(listenfd, (SA *)&cliaddr, &len);
//	if(conn < 0)
//		err_exit("connect");
//	printf("There is a new Connection\n");

	epfd = epoll_create(5);
	if(epfd < 0)
		err_exit("epoll_create");

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = listenfd;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) < 0)
		err_exit("epoll_ctl");

	signal(SIGPIPE, SIG_IGN); //防止客户端未读完就close所造成的SIGPIPE信号中断程序

	zh_threadpool_s *tp = threadpool_init(conf.threads_num);

	while(1)
	{
		nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if(nfds < 0)
			err_exit("epoll_wait");
		int i;
		for(i = 0 ; i < nfds; i++)
		{
			if(events[i].data.fd == listenfd)
			{
				while(1)
				{
					conn = accept(listenfd, (SA *)&cliaddr, &len);
					if(conn < 0)
					{
						if(errno == EAGAIN || errno == EWOULDBLOCK)
							break;
						else
							err_exit("accept");
					}
					//printf("There is a new Connection, fd = %d\n", conn);
					n = make_sock_non_blocking(conn);
					if(conf.no_delay)
					{
						int on = 1;
						setsockopt(conn, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
					}
					if(n < 0)
						err_exit("make_sock_non_blocking");
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = conn; 
					if(epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev) < 0)
						err_exit("epoll_ctl1");
				}
			}
			else if(events[i].events & EPOLLIN)
			{
				int *fd = (int *)malloc(sizeof(int));
				*fd = events[i].data.fd;
				threadpool_add(tp, handlerequest, (void *)fd); //以后复杂了，就传递结构，用events[i].data.ptr
//				close(events[i].data.fd);
			//	handlerequest(&events[i].data.fd);
//				if(epoll_ctl(epfd, EPOLL_CTL_DEL, conn, NULL) < 0) //conn has been closed in handlerequest, then it would removed from epoll
//					err_exit("epoll_ctl2");
			}
		}
	}

	//readandprint(conn);
	//handlerequest(conn);
    close(conn);
	return 0;
}
