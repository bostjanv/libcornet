#include "taskimpl.h"
#include <sys/poll.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h> /* send */

enum
{
	MAXFD = 1024
};

static struct pollfd pollfd[MAXFD];
static Task *polltask[MAXFD];
static int npollfd;
static int startedfdtask;
static Tasklist sleeping;
static int sleepingcounted;
static uvlong nsec(void);

static int signaled = 0;

void fdsignal() {
    int i;

    signaled = 1;

    /* wake up tasks */
#ifdef DEBUG    
    printf("waking up %d tasks\n", npollfd);
    /*printf("sleepingcounted= %d\n", sleepingcounted);
    Task *t;
	for(t=sleeping.head; t!=nil; t=t->next) {
        printf("task %d is sleeping\n", t->id);
    }*/
    for (i = 0; i < npollfd; ++i) {
        printf("task %d: %d\n", polltask[i]->id, polltask[i]->ready);
    }
#endif
    for(i=0; i<npollfd; i++){
        while(i < npollfd){
            taskready(polltask[i]);
            --npollfd;
            pollfd[i] = pollfd[npollfd];
            polltask[i] = polltask[npollfd];
        }
    }
}

int fdsignaled() {
    return signaled;
}

void
fdtask(void *v)
{
	int i, ms;
	Task *t;
	uvlong now;
	
	//tasksystem();
	taskname("fdtask");
	for(;;){
		/* let everyone else run */
		while(taskyield() > 0)
			;

#ifdef DEBUG        
        for (i = 0; i < npollfd; ++i) {
            printf("task %d: %d\n", polltask[i]->id, polltask[i]->ready);
        }
#endif

        printf("npollfd= %d\n", npollfd);

        /* NOTE(bv): somebody wants us to quit */
        if (signaled) {            
            printf("Exiting fdtask: id= %d\n", taskid());
            return;
        }

		/* we're the only one runnable - poll for i/o */
		errno = 0;
		taskstate("poll");
		if((t=sleeping.head) == nil)
			ms = -1;
		else{
			/* sleep at most 5s */
			now = nsec();
			if(now >= t->alarmtime)
				ms = 0;
			else if(now+5*1000*1000*1000LL >= t->alarmtime)
				ms = (t->alarmtime - now)/1000000;
			else
				ms = 5000;
		}
		if(poll(pollfd, npollfd, ms) < 0){
			if(errno == EINTR)
				continue;
			fprintf(stderr, "poll: %s\n", strerror(errno));
			taskexitall(0);
		}

		/* wake up the guys who deserve it */
		for(i=0; i<npollfd; i++){
			while(i < npollfd && pollfd[i].revents){
				taskready(polltask[i]);
				--npollfd;
				pollfd[i] = pollfd[npollfd];
				polltask[i] = polltask[npollfd];
			}
		}
		
		now = nsec();
		while((t=sleeping.head) && now >= t->alarmtime){
			deltask(&sleeping, t);
			if(!t->system && --sleepingcounted == 0)
				taskcount--;
			taskready(t);
		}
	}
}

uint
taskdelay(uint ms)
{
	uvlong when, now;
	Task *t;
	
	if(!startedfdtask){
		startedfdtask = 1;
		taskcreate(fdtask, 0, 32768);
	}

	now = nsec();
	when = now+(uvlong)ms*1000000;
	for(t=sleeping.head; t!=nil && t->alarmtime < when; t=t->next)
		;

	if(t){
		taskrunning->prev = t->prev;
		taskrunning->next = t;
	}else{
		taskrunning->prev = sleeping.tail;
		taskrunning->next = nil;
	}
	
	t = taskrunning;
	t->alarmtime = when;
	if(t->prev)
		t->prev->next = t;
	else
		sleeping.head = t;
	if(t->next)
		t->next->prev = t;
	else
		sleeping.tail = t;

	if(!t->system && sleepingcounted++ == 0)
		taskcount++;
	taskswitch();

	return (nsec() - now)/1000000;
}

void
fdwait(int fd, int rw)
{
	int bits;

	if(!startedfdtask){
		startedfdtask = 1;
		taskcreate(fdtask, 0, 32768);
	}

	if(npollfd >= MAXFD){
		fprintf(stderr, "too many poll file descriptors\n");
		abort();
	}
	
	taskstate("fdwait for %s", rw=='r' ? "read" : rw=='w' ? "write" : "error");
	bits = 0;
	switch(rw){
	case 'r':
		bits |= POLLIN;
		break;
	case 'w':
		bits |= POLLOUT;
		break;
	}

	polltask[npollfd] = taskrunning;
	pollfd[npollfd].fd = fd;
	pollfd[npollfd].events = bits;
	pollfd[npollfd].revents = 0;
	npollfd++;
	taskswitch();
}

/* Like fdread but always calls fdwait before reading. */
int
fdread1(int fd, void *buf, int n)
{
	int m;
	
	do {
		fdwait(fd, 'r');
        if (signaled) {
            return 0;
        }
    } while((m = read(fd, buf, n)) < 0 && errno == EAGAIN);
	return m;
}

int
fdread(int fd, void *buf, int n)
{
	int m;
	
	while((m=read(fd, buf, n)) < 0 && errno == EAGAIN) {
		fdwait(fd, 'r');
        if (signaled) {
            return 0;
        }
    }
	return m;
}

int
fdwrite(int fd, void *buf, int n)
{
	int m, tot;
	
	for(tot=0; tot<n; tot+=m){
		//while((m=write(fd, (char*)buf+tot, n-tot)) < 0 && errno == EAGAIN)
        while((m=send(fd, (char*)buf+tot, n-tot, MSG_NOSIGNAL)) < 0 && errno == EAGAIN) {
            fdwait(fd, 'w');
            if (signaled) {
                return 0;
            }
        }
		if(m < 0)
			return m;
		if(m == 0)
			break;
	}
	return tot;
}

int
fdnoblock(int fd)
{
#ifdef SO_NOSIGPIPE
    int set = 1;
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
#endif
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
}

static uvlong
nsec(void)
{
	struct timeval tv;

	if(gettimeofday(&tv, 0) < 0)
		return -1;
	return (uvlong)tv.tv_sec*1000*1000*1000 + tv.tv_usec*1000;
}

