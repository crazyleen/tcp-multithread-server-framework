#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>
#include <syslog.h>
#include "misc.h"


/**
 * catch_signal - Install signal handler for signo
 * @signo: Signal you want to catch
 * @func: Signal handler
 * @return: old signal handler
 *
 * Reliable version of signal(), using POSIX sigaction().
 */
//typedef void SigFunc(int);
SigFunc *catch_signal(int signo, SigFunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}


/**
 * print hex stream
 */
void printhex(const void *buf, int len) {
	int i = 0;
	const char *p = (const char *) buf;
	for (i = 1; i < len; i++) {
		printf("%02hhx ", *p++);
	}
	printf("\n");
}

/*
 * write PID to file
 */
int write_pid(const char *fpathname) {
	FILE *fp;
	//truncate file
	fp = fopen(fpathname, "w");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s: %s", fpathname, strerror(errno));
		return -1;
	}
	if (fprintf(fp, "%ld\n", (long) getpid()) < 0) {
		perror("fprintf");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

/**
 * getpid_from_file - get pid from file in /var/run/???*.pid
 * @filename: file path name
 * @return: -1 for error, >0 pid
 */
pid_t getpid_from_file(const char *pathname) {
	FILE *fp;
	pid_t pid = -1;
	fp = fopen(pathname, "r");
	if (fp == NULL) {
		printf("%s: %s\n", __FUNCTION__, strerror(errno));
		return -1;
	}
	if(fscanf(fp, "%d", &pid) < 0){
		perror("fscanf");
	}
	fclose(fp);

	if (pid < 0) {
		pid = -1;
	}

	return pid;
}

/**
 * write lock
 * @return: 0 on success,  <0 and (errno == EACCES || errno == EAGAIN) is locked file
 */
int lockfile(int fd) {
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd, F_SETLK, &fl));
}

/**
 * open and set write lock
 * @return: return fd, -1 on error, -2 on locked file(failed to set lock)
 */
int open_file_lock(const char *file) {
	int fd;

	//open for lock
	fd = open(file, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (fd < 0) {
		printf("%s: can't open %s: %s", __FUNCTION__, file, strerror(errno));
		//open error
		return -1;
	}
	if (lockfile(fd) < 0) {
		if (errno == EACCES || errno == EAGAIN) {
			//locked file, can't lock file again
			close(fd);
			return -2;
		}
		//lock error
		printf("can't lock %s: %s", file, strerror(errno));
		return -1;
	}

	return fd;
}

/**
 * open_file_lock_test - test a file lock
 * @return: return pid of lock owner, 0 on no lock, -1 on open error
 */
pid_t open_file_lock_test(const char *file)
{
	int fd;
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;

	//open for lock
	fd = open(file, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
	if (fd < 0) {
		printf("%s: can't open %s: %s", __FUNCTION__, file, strerror(errno));
		//open error
		return -1;
	}

	//get lock
	if (fcntl(fd, F_GETLK, &fl) < 0){
		perror("fcntl");
	}
	close(fd);

	if (fl.l_type == F_UNLCK){
		/* unlock file */
		return 0;
	}

	/* locked file, return pid of lock owner */
	return (fl.l_pid);
}

pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len) {
	struct flock lock;

	lock.l_type = type; /* F_RDLCK or F_WRLCK */
	lock.l_start = offset; /* byte offset, relative to l_whence */
	lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
	lock.l_len = len; /* #bytes (0 means to EOF) */

	if (fcntl(fd, F_GETLK, &lock) < 0)
		perror("fcntl");

	if (lock.l_type == F_UNLCK)
		return (0); /* false, region isn't locked by another proc */
	return (lock.l_pid); /* true, return pid of lock owner */
}

/**
 * write_pid_lock - lock file and write pid
 * @return: return fd, -1 on locked file, -2 on error
 */
int write_pid_lock(const char *file) {
	int fd;
	char buf[18];

	fd = open_file_lock(file);
	if (fd < 0) {
		//fail, -1 for locked, -2 on error
		return fd;
	}

	//write pid to file
	if(ftruncate(fd, 0) < 0) {
		;
	}
	snprintf(buf, 18, "%ld\n", (long) getpid());
	if(write(fd, buf, strlen(buf) + 1) != strlen(buf) + 1) {
		printf("write %s to pid file error\n", buf);
	}

	return fd;
}

/**
 * get_file_type - get file type
 * @return: 0 regular file, 1 directory, 2 Character device
 * 			3 block device, 4 fifo, 5 symbolic link, 6 socket
 * 			-1 on error.
 */
int get_file_type(char *file) {
	struct stat buf;
	if (lstat(file, &buf) < 0) {
		perror("lstat");
		return -1;
	}

	if (S_ISREG(buf.st_mode)) {
		//regular file
		return 0;
	} else if (S_ISDIR(buf.st_mode)) {
		//dir.
		return 1;
	} else if (S_ISCHR(buf.st_mode)) {
		//character device
		return 2;
	} else if (S_ISBLK(buf.st_mode)) {
		//block device
		return 3;
	} else if (S_ISFIFO(buf.st_mode)) {
		//fifo
		return 4;
	} else if (S_ISLNK(buf.st_mode)) {
		return 5;
	} else if (S_ISSOCK(buf.st_mode)) {
		//socket file
		return 6;
	}

	return -1;
}

/**
 * redirect_stdout - redirect stdout and stderr to a file
 * @return: 0 on success, -1 on error
 */
int redirect_stdout(char *file) {
	int fd = -1;
	if (file == NULL) {
		printf("%s: (NULL) para\n", __FUNCTION__);
		return -1;
	}

	fd = open(file, O_WRONLY | O_CREAT | O_APPEND | O_NONBLOCK | O_NOCTTY,
			0666);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	//only redirect stdout and stderr
	if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO
			|| dup2(fd, STDERR_FILENO) != STDERR_FILENO) {
		//Redirect error
		printf("redirect error\n");
		close(fd);
		return -1;
	}
	//Redirect success
	close(fd);

	return 0;
}

/**
 * try_redirect - get ${env_name} in env to redirect stdout and stderr to it
 * @return: 0 on success, -1 on error
 *
 * NOTE: olny support regular file and socket file
 */
int try_redirect(const char *env_name){
	char *file = NULL;
	int file_type = 0;

	/* We redirect to ${env_name} if it's set in env. */
	file = getenv(env_name);
	if(file == NULL){
		//printf("no env config for io redirection\n");
		return -1;
	}

	printf("stdout stderr ==> %s\n", file);
	file_type = get_file_type(file);
	if(file_type == 0){
		redirect_stdout(file);
	}else if(file_type == 6){
		//socket file
		int fd = open_unix_socket_file(file);
		if(fd < 0){
			return -1;
		}
		//only redirect stdout and stderr
		if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO
				|| dup2(fd, STDERR_FILENO) != STDERR_FILENO) {
			//Redirect error
			printf("redirect error\n");
			close(fd);
			return -1;
		}
		//Redirect success
		close(fd);
	}

	return 0;
}
/**
 * select_read - read with select()
 * @return: Return the number read, -1 for select error or time out, 0 for EOF.
 */
int select_read(int fd, void *buf, int len, struct timeval *timeout) {
	int ret;
	fd_set input;

	FD_ZERO(&input);
	FD_SET(fd, &input);

	ret = select(fd + 1, &input, NULL, NULL, timeout);

	// see if there was an error or actual data
	if (ret < 0) {
		perror("select");
	} else if (ret == 0) {
		printf("serial_read TIMEOUT\n");
	} else {
		if (FD_ISSET(fd, &input)) {
			return read(fd, buf, len);
		}
	}

	return -1;
}

/**
 *	serial_set - Config serial port parameter
 *	fd			serial device file descriptor
 *	baud_rate		serial communication baud rate
 *	data_bit			data bit count, default is 8
 *	stop_bit     stop bit count, default is 1
 *	parity				odd or even parity, default is no parity
 *	flow_ctrl		hardware or software flow control, default is no fc
 * @return:	0 on success, -1 on error.
 */
int serial_set(int fd, int baud_rate, char data_bit, char stop_bit, char parity,
		char flow_ctrl) {
	struct termios termios_new;
	int baud;
	int ret = -1;

	bzero(&termios_new, sizeof(termios_new));
	cfmakeraw(&termios_new); /* set termios raw data */

	switch (baud_rate) {
	case 2400:
		baud = B2400;
		break;
	case 4800:
		baud = B4800;
		break;
	case 38400:
		baud = B38400;
		break;
	case 57600:
		baud = B57600;
		break;
	case 115200:
		baud = B115200;
		break;
	default:
		baud = B9600;
		break;
	}/* end switch */
	cfsetispeed(&termios_new, baud);
	cfsetospeed(&termios_new, baud);

	termios_new.c_cflag |= (CLOCAL | CREAD);

	/* flow control */
	switch (flow_ctrl) {
	case 'H':
	case 'h':
		termios_new.c_cflag |= CRTSCTS; /* hardware flow control */
		break;
	case 'S':
	case 's':
		termios_new.c_cflag |= (IXON | IXOFF | IXANY); /* software flow control */
		break;
	default:
		termios_new.c_cflag &= ~CRTSCTS; /* default no flow control */
		break;
	}/* end switch */

	/* data bit */
	termios_new.c_cflag &= ~CSIZE;
	switch (data_bit) {
	case 5:
		termios_new.c_cflag |= CS5;
		break;
	case 6:
		termios_new.c_cflag |= CS6;
		break;
	case 7:
		termios_new.c_cflag |= CS7;
		break;
	default:
		termios_new.c_cflag |= CS8;
		break;
	}/* end switch */

	/* parity check */
	switch (parity) {
	case 'O':
	case 'o':
		termios_new.c_cflag |= PARENB; /* odd check */
		termios_new.c_cflag &= ~PARODD;
		break;
	case 'E':
	case 'e':
		termios_new.c_cflag |= PARENB; /* even check */
		termios_new.c_cflag |= PARODD;
		break;
	default:
		termios_new.c_cflag &= ~PARENB; /* default no check */
		break;
	}/* end switch */

	/* stop bit */
	if (stop_bit == 2)
		termios_new.c_cflag |= CSTOPB; /* 2 stop bit */
	else
		termios_new.c_cflag &= ~CSTOPB; /* 1 stop bit */

	/* other attribute */
	termios_new.c_oflag &= ~OPOST;
	termios_new.c_iflag &= ~ICANON;
	termios_new.c_cc[VMIN] = 1; /* read char min quantity */
	termios_new.c_cc[VTIME] = 1; /* wait time unit (1/10) second */

	/* clear data in receive buffer */
	tcflush(fd, TCIFLUSH);

	/* set config data */
	ret = tcsetattr(fd, TCSANOW, &termios_new);

	return ret;
}


/**
 * remount_rw_root - remount root rw if it's read-only file system
 * @return 0 on success
 */
int remount_rw_root(void) {
	FILE *fp = NULL;
	int ret = 0;

	//test if root fs is rw
	fp = fopen("/systest.txt", "w");
	if(fp == NULL){
		/* EROFS : Read-only file system */
		if(errno == EROFS){
			printf("%s cmdline: mount -o remount,rw /\n", __FUNCTION__);
			ret = system("mount -o remount,rw /");
			if(ret != 0){
				printf("%s: remount error", __FUNCTION__);
			}
		}
	}else{
		printf("no remount /\n");
		fclose (fp);
	}

	return ret;
}

/* become  daemon */
void daemonize(void)
{
	int pid;

	/*
	*clear file creation mask
	*/
	umask(0);

	/*
	*Become a session leader to lose controlling TTY
	*/
	if((pid = fork()) < 0)
	{
		//syslog(LOG_ERR, "Create daemon failed: %s\n", strerror(pid));
		exit(1);
	}
	else if(pid > 0)
	{
		exit(0);
	}
	setsid();

	/*
	*change the current working directory to the root so
	*we won't prevent file systems from being unmounted
	*/
	if (chdir("/") < 0)
		syslog(LOG_ERR, "can't change directory to /");

	/*
	*Close all open file descriptions
	*/
	//for (i = 0; i < 1024; i++)
	//	close(i);
}


/**
 * open unix af socket file (AF_UNIX) 打开unix域套接字
 * @return: return fd, -1 on error
 */
int open_unix_socket_file(char *sfile)
{
	char lfile[30];
	int sock_fd = -1;
	struct sockaddr_un sunx;
	socklen_t addrLength;

	if (realpath(sfile, lfile) == NULL) {
		return -1;
	}

	memset(&sunx, 0, sizeof(sunx));
	sunx.sun_family = AF_UNIX;
	strncpy(sunx.sun_path, lfile, sizeof(sunx.sun_path));
	if ((sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
		//printf("Couldn't get file descriptor for socket\n");
		return -1;
	}
	(void)fcntl(sock_fd, F_SETFD, 1);
	addrLength = sizeof(sunx.sun_family) + strlen(sunx.sun_path);
	if (connect(sock_fd, (struct sockaddr *)&sunx, addrLength) < 0) {
		close(sock_fd);
		return -1;
	}
	return sock_fd;
}
