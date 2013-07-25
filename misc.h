#ifndef __MISC_H__
#define	__MISC_H__

//global define
#define MOTE_MAP_PATH_NAME 	"/tmp/mote-map"
#define TIME_FILE 			"/tmp/time"

//-------------------- snio.c ---------------------------------------
void printhex(const void *buf, const int len);

/**
 * write lock
 * @return: 0 on success,  <0 and (errno == EACCES || errno == EAGAIN) is locked file
 */
int lockfile(int fd);

/**
 * open and set write lock
 * @return: return fd, -1 on error, -2 on locked file(failed to set lock)
 */
int open_file_lock(const char *file);

/**
 * open_file_lock_test - test a file lock
 * @return: return pid of lock owner, 0 on no lock, -1 on open error
 */
pid_t open_file_lock_test(const char *file);

/*
 * write PID to file
 */
int write_pid(const char *fpathname);

/**
 * write_pid_lock - lock file and write pid
 * @return: return fd, -1 on locked file, -2 on error
 */
int write_pid_lock(const char *file);

/**
 * getpid_from_file - get pid from file in /var/run/???*.pid
 * @filename: file path name
 * @return: -1 for error, >0 pid
 */
pid_t getpid_from_file(const char *pathname);

/**
 * redirect_stdout - redirect stdout and stderr to a file
 * @return: 0 on success, -1 on error
 */
int redirect_stdout(char *file);

/**
 * try_redirect - get ${env_name} in env to redirect stdout and stderr to it
 * @return: 0 on success, -1 on error
 *
 * NOTE: olny support regular file and socket file
 */
int try_redirect(const char *env_name);

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
		char flow_ctrl);

/**
 * select_read - read with select()
 * @return: Return the number read, -1 for select error or time out, 0 for EOF.
 */
int select_read(int fd, void *buf, int len, struct timeval *timeout);

//-------------------- snfork.c ---------------------------------------

/**
 * remount_rw_root - remount root rw if it's read-only file system
 * @return 0 on success
 */
int remount_rw_root(void);

/* become  daemon */
void daemonize(void);

/**
 * get_file_type - get file type
 * @return: 0 regular file, 1 directory, 2 Character device
 * 			3 block device, 4 fifo, 5 symbolic link, 6 socket
 * 			-1 on error.
 */
int get_file_type(char *file);

/**
 * redirect_stdout - redirect stdout and stderr to a file
 * @return: 0 on success, -1 on error
 */
int redirect_stdout(char *file);

//-------------------- af_unix_socket.c ---------------------------------------
/**
 * open unix af socket file (AF_UNIX) ¿?¿?unix¿?¿?¿?¿?
 * @return: return fd, -1 on error
 */
int open_unix_socket_file(char *sfile);

//-------------------- snsignal.c ---------------------------------------

/**
 * catch_signal - Install signal handler for signo
 * @signo: Signal you want to catch
 * @func: Signal handler
 * @return: old signal handler
 *
 * Reliable version of signal(), using POSIX sigaction().
 */
typedef void SigFunc(int);
SigFunc *catch_signal(int signo, SigFunc *func);

#endif /* __MISC_H__ */

