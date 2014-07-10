#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>

#include <fscc.h> /* fscc_* */

#define DATA_LENGTH 3000
#define TRY_LOOP

int main(void)
{
    fscc_handle h;
    struct fscc_registers regs;
    int e = 0;
    char idata[DATA_LENGTH] = {0};

    unsigned i = 0;
    unsigned bytes_stored = 0;
    unsigned bytes_read = 0;
    unsigned total_bytes_read = 0;

    /* unsigned tmp; */
    unsigned timeout = 1000;
    struct fscc_memory_cap memcap;
    char fname[] = "transp_test.data";

    FILE *fp;
    fp = fopen(fname,"wb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open output file %s\n", fname);
        return EXIT_FAILURE;
    }

    e = fscc_connect(0, &h);
    if (e != 0) {
        fprintf(stderr, "fscc_connect failed with %d\n", e);
        return EXIT_FAILURE;
    }


    /*Deal with memory cap*/
    FSCC_MEMORY_CAP_INIT(memcap);

    memcap.input = 500000; /* ~1MB */
    memcap.output = 500000; /* ~ 1MB */

    fscc_set_memory_cap(h, &memcap);

    /* Initialize our registers structure */
    FSCC_REGISTERS_INIT(regs);

    /* ####CCR0#### */
    /* #(0-1)TRANSPARENT MODE (10) */
    /* #(2-4)CLOCK = MODE 2 (RxClk) (010) */
    /* #(7-5)LINE ENCODING = NRZ (000) */
    /* #(10-8)FSC = MODE 3 (011) (Sync signal found at beginning[mode 2=010] and end[mode 3=011] of data) */
    /* #(11)SHARED FLAGS (0) */
    /* #(12)ITF  0 */
    /* #(13-15)Num sync bytes = 0 (NSB = 000) */
    /* #(16-18)Num term bytes = 0 (NTB = 000) */
    /* p.registers.CCR0 = 0b000000001100001010 */
    regs.CCR0 = 0x20a;

    /* ####CCR1#### */
    /* #(3)CTSC = 0 */
    /* #(4)ZINS = 0 */
    /* #(7)SYNC2F = 0 (1 =Transfer SYNC characters to RxFIFO) */
    /* #(8)TERM2F = 0 (0 = DO NOT Transfer TERM bytes to RxFIFO) */
    /* #(12)DRCRC = 1 (Disable receive CRC checking) */
    /* p.registers.CCR1=0b1000000000000 */
    regs.CCR1 = 0x1000;

    /* Set the CCR0 and CCR1 register values */
    e = fscc_set_registers(h, &regs);
    if (e != 0) {
        fscc_disconnect(h);
        fprintf(stderr, "fscc_set_registers failed with %d\n", e);
        return EXIT_FAILURE;
    }

    /* Set clock frequency (necessary?) */
    e = fscc_set_clock_frequency(h, 9600000);
    if (e != 0) {
        fscc_disconnect(h);
        fprintf(stderr, "fscc_set_clock_frequency failed with %d\n", e);
        return EXIT_FAILURE;
    }

    e = fscc_enable_append_status(h);
    if (e != 0) {
        fscc_disconnect(h);
        fprintf(stderr, "fscc_enable_append_status failed with %d\n", e);
        return EXIT_FAILURE;
    }

    /* Purge, because we've changed to TRANSPARENT mode */
    printf("Purging...\n");
    e = fscc_purge( h, 0, 1);
    if( e == 16000) {
      fscc_disconnect(h);
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
#endif


#ifdef TRY_LOOP
    fprintf(stdout, "Press any key to stop collecting data...\n");

    while (_kbhit() == 0) {
        bytes_read = 0;
        bytes_stored = 0;

        memset(&idata, 0, sizeof(idata));

        e = fscc_read_with_timeout(h, idata, sizeof(idata), &bytes_read, timeout);
        if (e != 0) {
            fscc_disconnect(h);
            fprintf(stderr, "fscc_read_with_timeout failed with %d\n", e);
            return EXIT_FAILURE;
        }
	printf("%u\n",bytes_read);
        /* if (bytes_read) { */
	total_bytes_read += bytes_read;
	fwrite(idata, sizeof(idata[0]), bytes_read, fp);

            if (bytes_stored != bytes_read)
                fprintf(stderr, "error writing to file\n");
        }

        i++;
	printf("%u\n",i);
    }

    fprintf(stdout, "Read %i bytes of data\n", total_bytes_read);

    FSCC_REGISTERS_INIT(regs);
    regs.BGR = 0;
    regs.CCR0 = 0x00100000;
    regs.CCR1 = 0x04007008;
    regs.CCR2 = 0x00000000;

    e = fscc_set_registers(h, &regs);
    if (e != 0) {
        fscc_disconnect(h);
        fprintf(stderr, "fscc_set_registers failed with %d\n", e);
        return EXIT_FAILURE;
    }

    e = fscc_purge(h, 0, 1);
    if (e != 0) {
        fscc_disconnect(h);
        fprintf(stderr, "fscc_purge failed with %d\n", e);
        return EXIT_FAILURE;
    }
#endif

    fscc_disconnect(h);
    fclose(fp);
    
  return EXIT_SUCCESS;
}

/* This one will allow data collection to be interrupted */
#ifdef TRY_LOOP
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
#endif
