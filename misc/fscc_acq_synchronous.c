#include <inttypes.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>

#include <fscc.h> /* fscc_* */

#define DATA_LENGTH 131204
#define TRY_LOOP

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

int main(void)
{
  fscc_handle h;

  struct fscc_registers regs;
  unsigned rx_multiple_status;

  int port = 1;
  int e = 0;
  char idata[DATA_LENGTH] = {0};

  int i;
  unsigned bytes_stored = 0;
  unsigned bytes_read = 0;
  unsigned total_bytes_read = 0;

  unsigned timeout = 1000;
  struct fscc_memory_cap memcap;
  char fname[] = "synchr_test.data";

  int term = 1;

  FILE *fp;
  fp = fopen(fname,"wb");
  if (fp == NULL) {
    fprintf(stderr, "cannot open output file %s\n", fname);
    return EXIT_FAILURE;
  }

  e = fscc_connect(port, &h);
  if (e != 0) {
    fprintf(stderr, "fscc_connect failed with %d\n", e);
    return EXIT_FAILURE;
  }

  /*Deal with memory cap*/
  FSCC_MEMORY_CAP_INIT(memcap);

  memcap.input = 600000; /* ~1MB */
  memcap.output = 50000; /* ~50kB */

  fscc_set_memory_cap(h, &memcap);

  /* Initialize our registers structure */
  FSCC_REGISTERS_INIT(regs);

  /* ####CCR0#### */
  /* #(0-1)X-SYNC MODE (01) */
  /* #(2-4)CLOCK = MODE 2 (RxClk) (010) */
  /* #(7-5)LINE ENCODING = NRZ (000) */
  /* #(10-8)FSC = MODE 3 (011) (Sync signal found at beginning[mode 2=010] and end[mode 3=011] of data) */
  /* #(11)SHARED FLAGS (0) */
  /* #(12)ITF  0 */
  /* #(13-15)Num sync bytes = 2 (NSB = 100) */
  /* #(16-18)Num term bytes = 0 (NTB = 000) */
  /* #(22) Order of bit transmission = 1 (1 = MSB first--totally crucial...) */ 
  if(term) {
    regs.CCR0 = 0x448009; /* term bytes, shared flags ON */
  }
  else {
    regs.CCR0 = 0x440C09; /* sync bytes, term bytes, shared flags ON */
  }

  /* ####CCR1#### */
  /* #(3)CTSC = 0 */
  /* #(4)ZINS = 0 */
  /* #(7)SYNC2F = 0 (1 =Transfer SYNC characters to RxFIFO) */
  /* #(8)TERM2F = 0 (0 = DO NOT Transfer TERM bytes to RxFIFO) */
  /* #(12)DRCRC = 1 (Disable receive CRC checking) */
  if(term) {
    regs.CCR1 = 0x1180;
  }
  else {
    regs.CCR1 = 0x1100;
  }

  /* ####CCR2#### */
  /* #(0-3) Frame Sync Receive Offset = 0b0 (data aligned with FSR signal) */
  regs.CCR2 = 0x0;


  /* ####SSR#### */
  /* As it stands, this syncs to 'lege', pushes 'lege' onto RxFIFO per SYNC2F 
   * bit in CCR1, finds the TERM bytes "000D", pushes them on the RxFIFO per
   * TERM2F bit in CCR1, and then appends the timestamp for a whopping 
   * 131204 bytes per frame.
   */
  if(term) {
    //      printf('Setting term bytes to %4c\n', sync);
    regs.SSR = 0x6567656c; //egel--Syncs to "lege" in "College" because it is 
    regs.SMR = 0x00000000; //twelve bytes after "D"
 
    regs.TSR = 0x44000000; //D000--the term register doesn't like four 
    //consecutive zeroes, so it looks for three 
    //and then a D
    regs.TMR = 0x00000000; //Clear term mask register
    /* regs.SSR = 0x74726144; */ //traD
    /* regs.SSR = 0x6874756f; //htuo */
  }
  else {
    printf("Setting sync bytes to Dart and Coll\n");
    regs.SSR = 0x00000000;
    regs.SMR = 0x00000000; // Clear sync mask register
    regs.TSR = 0x44617274;
    regs.TMR = 0x00000000;
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
    
  /* Set the CCR0, CCR1, and relevant sync register values */
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

  /* Append 8-byte timestamp with a call to do_gettimeofday */
  e = fscc_enable_append_timestamp(h);
  if (e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_enable_append_timestamp failed with %d\n", e);
    return EXIT_FAILURE;
  }

  /* Enable reading of multiple "frames" (as defined by FSR signal) at once */
  e = fscc_enable_rx_multiple(h);
  if (e !=0) {
	fscc_disconnect(h);
	fprintf(stderr, "fscc_enable_rx_multiple failed with %d\n", e);
	return EXIT_FAILURE;
  }

  /* Purge, because we've changed to X-SYNC mode */
  printf("Purging...\n");
  e = fscc_purge( h, 1, 1);
  if( e == 16000) {
    printf("Timed out--are you sure you're providing a clock signal?\n");
    printf("Laterz...\n");
    return EXIT_FAILURE;
  }
  else if(e != 0) {
    fscc_disconnect(h);
    fprintf(stderr, "fscc_purge failed with %d\n", e);
    return EXIT_FAILURE;    
  }


#ifdef TRY_SINGLE
  //  for ( i = 0; i < 2; i++){

    memset(&idata, 0, sizeof(idata));

    /* Do some reading */
    e = fscc_read_with_timeout(h, idata, sizeof(idata), &bytes_read, timeout);
    if( e == 16001) {
      fscc_disconnect(h);
      printf("FSCC_INCRORRECT_MODE\n");
      return EXIT_FAILURE;
    }
    else if( e == 16002 ) {
      fscc_disconnect(h);
      printf("FSCC_BUFFER_TOO_SMALL\n");
      return EXIT_FAILURE;
    }
    else if( e != 0) {
      fscc_disconnect(h);
      printf("Some error or other. Tough to say.\n");
    }
    
    /* Write the data */
    printf("Got some data, gonna write it out...\n");
    fwrite(idata, sizeof(idata[0]), sizeof(idata)/sizeof(idata[0]), fp);
    //  }
#endif

#ifdef TRY_LOOP
  fprintf(stdout, "Press any key to stop collecting data...\n");

  while (_kbhit() == 0) {
    /* while (i < 10) { */
    bytes_read = 0;
    bytes_stored = 0;

    memset(&idata, 0, sizeof(idata));

    e = fscc_read_with_timeout(h, idata, sizeof(idata), &bytes_read, timeout);
    if (e != 0) {
      fscc_disconnect(h);
      fprintf(stderr, "fscc_read_with_timeout failed with %d\n", e);
      return EXIT_FAILURE;
    }
    if (i % 100 == 0) {
      printf("%d",i);
    }
    if (bytes_read) {
      total_bytes_read += bytes_read;
      bytes_stored = fwrite(idata, 1, bytes_read, fp);

      if (bytes_stored != bytes_read)
	fprintf(stderr, "error writing to file\n");
      else if ((i++ % 10) == 0)
	printf("Read %i bytes of data\n", bytes_stored);
    }

  }

  fprintf(stdout, "Read %i bytes of data\n", total_bytes_read);
#endif
    
  fscc_disconnect(h);
  fclose(fp);

  return EXIT_SUCCESS;
}
