/*
 * GQmpeg
 * (C) 2002 John Ellis
 *
 * Author: John Ellis
 *
 * This software is released under the GNU General Public License (GNU GPL).
 * Please read the included file COPYING for more information.
 * This software comes with no warranty of any kind, use at your own risk!
 */


#include "gqmpeg.h"
#include "cpu_perc.h"

#include <fcntl.h>

/*
 *-----------------------------------------------------------------------------
 * This section figures the cpu utilization (%) of a pid
 *
 * the ifdef's are getting a little heavy here, be careful
 *-----------------------------------------------------------------------------
 */

/* includes needed for Sun Solaris */
#ifdef sun
  #include <sys/procfs.h>
#endif

/* includes needed for SGI IRIX 5.3 and 6.5 */
#ifdef SGI
  #include <sys/signal.h>
  #include <sys/fault.h>
  #include <sys/syscall.h>
  #include <sys/procfs.h>
#endif

#ifndef HZ
#define HZ 100 /* for FreeBSD? */
#endif

/* for all the BSD4.4 descendants (FreeBSD, NetBSD, OpenBSD, BSD/OS...) */
#if defined(BSD) && (BSD >= 199306)
  #include <fcntl.h>
  #include <sys/mount.h>
#endif

gfloat check_child_cpu_usage(int childs_pid)
{
#if defined(BSD) && (BSD >= 199306)
	/* don't blame GQmpeg author, <rohee@OpenBSD.ORG> is the only
	   responsible for this mess, if it breaks other BSDs that's
	   not intentional, normally it'll help for any BSD 4.4 derivative */

	gfloat percent_of_cpu = 0.0;
	char procfs_mountpoint[MAXPATHLEN];
	char procname[MAXPATHLEN];
	int procfile;
	struct timezone timez;

	static int saved_pid = 0;
	static struct timeval last_measure_time;
	static struct timeval last_cumulative_time;

	/* find where the proc fs is mounted, some strange people could mount it
	   in another place than /proc, it can even be not mounted at all */
	{
	  struct statfs *mntbufp;
	  int fs_count;
	  int procfs_found = 0;
	  
	  fs_count = getmntinfo(&mntbufp, MNT_WAIT);
	  
	  while(fs_count--)               
#ifdef __FreeBSD__ /* FreeBSD prior to 3.0 doesn't have f_fstypename and
                      f_mntfromname happens to be set to procfs, but
	              it's a kludge */
	    if(!strcmp(mntbufp[fs_count].f_mntfromname, "procfs") &&
#else
	    if(!strcmp(mntbufp[fs_count].f_fstypename, "procfs") &&
#endif
	       (procfs_found = 1))
	      snprintf(procfs_mountpoint, sizeof(procfs_mountpoint), mntbufp[fs_count].f_mntonname);
	  
	  if(!procfs_found)
	    return 0.0;
	}

	snprintf(procname, sizeof(procname), "%s/%d/status", procfs_mountpoint, childs_pid); /* no overflow */
	
	if ( (procfile = open(procname, O_RDONLY)) == -1 )
		;
	else
		{
		char *status_string;
		char *saved_string;
		int i;
		struct timeval start_time;  
		struct timeval cumulative_time;  
		struct timeval measure_time;
		gfloat cpu_consumed;
		gfloat elapsed_time;

		status_string = (char *) g_malloc0(128 * sizeof(char));
		read(procfile, status_string, 128);
		close(procfile);

		saved_string = status_string;

		/* skip to the interesting values */
		i= 7;
		while(i--)
			strsep(&status_string, " "); /* strsep is only for BSD,
							but it's BSD here */

		/* startup time */
		start_time.tv_sec = atoi( strsep(&status_string, ","));
		start_time.tv_usec = atoi( strsep(&status_string, " "));

		/* user time */
		cumulative_time.tv_sec = atoi( strsep(&status_string, ","));
		cumulative_time.tv_usec = atoi( strsep(&status_string, " "));

		/* system time */
		cumulative_time.tv_sec += atoi( strsep(&status_string, ","));
		cumulative_time.tv_usec += atoi( strsep(&status_string, " "));

		free(saved_string); /* status_string is modified, that's why
				       I've to save it */

		gettimeofday(&measure_time, &timez);

		if (childs_pid == saved_pid) /* same proc since last measure */
			{
			elapsed_time = (measure_time.tv_sec -
			last_measure_time.tv_sec) * 1000000 +
			measure_time.tv_usec - last_measure_time.tv_usec;

			cpu_consumed = (cumulative_time.tv_sec -
			last_cumulative_time.tv_sec) * 1000000 +
			cumulative_time.tv_usec - last_cumulative_time.tv_usec;
			}
		else /* new process since last measure, count from start */
			{
			elapsed_time = (measure_time.tv_sec -
			start_time.tv_sec) * 1000000 +
			measure_time.tv_usec - start_time.tv_usec;

			cpu_consumed = cumulative_time.tv_sec * 1000000 +
			cumulative_time.tv_usec;
			}

		percent_of_cpu = cpu_consumed / elapsed_time * 100;
		saved_pid = childs_pid;
		last_cumulative_time =  cumulative_time;
		last_measure_time = measure_time;
		}

	/* end of BSD code */
#else

/* needed for SGI IRIX 5.3 and 6.5 */
#ifdef SGI
	int fd;
	int retval;
	prusage_t p;
#endif

/* needed to get cpu % on Sun Solaris */
#ifdef sun

	gfloat percent_of_cpu = 0.0;
	prpsinfo_t psinfo;
	char procname[20];
	int  procfile;
  
	snprintf(procname, sizeof(procname), "/proc/%.5d", childs_pid);

	if ( (procfile = open(procname, O_RDONLY)) == -1 )
		percent_of_cpu = 0.0;
	else
		{
		if ( ioctl(procfile, PIOCPSINFO, &psinfo) == -1 )
			percent_of_cpu = 0.0;
		else
			percent_of_cpu = ( ((gfloat) psinfo.pr_pctcpu) *100.0 / 32768.0 ); 
		close(procfile);
		}

	/* end of sun #if */

#else

	/* linux code... */

	struct timeval time;
	static struct timeval oldtime;
	struct timezone timez;
	static gfloat old_child_time;

	gchar s[32], str[256];
	gchar *strp;
	glong child_time;
	gfloat child, cpu;
	gfloat percent_of_cpu;
	int i;

	FILE *f;

	if (childs_pid <=0) return 0;

	gettimeofday(&time, &timez);

	snprintf(s, sizeof(s), "/proc/%d/stat", childs_pid);

/* needed for SGI IRIX 5.3 and 6.5 */
#ifdef SGI

	snprintf(s, sizeof(s), "/proc/pinfo/%05d",childs_pid);

	fd = open(s, O_RDONLY);
	if (fd < 0) return 0;
	retval = ioctl(fd, PIOCUSAGE, &p);
	if (retval < 0)
		{
		perror(s);
		close(fd);
		return 0;
		}
	close(fd);
	child_time = (p.pu_utime.tv_sec * 100) + p.pu_utime.tv_nsec / 10000000;

#else

	/* linux code - er - these #ifdef are getting messy */

	f = fopen(s,"r");

	/* open failed */
	if (!f) return 0;

	fgets(str, 255, f);
	fclose(f);

	strp = str;
	for (i=0; i < 13; i++)
		{
		while (*strp != ' ' && *strp != '\0') strp++;
		if (*strp == ' ') strp++;
		}
	child_time = strtol(strp, NULL, 10);

	/* end of SGI #if */
#endif

	child = child_time - old_child_time;
	cpu = (time.tv_sec - oldtime.tv_sec)
		+ (float) (time.tv_usec - oldtime.tv_usec) / 1000000.0;

	oldtime.tv_sec = time.tv_sec;
	oldtime.tv_usec = time.tv_usec;
	old_child_time = child_time;

	percent_of_cpu = (child / HZ) / cpu * 100;

	/* end of sun #if */
#endif

	/* end of BSD #if (I think... FIXME: cleanup this #if mess!) */
#endif

	if (percent_of_cpu > 99.0) percent_of_cpu = 99.0;

	return percent_of_cpu;
}


