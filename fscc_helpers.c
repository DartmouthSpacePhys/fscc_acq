/*
 * fscc_helpers.c
 *
 *  Ripped off: Early June (?), 2013
 *      Author: wibble
 *       Thief: SMH
 *
 */

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>

#include <fscc.h> /* fscc_* */
#include "fscc_helpers.h"
#include "defaults.h"

void printe(char *format, ...) {
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	fflush(stderr);
	va_end(args);
}

void fscc_log(char *format, ...) {
	static FILE *log = NULL;
    static bool init = true;
	static time_t start_time;
	char ostr[1024];
	struct tm ct;
	va_list args, iarg;

    va_start(args, format);

    if (init) {
    	// First run.  Initialize logs.

		va_copy(iarg, args);

		printf("Logging to");fflush(stdout);

		openlog("fscc_acq", LOG_CONS|LOG_NDELAY, LOG_LOCAL7);
		printf(" syslog");fflush(stdout);

		start_time = va_arg(iarg, time_t);
		gmtime_r(&start_time, &ct);
		sprintf(ostr, "%s-%04i%02i%02i-%02i%02i%02i-epp.log", format,
				ct.tm_year+1900, ct.tm_mon+1, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec);
		log = fopen(ostr, "a+");
		if (log == NULL) {
			syslog(LOG_ERR, "[T+%li] Opening log file '%s' failed!", time(NULL)-start_time, ostr);
			fprintf(stderr, "Opening log file '%s' failed!\n", ostr);
		} else {
			printf(" and %s", ostr); fflush(stdout);
			fprintf(log, "Logging to syslog and %s started.\n", ostr); fflush(log);
		}
		printf(" started.\n"); fflush(stdout);

		init = false;
    }
    else {
		vsprintf(ostr, format, args);
		syslog(LOG_ERR, "[T+%li] %s", time(NULL)-start_time, ostr);

		if (log != NULL) fprintf(log, "[fscc_acq T+%li] %s\n", time(NULL)-start_time, ostr);fflush(log);
    }
	va_end(args);
}

void init_opt(struct fscc_opt *o) {
  memset(o, 0, sizeof(struct fscc_opt));
  o->acqsize = DEF_TRANSP_ACQSIZE;
  memset(o->ports, 0, sizeof(int) * MAXPORTS); o->ports[0] = 1;
  o->mport = 1;
  o->oldsport = false;
  o->prefix = DEF_PREFIX;
  o->outdir = DEF_OUTDIR;
  
  o->rtdsize = DEF_RTDSIZE;
  o->rtdfile = DEF_RTDFILE;
  o->dt = DEF_RTD_DT;
  o->rtdavg = DEF_RTDAVG;

  o->maxacq = 0;
  o->xsync_mode = false;
  
  o->debug = false;
  o->verbose = false;
}

int parse_opt(struct fscc_opt *options, int argc, char **argv) {
  int ports[MAXPORTS];
  char *pn;
  int c, i = 0;
  
  while (-1 != (c = getopt(argc, argv, "A:x:p:o:OP:S:C:R:m:rd:a:XvVh"))) {
    switch (c) {
    case 'A':
      options->acqsize = strtoul(optarg, NULL, 0);
      break;
    case 'x':
      options->maxacq = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      pn = strtok(optarg, ",");
      ports[i] = strtoul(pn, NULL, 0);
      while ((pn = strtok(NULL , ",")) != NULL) {
	i++;
	ports[i] = strtoul(pn, NULL, 0);
      }
      qsort(ports, i+1, sizeof(int), int_cmp); // Sort port list
      int j;
      for (j = 0; j <= i; j++) {
	options->ports[j] = ports[j];
	options->mport = i+1;
      }
      break;
    case 'P':
      options->prefix = optarg;
      break;
    case 'o':
      options->outdir = optarg;
      break;      
    case 'S':
      options->fifosize = strtoul(optarg, NULL, 0);
      break;
    case 'R':
      options->rtdsize = strtoul(optarg, NULL, 0);
      break;
    case 'm':
      options->rtdfile = optarg;
      break;
    case 'd':
      options->dt = strtod(optarg, NULL);
      break;
    case 'a':
      options->rtdavg = strtoul(optarg, NULL, 0);
      break;
    case 'X':
      options->xsync_mode = true;
      options->acqsize = DEF_SYNCHR_ACQSIZE;
      break;
    case 'v':
      options->verbose = true;
      break;
    case 'V':
      options->debug = true;
      break;
    case 'h':
    default:
      printf("fscc_acq: Acquire data from serial port(s) via FSCC-LVDS.\n\n Options:\n");
      printf("\t-A <#>\tAcquisition request size [Default: %i].\n", DEF_TRANSP_ACQSIZE);
      printf("\t-X Use X-Sync mode (NOTE: Default acq request size becomes %i).\n",DEF_SYNCHR_ACQSIZE);
      printf("\t\t -->See FSCC-LVDS manual for more information on X-Sync mode.\n");
      printf("\t-x <#>\tMax <acqsize>-byte acquisitions [Inf].\n");
      printf("\t-p <#>\tPort(s) to acquire from (see below) [1].\n");
      printf("\t\tCan either give a single port, or a comma-separated list.\n");
      printf("\t\ti.e., \"0,1\", \"0\", or \"1\"\n");
      printf("\t-P <s>\tSet output filename prefix [%s].\n", DEF_PREFIX);
      printf("\t-o <s>\tSet output directory [%s].\n", DEF_OUTDIR);
      printf("\n");
      printf("\t-R <#>\tReal-time display output size (in words) [%i].\n", DEF_RTDSIZE);
      printf("\t-m <s>\tReal-time display file [%s].\n", DEF_RTDFILE);
      printf("\t-d <#>\tReal-time display output period [%i].\n", DEF_RTD_DT);
      printf("\t-a <#>\tNumber of RTD blocks to average [%i].\n", DEF_RTDAVG);
      printf("\n");
      printf("\t-v Be verbose.\n");
      printf("\t-V Print debug-level messages.\n");
      printf("\t-h Display this message.\n");
      exit(1);
    }
    
  }
  
  return argc;
}

/* qsort int comparison function */
int int_cmp(const void *a, const void *b)
{
  const int *ia = (const int *)a; // casting pointer types
  const int *ib = (const int *)b;
  return *ia  - *ib;
  /* integer comparison: returns negative if b > a
     and positive if a > b */
}

int init_fscc(int port, struct fscc_opt *o, struct fscc_port *c){

  int e = 0;
  
  /*Deal with memory cap*/
  FSCC_MEMORY_CAP_INIT(c->memcap);
  
  c->memcap.input = 1000000; /* ~1MB */
  c->memcap.output = 50000; /* ~50kB */
  
  fscc_set_memory_cap(c->h, &(c->memcap) );
  
  /* Initialize our registers structure */
  FSCC_REGISTERS_INIT(c->regs);
  
  if(o->xsync_mode){
  
    printf("Port %i: X-Sync mode\n",port);
    /* ####CCR0#### */
    /* #(0-1)X-SYNC MODE (01) */
    /* #(2-4)CLOCK = MODE 2 (RxClk) (010) */
    /* #(7-5)LINE ENCODING = NRZ (000) */
    /* #(10-8)FSC = MODE 0 (000) (Sync signal found at beginning[mode 2=010] and end[mode 3=011] of data) */
    /* #(11)SHARED FLAGS (0) */
    /* #(12)ITF  0 */
    /* #(13-15)Num sync bytes = 4 (NSB = 100) */
    /* #(16-18)Num term bytes = 4 (NTB = 100) */
    /* #(22) Order of bit transmission = 1 (1 = MSB first--totally crucial) */
    c->regs.CCR0 = 0x448209;
  
    /* ####CCR1#### */
    /* #(3)CTSC = 0 */
    /* #(4)ZINS = 0 */
    /* #(7)SYNC2F = 1 (1 =Transfer SYNC characters to RxFIFO) */
    /* #(8)TERM2F = 1 (0 = DO NOT Transfer TERM bytes to RxFIFO) */
    /* #(12)DRCRC = 1 (Disable receive CRC checking) */
    /* p.registers.CCR1=0b1000000000000 */
    c->regs.CCR1 = 0x1180;
  
    /* ####CCR2#### */
    /* #(0-3) Frame Sync Receive Offset = 0x0 (FSRO = 0--no offset) */
    c->regs.CCR2 = 0x0;
  
    /* ####SSR#### */
    /* As it stands, this syncs to 'lege', pushes 'lege' onto RxFIFO per SYNC2F 
     * bit in CCR1, finds the TERM bytes "000D", pushes them on the RxFIFO per
     * TERM2F bit in CCR1, and then appends the timestamp for a whopping 
     * 131204 bytes per frame.
     */
    //printf('Setting term bytes to "000D"\n');
    c->regs.SSR = 0x6567656c; //egel--Syncs to "lege" in "College" because it is 
    c->regs.SMR = 0x00000000; //twelve bytes after "D"
  
    c->regs.TSR = 0x44000000; //D000--the term register doesn't like four 
    //consecutive zeroes, so it looks for three 
    //zeroes and then a D
    c->regs.TMR = 0x00000000; //Clear term mask register

  

    /* Set the CCR0 and CCR1 register values */
    e = fscc_set_registers(c->h, &(c->regs) );
    if (e != 0) {
      fscc_disconnect(c->h);
      fprintf(stderr, "fscc_set_registers failed with %d\n", e);
      return EXIT_FAILURE;
    }
  
    /* Set clock frequency (necessary?) */
    e = fscc_set_clock_frequency(c->h, 18432000);
    if (e != 0) {
      fscc_disconnect(c->h);
      fprintf(stderr, "fscc_set_clock_frequency failed with %d\n", e);
      return EXIT_FAILURE;
    }
  
    e = fscc_disable_append_status(c->h);
    if (e != 0) {
      fscc_disconnect(c->h);
      fprintf(stderr, "fscc_disable_append_status failed with %d\n", e);
      return EXIT_FAILURE;
    }
  
    /* Append 8-byte timestamp with a call to do_gettimeofday */
    e = fscc_enable_append_timestamp(c->h);
    if (e != 0) {
      fscc_disconnect(c->h);
      fprintf(stderr, "fscc_enable_append_timestamp failed with %d\n", e);
      return EXIT_FAILURE;
    }
    
  } 
  else { //transparent mode

    printf("Port %i: Transparent mode\n",port);
    /* ####CCR0#### */
    /* #(0-1)TRANSPARENT MODE (10) */
    /* #(2-4)CLOCK = MODE 2 (RxClk) (010) */
    /* #(7-5)LINE ENCODING = NRZ (000) */
    /* #(10-8)FSC = MODE 4 (100) (Sync signal found at beginning[mode 2=010] and end[mode 3=011] of data) */
    /* #(11)SHARED FLAGS (0) */
    /* #(12)ITF  0 */
    /* #(13-15)Num sync bytes = 0 (NSB = 000) */
    /* #(16-18)Num term bytes = 0 (NTB = 000) */
    /* #(22) Order of bit transmission = 1 (1 = MSB first--totally crucial...) */
    c->regs.CCR0 = 0x40040a;

    /* ####CCR1#### */
    /* #(3)CTSC = 0 */
    /* #(4)ZINS = 0 */
    /* #(7)SYNC2F = 0 (1 =Transfer SYNC characters to RxFIFO) */
    /* #(8)TERM2F = 0 (0 = DO NOT Transfer TERM bytes to RxFIFO) */
    /* #(12)DRCRC = 1 (Disable receive CRC checking) */
    /* p.registers.CCR1=0b1000000000000 */
    c->regs.CCR1 = 0x00001000;

    /* ####CCR2#### */
    /* #(0-3) Frame Sync Receive Offset = 0b1111 (data aligned with FSR signal) */
    /* #(31-16) Receive Length Check = 0X8000 (32768 bytes) */
    c->regs.CCR2 = 0x0;
    c->regs.SSR = 0x0;
    c->regs.SMR = 0x0;
    c->regs.TSR = 0x0;
    c->regs.TMR = 0x0;

    /* Set the CCR0 and CCR1 register values */
    e = fscc_set_registers(c->h, &(c->regs));
    if (e != 0) {
        fscc_disconnect(c->h);
        fprintf(stderr, "fscc_set_registers failed with %d\n", e);
        return EXIT_FAILURE;
    }

    /* Set clock frequency (necessary?) */
    e = fscc_set_clock_frequency(c->h, 18432000);
    if (e != 0) {
        fscc_disconnect(c->h);
        fprintf(stderr, "fscc_set_clock_frequency failed with %d\n", e);
        return EXIT_FAILURE;
    }

    e = fscc_disable_append_status(c->h);
    if (e != 0) {
        fscc_disconnect(c->h);
        fprintf(stderr, "fscc_disable_append_status failed with %d\n", e);
        return EXIT_FAILURE;
    }

    e = fscc_disable_append_timestamp(c->h);
    if (e != 0) {
        fscc_disconnect(c->h);
        fprintf(stderr, "fscc_disable_append_timestamp failed with %d\n", e);
        return EXIT_FAILURE;
    }

  }

  /* Enable reading of multiple "frames" (as defined by FSR signal) at once */
  e = fscc_enable_rx_multiple(c->h);
  if (e !=0) {
	fscc_disconnect(c->h);
	fprintf(stderr, "fscc_enable_rx_multiple failed with %d\n", e);
	return EXIT_FAILURE;
  }

  /* Purge, because you never know... */
  printf("Purging...\n");
  e = fscc_purge( c->h, 1, 1);
  if( e == 16000) {
    fscc_disconnect(c->h);
    printf("Timed out--are you sure you're providing a clock signal?\n");
    return EXIT_FAILURE;
  } 
  else if(e != 0) {
    fscc_disconnect(c->h);
    fprintf(stderr, "fscc_purge failed with %d\n", e);
    return EXIT_FAILURE;    
  }

  return EXIT_SUCCESS;
  
}


int fscc_status(int port, fscc_handle h) {

  unsigned rx_multiple_status;

  struct fscc_registers regs;

  FSCC_REGISTERS_INIT(regs);

  regs.CCR0 = FSCC_UPDATE_VALUE;
  regs.CCR1 = FSCC_UPDATE_VALUE;
  regs.CCR2 = FSCC_UPDATE_VALUE;

  regs.SSR = FSCC_UPDATE_VALUE;
  regs.SMR = FSCC_UPDATE_VALUE;

  regs.TSR = FSCC_UPDATE_VALUE;
  regs.TMR = FSCC_UPDATE_VALUE;

  fscc_get_registers(h, &regs);

  fscc_get_rx_multiple(h, &rx_multiple_status);

  printf("Status of FSCC-LVDS registers for port %i:\n", port);
  printf("CCR0: %" PRIx64 "\n", regs.CCR0);
  printf("CCR1: %" PRIx64 "\n", regs.CCR1);
  printf("CCR2: %" PRIx64 "\n", regs.CCR2);
  printf("\n");
  printf("SSR: %" PRIx64 "\n", regs.SSR);
  printf("SMR: %" PRIx64 "\n", regs.SMR);
  printf("\n");
  printf("TSR: %" PRIx64 "\n", regs.TSR);
  printf("TMR: %" PRIx64 "\n", regs.TMR);
  printf("\n");
  printf("Rx-Multiple status: %u\n", rx_multiple_status);

  return EXIT_SUCCESS;
}


/*turn the receiver for a given port on or off */
int toggle_receive(int oport, struct fscc_port *c, int toggle)
{
  int e = 0;

  /* ####CCR0#### */
  /* #(25) Disable receiver */
  if ( toggle == 1 )
    c->regs.CCR0 &= ~(1 << 25 ) ;
  else if ( toggle == 0 )
    c->regs.CCR0 |= 1 << 25;

  /* Set CCR0 register value */
  e = fscc_set_registers(c->h, &c->regs);
  if (e != 0) {
    //    fscc_disconnect(oh);
    fprintf(stderr, "fscc_set_registers failed with %d\n", e);
    return EXIT_FAILURE;
  }
  if (toggle == 0 )
    printf("Port %i receiver disabled\n", oport);
  else if (toggle == 1 )
    printf("Port %i receiver enabled\n", oport);
  return EXIT_SUCCESS;
}
