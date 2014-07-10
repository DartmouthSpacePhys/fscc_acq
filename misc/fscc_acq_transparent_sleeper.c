#include <inttypes.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h> /* For sleeping... */

#include <fscc.h> /* fscc_* */

#define PORT 0
#define DATA_LENGTH 131200
#define MIN_BYTES_READ 100
/*
 *
 *To view data, try this in gnuplot:
 * plot '<head -20 transp_test.data' binary format="%int16%int16" endian=swap using 1 with lines, '<head -20 transp_test.data' binary format="%int16%int16" endian=swap using 2 with lines
 *
 */

/* This one will allow data collection to be interrupted */
int _kbhit(void) {
  static const int STDIN = 0;
  static unsigned initialized = 0;
  int bytes_waiting;

  if (!initialized) {
    /* Use termios to turn off line buffering */
    struct termios term;
    tcgetattr(STDIN, &term);
    term.c_lflag &= ~ICANON;
    tcsetattr(STDIN, TCSANOW, &term);
    setbuf(stdin, NULL);
    initialized = 1;
  }

  ioctl(STDIN, FIONREAD, &bytes_waiting);
  return bytes_waiting;
}

/*turn the receiver for a given port on or off */
int toggle_receive(int oport, fscc_handle *oh,int toggle)
{
  int e = 0;

  struct fscc_registers oregs;


  e = fscc_connect(oport, oh);
  if (e != 0) {
    fprintf(stderr, "fscc_connect failed with %d\n", e);
    return EXIT_FAILURE;
  }

  FSCC_REGISTERS_INIT(oregs);

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
  /* #(25) Disable receiver */
  if ( toggle == 1 )
    oregs.CCR0 = 0x040040a;
  else if ( toggle == 0 )
    oregs.CCR0 = 0x240040a;
  /* ####CCR1#### */
  /* #(3)CTSC = 0 */
  /* #(4)ZINS = 0 */
  /* #(7)SYNC2F = 0 (1 =Transfer SYNC characters to RxFIFO) */
  /* #(8)TERM2F = 0 (0 = DO NOT Transfer TERM bytes to RxFIFO) */
  /* #(12)DRCRC = 1 (Disable receive CRC checking) */
  /* p.registers.CCR1=0b1000000000000 */
  oregs.CCR1 = 0x00001000;

  /* ####CCR2#### */
  /* #(0-3) Frame Sync Receive Offset = 0b1111 (data aligned with FSR signal) */
  oregs.CCR2 = 0x0;


  /* Set the CCR{0,1,2} register values */
  e = fscc_set_registers(*oh, &oregs);
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

int main(void)
{
  fscc_handle h, oh;

  struct fscc_registers regs;
  unsigned rx_multiple_status;

  int e = 0;
  int ee = 0;
  int receiving;
  char idata[DATA_LENGTH] = {0};

  int port = PORT;
  int oport;
    
  int i;
  unsigned bytes_stored = 0;
  unsigned bytes_read = 0;
  unsigned total_bytes_read = 0;

  /* unsigned tmp; */
  unsigned timeout = 1000;
  struct fscc_memory_cap memcap;
  char fname[] = "/home/spencerh/data/master_test.data";

  FILE *fp;
  fp = fopen(fname,"wb");
  if (fp == NULL) {
    fprintf(stderr, "cannot open output file %s\n", fname);
    return EXIT_FAILURE;
  }

  /* Disable the other receiver */
  if (port == 0 ) {
    oport = 1;
  }
  else if (port == 1) {
    oport = 0;
  }
  else {
    fprintf(stderr,"Invalid port being used--should be '0' or '1'.\n"); 
    return EXIT_FAILURE;
  }

  e = fscc_connect(oport, &oh);
  if (e != 0) {
    fprintf(stderr, "fscc_connect failed with %d\n", e);
    return EXIT_FAILURE;
  }
  e = toggle_receive(oport,&oh,0);

  e = fscc_connect(port, &h);
  if (e != 0) {
    fprintf(stderr, "fscc_connect failed with %d\n", e);
    return EXIT_FAILURE;
  }


  /*Deal with memory cap*/
  FSCC_MEMORY_CAP_INIT(memcap);

  memcap.input = 1000000; /* ~1MB */
  memcap.output = 50000; /* ~50kB */

  fscc_set_memory_cap(h, &memcap);

  /* Initialize our registers structure */
  FSCC_REGISTERS_INIT(regs);

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
  regs.CCR0 = 0x40040a;

  /* ####CCR1#### */
  /* #(3)CTSC = 0 */
  /* #(4)ZINS = 0 */
  /* #(7)SYNC2F = 0 (1 =Transfer SYNC characters to RxFIFO) */
  /* #(8)TERM2F = 0 (0 = DO NOT Transfer TERM bytes to RxFIFO) */
  /* #(12)DRCRC = 1 (Disable receive CRC checking) */
  /* p.registers.CCR1=0b1000000000000 */
  regs.CCR1 = 0x00001000;

  /* ####CCR2#### */
  /* #(0-3) Frame Sync Receive Offset = 0b1111 (data aligned with FSR signal) */
  regs.CCR2 = 0x0;


  /* Set the CCR{0,1,2} register values */
  e = fscc_set_registers(h, &regs);
  if (e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_set_registers failed with %d\n", e);
    return EXIT_FAILURE;
  }

  /* Set clock frequency (necessary?) */
  e = fscc_set_clock_frequency(h, 18432000);
  if (e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_set_clock_frequency failed with %d\n", e);
    return EXIT_FAILURE;
  }

  e = fscc_disable_append_status(h);
  if (e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_disable_append_status failed with %d\n", e);
    return EXIT_FAILURE;
  }

  e = fscc_disable_append_timestamp(h);
  if (e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_disable_append_timestamp failed with %d\n", e);
    return EXIT_FAILURE;
  }


  /* Enable reading of multiple "frames" (as defined by FSR signal) at once */
  e = fscc_enable_rx_multiple(h);
  if (e !=0) {
	fscc_disconnect(h);
	fprintf(stderr, "fscc_enable_rx_multiple failed with %d\n", e);
	return EXIT_FAILURE;
  }

  /* Purge, because we've changed to TRANSPARENT mode */
  printf("Purging...\n");
  e = fscc_purge( h, 1, 1);
  if( e == 16000) {
    fscc_disconnect(h);
    fprintf(stderr,"Timed out--are you sure you're providing a clock signal?\n");
    fprintf(stderr,"Laterz...\n");
    return EXIT_FAILURE;
  } 
  else if(e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_purge failed with %d\n", e);
    return EXIT_FAILURE;    
  }

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

  fprintf(stdout, "Press any key to stop collecting data...\n");

  receiving = 1;
  while (_kbhit() == 0) {
    /* while (i < 10) { */
    bytes_read = 0;
    bytes_stored = 0;
    usleep(100000);
    memset(&idata, 0, sizeof(idata));
    if (receiving) {
      e = fscc_read_with_timeout(h, idata, sizeof(idata), &bytes_read, timeout);
    }
    else {
      e = 1;
    }
    if( e == 16001) {
      fscc_disconnect(h);
      fprintf(stderr,"FSCC_INCORRECT_MODE\n");
      return EXIT_FAILURE;
    }
    else if( e != 0) {
      //      ee = 1;
      while (e != 0) {
	if (e == 16002) {
	  fprintf(stderr,"FSCC_BUFFER_TOO_SMALL\n");
	}
	fprintf(stderr,"Attempting to reconnect in 1 seconds...\n");
	sleep(1);
	if ( receiving == 1 ) {
	  toggle_receive(port, &h, 0);
	  receiving = 0;
	}
	//	ee = fscc_connect(port, &h);
	sleep(1);
	//	if (ee != 0) {
	//	  fprintf(stderr, "fscc_connect failed with %d\n", ee);
	//	  fprintf(stderr, "I'll try again in a few seconds.\n");
	//	}
	//	else {
	printf("Purging...\n");
	ee = fscc_purge( h, 1, 1);
	sleep(1);
	if ( ee == 0 ){
	  fprintf(stderr,"Attempting to get back on the horn and re-enable receiving...\n");
	  e = toggle_receive(port, &h, 1);
	  receiving = 1;
	}	
	else if ( ee == 16000 ) {
	  while( ee == 16000) {
	    fprintf(stderr,"Timed out--are you sure you're providing a clock signal?\n");
	    fprintf(stderr, "Sleep it off for 1 second...\n");
	    sleep(1);
	    ee = fscc_purge(h, 1, 1);
	    if ( ee == 0 ){
	      fprintf(stderr,"Attempting to get back on the horn and re-enable receiving...\n");
	      //	    if ( receiving == 0 ) {
	      e = toggle_receive(port, &h, 1);
	      receiving = 1;
	      //	      receiving = 1;
	      //	    }
	    }
	  }
 	}
      } 
    }
    else {
      if (bytes_read > MIN_BYTES_READ) {
	total_bytes_read += bytes_read;
	bytes_stored = fwrite(idata, 1, bytes_read, fp);
	if (bytes_stored != bytes_read) {
	  fprintf(stderr, "error writing to file\n");
	}
	else if ((i++ % 10) == 0)  {
	  printf("Read %i bytes of data\n", bytes_stored);
	}
      }
      else if ( bytes_read != 0 ){
	fprintf(stderr,"Only read %i bytes--I'll try to figure this out...\n",bytes_read);
	sleep(1);
	
	if (receiving) {
	  toggle_receive(port, &h, 0);
	  receiving = 0;
	}
	sleep(1);
      }
    }
  }
  
  fprintf(stdout, "Total: Read %i bytes of data\n", total_bytes_read);

  fscc_disconnect(h);
  fclose(fp);
    
  return EXIT_SUCCESS;
}
