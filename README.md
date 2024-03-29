webserver
=========

Operating System project.

The code is compiled using:
cc server.c -lthread -o myhttpd

Usage: ./myhttpd [−d] [−h] [−l file] [−p port] [−r dir] [−t time] [−n thread_num]  [−s sched]
	−d : Enter debugging mode. That is, do not daemonize, only accept
	one connection at a time and enable logging to stdout.  Without
	this option, the web server should run as a daemon process in the
	background.
	−h : Print a usage summary with all options and exit.
	−l file : Log all requests to the given file. See LOGGING for
	details.
	−p port : Listen on the given port. If not provided, myhttpd will
	listen on port 8080.
	−r dir : Set the root directory for the http server to dir.
	−t time : Set the queuing time to time seconds.  The default should
	be 60 seconds.
	−n thread_num : Set number of threads waiting ready in the execution thread pool to
	threadnum.  The default is 4 execution threads.
	−s sched : Set the scheduling policy. It can be either FCFS or SJF.
	The default will be FCFS.

