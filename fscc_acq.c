/* fscc_acq
 *  ->A Frankenstein of code written by SMH as well as older code written
 *    by MPD for acquisition through QuickUSB
 *
 * se creó y encargó : Jun 3, 2014
 *
 *
 */

/*
 *Quehaceres: Need to handle different sizes present for X-Sync mode
 *Search for "//!!!" to find things that need to be changed or considered
 *
 */

#define _GNU_SOURCE

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <termios.h>

#include <fscc.h> /* fscc_* */
#include "fscc_errors.h"
#include "fscc_helpers.h"
#include "fscc_acq.h"

#define MIN_BYTES_READ 100

static bool running = true;

int main(int argc, char **argv)
{
  struct fscc_opt o;
  struct fscc_port *pcontrol;
  int e;  

  init_opt(&o);
  parse_opt(&o, argc, argv);

  //Do it now so that we know what o.mport is
  pcontrol = malloc(o.mport * sizeof(struct fscc_port));

  
  signal(SIGINT, do_depart);

  //Initialize all ports who want to help out
  //####################################
  for (int i = 0; i < o.mport; i++ ) {
    e = fscc_connect(o.ports[i], &pcontrol[i].h );
    if (e != 0) {
      fprintf(stderr, "fscc_connect failed with %d\n", e);
      return EXIT_FAILURE;
    }
    e = init_fscc(o.ports[i], &o, &pcontrol[i] );
    if (e != EXIT_SUCCESS) {
      fprintf(stderr,"Could not initialize FSCC-LVDS!!\n");
      fprintf(stderr,"Laterz...\n");
      return EXIT_FAILURE;
    }
    e = toggle_receive(o.ports[i], &pcontrol[i], 0);
    if (e != 0) {
      printe("Failed to turn off data reception for port %i!\n",i);
      printe("Laterz...\n");
      return EXIT_FAILURE;
    }
  }
  //#####################################
  
  fscc_mport(o,pcontrol);

}

void fscc_mport(struct fscc_opt o, struct fscc_port *pc) {
  
  time_t pg_time;
  int tret, ret, rtdsize = 0;
  struct fscc_ptargs *thread_args;
  pthread_t *data_threads;
  //	pthread_t rtd_thread;
  
  short int **rtdframe, *rtdout = NULL;
  struct header_info header;
  int rfd, active_threads = 0;
  char *rmap = NULL, lstr[1024];
  struct stat sb;
  pthread_mutex_t *rtdlocks;
  double telapsed;
  struct timeval now, then;

  pg_time = time(NULL);
  sprintf(lstr, "%s/%s", o.outdir, o.prefix);
  fscc_log(lstr, pg_time);
  //  usleep(100000);
  fscc_log("Dartmouth College RxDSP serial acquisition, started %s", ctime(&pg_time));
  fscc_log("Test test test test test ");
  fscc_log("est est est ");
  fscc_log("tes tes tes %i foo",3);


  data_threads = malloc(o.mport * sizeof(pthread_t));
  printf("o.mport is currently %i\n",o.mport);
  rtdlocks = malloc(o.mport * sizeof(pthread_mutex_t));
  thread_args = malloc(o.mport * sizeof(struct fscc_ptargs));
  rtdframe = malloc(o.mport * sizeof(short int *));
  
  if (o.dt > 0) {
    printf("RTD");
    
    rtdsize = o.rtdsize * sizeof(short int);
    if (rtdsize > 2*o.acqsize) printf("RTD Total Size too big!\n");
    else printf(" (%i", o.rtdsize);
    if (1024*o.rtdavg > rtdsize) printf("Too many averages for given RTD size.\n");
    else printf("/%iavg)", o.rtdavg);
    printf("...");
    
    rtdout = malloc(o.mport * rtdsize);
    
    if ((rtdframe == NULL) || (rtdout == NULL)) {
      printe("RTD mallocs failed.\n");
    }
    
    for (int i = 0; i < o.mport; i++) {
      rtdframe[i] = malloc(rtdsize);
    }
    
    /*
     * Create/truncate the real-time display file, fill it
     * with zeros to the desired size, then mmap it.
     */
    rfd = open(o.rtdfile,
	       O_RDWR|O_CREAT|O_TRUNC,
	       S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (rfd == -1) {
      printe("Failed to open rtd file.\n"); return;
    }
    if ((fstat(rfd, &sb) == -1) || (!S_ISREG(sb.st_mode))) {
      printe("Improper rtd file.\n"); return;
    }
    int mapsize = o.mport*rtdsize + 100;
    char *zeroes = malloc(mapsize);
    memset(zeroes, 0, mapsize);
    ret = write(rfd, zeroes, mapsize);
    free(zeroes);
    //		printf("mmap, %i",rfd);fflush(stdout);
    rmap = mmap(0, mapsize, PROT_WRITE|PROT_READ, MAP_SHARED, rfd, 0);
    if (rmap == MAP_FAILED) {
      printe("mmap() of rtd file failed.\n"); return;
    }
    //		printf("postmmap");fflush(stdout);
    ret = close(rfd);
    madvise(rmap, mapsize, MADV_WILLNEED);
    
    /*
     * Set up basic RTD header
     */
    header.num_read = o.rtdsize*o.mport;
    sprintf(header.site_id,"%s","RxDSP Woot?");
    header.hkey = 0xF00FABBA;
    header.num_channels=o.mport;
    header.channel_flags=0x0F;
    header.num_samples=o.rtdsize;
//!!! DOES THIS NEED TO BE CHANGED TO 960000?
    header.sample_frequency=960000;
    header.time_between_acquisitions=o.dt;
    header.byte_packing=0;
    header.code_version=0.1;
  }
  
  
  /*
   * Set up and create the write thread for each serial port.
   */
  for (int i = 0; i < o.mport; i++) {
    thread_args[i].np = o.ports[i];
    thread_args[i].pc = &(pc[i]);
    ret = pthread_mutex_init(&rtdlocks[i], NULL);
    if (ret) {
      printe("RTD mutex init failed: %i.\n", ret); exit(EEPP_THREAD);
    }
    
    fscc_log("port %i...", o.ports[i]);
    printf("port %i...", o.ports[i]); fflush(stdout);
    thread_args[i].o = o;
    thread_args[i].retval = 0;
    thread_args[i].running = &running;
    thread_args[i].rtdframe = rtdframe[i];
    thread_args[i].rlock = &rtdlocks[i];
    thread_args[i].time = pg_time;
    
    ret = pthread_create(&data_threads[i], NULL, fscc_data_pt, (void *) &thread_args[i]);
    
    if (ret) {
      fscc_log("Thread %i failed!: %i.\n", i, ret); exit(EEPP_THREAD);
      printe("Thread %i failed!: %i.\n", i, ret); exit(EEPP_THREAD);
    } else active_threads++;
  }
  
  fscc_log("initalization done.\n");
  printf("done.\n"); fflush(stdout);
  
  if (o.debug) printf("Size of header: %li, rtdsize: %i, o.mport: %i.\n", sizeof(header), rtdsize, o.mport);


  /*
   * Now we sit back and update RTD data until all data threads quit.
   */
  gettimeofday(&then, NULL);
  while ((active_threads > 0) || running) {
    if (o.dt > 0) {
      gettimeofday(&now, NULL); // Check time
      telapsed = now.tv_sec-then.tv_sec + 1E-6*(now.tv_usec-then.tv_usec);

      if (telapsed > o.dt) {
	/*
	 * Lock every rtd mutex, then just copy in the lock, for speed.
	 */
	for (int i = 0; i < o.mport; i++) {
	  pthread_mutex_lock(&rtdlocks[i]);
	  memmove(&rtdout[i*o.rtdsize], rtdframe[i], rtdsize);
	  pthread_mutex_unlock(&rtdlocks[i]);
	}

	header.start_time = time(NULL);
	header.start_timeval = now;
	header.averages = o.rtdavg;

	memmove(rmap, &header, sizeof(struct header_info));
	memmove(rmap+102, rtdout, rtdsize*o.mport);

	then = now;
      }

    }

    /*
     * Check for any threads that are joinable (i.e. that have quit).
     */
    for (int i = 0; i < o.mport; i++) {
      ret = pthread_tryjoin_np(data_threads[i], (void *) &tret);

      tret = thread_args[i].retval;
      if (ret == 0) {
	active_threads--;
	if (tret) printf("port %i error: %i...", o.ports[i], tret);
	else printf("port %i clean...", o.ports[i]);

	if (running) {
	  /*
	   * Restart any threads that decided to crap out.
	   * GET UP AND FIGHT YOU SONOFABITCH!!!
	   */

	  ret = pthread_create(&data_threads[i], NULL, fscc_data_pt, (void *) &thread_args[i]);

	  if (ret) {
	    fscc_log("Port %i revive failed!: %i.\n", i, ret); exit(EEPP_THREAD);
	    printe("Port %i revive failed!: %i.\n", i, ret); exit(EEPP_THREAD);
	  } else active_threads++;

	} // if (running)
      } // if (ret == 0) (thread died)
    } // for (; i < o.mport ;)

    //!!!    usleep(50000); // Zzzz...
    usleep(5000); // Zzzz...
  }

  /*
   * Free.  FREE!!!
   */
  if (o.dt > 0) {
    for (int i = 0; i < o.mport; i++) {
      if (rtdframe[i] != NULL) free(rtdframe[i]);
    }
    free(rtdframe); free(rtdlocks);
    free(thread_args); free(data_threads);
    if (rtdout != NULL) free(rtdout);
  }

  printf("sayonara.\n");

  pthread_exit(NULL);
  
  
}

void *fscc_data_pt(void *threadarg) {

  struct fscc_ptargs arg;
  arg = *(struct fscc_ptargs *) threadarg;

//!!! Need to figure out how this is going to work.
//It's possible that the computer/FSCC is angry with you because you are
//disabling both receivers then trying to read, or perhaps creating two
//handles to the receivers and then discarding them, or perhaps you're
//asking too many things to happen at once and just need to put in some
//sleep here and there.

  fscc_handle h = arg.pc->h;
  int e = 0;
  int ee = 0;
  int receiving;

  unsigned count;

  int ret, rtdbytes;
  double telapsed;
  long long unsigned int i = 0;
  long long unsigned int frames, wcount;
//  struct frame_sync sync;

//  char dataz[arg.o.acqsize]; // why?
  char *dataz;
  char ostr[1024];
  struct tm ct;
  struct timeval start, now, then;
  FILE *ofile;

  dataz = malloc(arg.o.acqsize);

  printf("FSCC-LVDS port %i data thread init.\n", arg.np); fflush(stdout);
  
  rtdbytes = arg.o.rtdsize*sizeof(short int);
  
  //	printf("size of struct: %li, sync: %li\n", sizeof(struct frame_sync), sizeof(sync));
    
  e = toggle_receive(arg.np,arg.pc,1);
  e = fscc_status(arg.np, h);

  frames = count = wcount = 0;
  
  gmtime_r(&arg.time, &ct);
  sprintf(ostr, "%s/%s-%04i%02i%02i-%02i%02i%02i-p%02i.data", arg.o.outdir, arg.o.prefix,
	  ct.tm_year+1900, ct.tm_mon+1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec, arg.np);
  ofile = fopen(ostr, "a");
  if (ofile == NULL) {
    fprintf(stderr, "Failed to open output file %s.\n", ostr);
    arg.retval = EEPP_FILE; pthread_exit((void *) &arg.retval);
  }
   
  gettimeofday(&start, NULL);
  then = start;
   
  fscc_log("Serial port %i data acquisition start.  Output file: '%s'.  Acqsize: %i.\n", arg.np, ostr, arg.o.acqsize);
  if (arg.o.debug) { printf("Serial port %i data acquisition start.  acqsize = %i\n", arg.np, arg.o.acqsize); fflush(stdout); }

  int sleeptime = floor(arg.o.acqsize/333333.0*2*2*1e6*0.75);
  printf("Sleeping %i us.\n", sleeptime);
  
  /*
   * Main data loop
   */
  receiving = 1;
  while (*arg.running) {
    if (arg.o.debug) { printf("Serial port %i debug.\n", arg.np); fflush(stdout); }

    //MAYBE USE THIS OR SOMETHING LIKE IT TO SLEEP IF WE DON'T HAVE A TIMING SIGNAL
    /* if (!(fifo_status & 0b01)) { // Wait for acquire sequence line */
    /*   usleep(10); */
    /*   continue; */
    /* } */

    usleep(110000);
      
    // read
    if (arg.o.debug) { printf("Serial port %i read data.\n", arg.np); fflush(stdout); }
    
    //    memset(dataz, 0xff, arg.o.acqsize);
    memset(dataz, 0, arg.o.acqsize);
    if (receiving) {
      e = fscc_read_with_timeout(h, dataz, arg.o.acqsize, &count, 1000);
    }
    else {
      e = 1;
    }
    if( e == 16001) {
      fscc_disconnect(h);
      fprintf(stderr,"FSCC_INCORRECT_MODE\n");
      arg.retval = EXIT_FAILURE; pthread_exit((void *) &arg.retval);
    }
    else if( e != 0) {
      while (e != 0) {
	if (e == 16002) {
	  fprintf(stderr,"FSCC_BUFFER_TOO_SMALL\n");
	}
	fprintf(stderr,"Attempting to reconnect in 1 seconds...\n");
	sleep(1);
	if ( receiving == 1 ) {
	  toggle_receive(arg.np, arg.pc, 0);
	  receiving = 0;
	}
	sleep(1);
	fprintf(stderr, "fscc_connect failed with %d\n", ee);
	fprintf(stderr, "I'll try again in a few seconds.\n");
	printf("Purging...\n");
	ee = fscc_purge( h, 1, 1);
	sleep(1);
	if ( ee == 0 ){
	  fprintf(stderr,"Attempting to get back on the horn and re-enable receiving...\n");
	  e = toggle_receive(arg.np, arg.pc, 1);
	  receiving = 1;
	}	
	else if ( ee != 0 ) { 
	  while( ee != 0) {
	    fprintf(stderr,"Timed out--are you sure you're providing a clock signal?\n");
	    fprintf(stderr, "Sleep it off for 1 second...\n");
	    sleep(1);
	    ee = fscc_purge(h, 1, 1);
	    if ( ee == 0 ){
	      fprintf(stderr,"Attempting to get back on the horn and re-enable receiving...\n");
	      e = toggle_receive(arg.np, arg.pc, 1);
	      receiving = 1;
	    }
	  }
 	}
      } 
    }
    else {
      if (count > MIN_BYTES_READ) {
	wcount += count;
	ret = fwrite(dataz, 1, count, ofile);
	if (ret != count) {
	fscc_log("Failed to write data, port %i: %i.", arg.np, e);
	  fprintf(stderr, "error writing to file\n");
	}
	else if ((i++ % 10) == 0)  {
	  printf("Read %i bytes of data\n", ret);
	}
      }
      else if ( count != 0 ){
	wcount += count;
	fprintf(stderr,"Only read %i bytes--I'll try to figure this out...\n",count);
	sleep(1);
	
	if (receiving) {
	  toggle_receive(arg.np, arg.pc, 0);
	  receiving = 0;
	}
	sleep(1);
      }
    }
    
    gettimeofday(&then, NULL);
    //if (arg.np == 1) { printf("r"); fflush(stdout); }
    //        check_acq_seq(dev_handle, arg.np, &fifo_acqseq);
      
      // Good read
	
    if (arg.o.debug) { printf("Serial port %i FIFO done.\n", arg.np); fflush(stdout); }
	
      // copy and write
	
      // Check DSP header position within FIFO
      // "Dartmouth College "
      // "aDtromtu hoCllge e"
      //            if (arg.o.debug) {
	
      //            printf("p%i: %i\n",arg.np,hptr[16]*65536+hptr[17]); fflush(stdout);
      //            printf("%li.",hptr-dataz);
      //			fscc_log("Bad Colonel Frame Header Shift on module %i, frame %llu: %i.\n", arg.np, hptr-cframe->base, frames);
      //    			printe("CFHS on module %i, seq %i: %i.\n", arg.np, frames, hptr-dataz);
      //            }
	
      // Check alternating LSB position within Colonel Frame
      /*            if (arg.o.debug) {
		    for (int i = 0; i < 175; i++) {
		    ret = cfshort[31+i]&0b1;
		    if (ret) {
		    ret = i;
		    break;
		    }
		    }
		    if (ret != 3) {
		    fscc_log("Bad LSB Pattern Shift on module %i, frame %llu: %i.\n", arg.np, hptr-dataz, frames);
		    printe("Bad LSBPS on module %i, frame %llu: %i.\n", arg.np, hptr-dataz, frames);
		    }
		    } // if debug*/
	
      // Build add-on frame header
      /* memset(&sync, 0, 16); */
      /* strncpy(sync.pattern, "\xFE\x6B\x28\x40", 4); */
      /* sync.t_sec = then.tv_sec-TIME_OFFSET; */
      /* sync.t_usec = then.tv_usec; */
      /* sync.size = count; */
	
      //if (arg.np == 1) { printf("b"); fflush(stdout); }
      //	        check_acq_seq(dev_handle, arg.np, &fifo_acqseq);
      //if (arg.np == 1) { printf("sc: %lu\n", (unsigned long) size_commit); fflush(stdout); }
      // Write header and frame to disk
      //      ret = fwrite(&sync, 1, sizeof(struct frame_sync), ofile);
      //      if (ret != sizeof(struct frame_sync))
      //	fscc_log("Failed to write sync, port %i: %i.", arg.np, ret);
      //printf("foo"); fflush(stdout);
      //            fflush(ofile);
	
      //if (arg.np == 1) { printf("w"); fflush(stdout); }
      //	        check_acq_seq(dev_handle, arg.np, &fifo_acqseq);
    if ((arg.o.dt > 0) && (count == arg.o.acqsize)) {
	// Copy into RTD memory if we're running the display
	  
      pthread_mutex_lock(arg.rlock);
      if (arg.o.debug)
	printf("port %i rtd moving rtdbytes %i from cfb %p to rtdb %p with %u avail.\n",
	       arg.np, rtdbytes, dataz, arg.rtdframe, count);
      memmove(arg.rtdframe, dataz, rtdbytes);
      pthread_mutex_unlock(arg.rlock);
    }
	
    frames++;
    
    if ((arg.o.maxacq > 0) && (frames > arg.o.maxacq)) {
      *arg.running = false;
    }
    
  }
    
  gettimeofday(&now, NULL);
    
  /* Close the module */
  e = fscc_disconnect(h);
  if (e != 0) {
    printe("Couldn't close module %i: %li.\n", arg.np, e);
    arg.retval = e;
  }
    
  telapsed = now.tv_sec-start.tv_sec + 1E-6*(now.tv_usec-start.tv_usec);
    
  fscc_log("Port %i read %llu bytes in %.4f s: %.4f KBps.", arg.np, count, telapsed, (count/1024.0)/telapsed);
  printf("Port %i read %u bytes in %.4f s: %.4f KBps.\n", arg.np, count, telapsed, (count/1024.0)/telapsed);
    
  fscc_log("Port %i wrote %lli bytes in %.4f s: %.4f KBps.", arg.np, wcount, telapsed, (wcount/1024.0)/telapsed);
  printf("Port %i wrote %lli bytes in %.4f s: %.4f KBps.\n", arg.np, wcount, telapsed, (wcount/1024.0)/telapsed);
    
  arg.retval = EXIT_SUCCESS; pthread_exit((void *) &arg.retval);
}

static void do_depart(int signum) {
  running = false;
  fprintf(stderr,"\nStopping...");
  
  return;
}
