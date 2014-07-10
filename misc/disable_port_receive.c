#include <inttypes.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h> /* For sleeping... */

#include <fscc.h> /* fscc_* */

int toggle_receive(int oport, fscc_handle *oh, struct fscc_registers *pregs, int toggle)
{
  int e = 0;

  printf("CCR0: %" PRIX64 "\n", pregs->CCR0);
  /* ####CCR0#### */
  /* #(25) Disable receiver */
  if ( toggle == 1 )
    pregs->CCR0 &= ~(1 << 25 ) ;
  else if ( toggle == 0 )
    pregs->CCR0 |= (1 << 25);
  /* Set CCR0 register value */
  e = fscc_set_registers(*oh, pregs);
  if (e != 0) {
    fscc_disconnect(*oh);
    fprintf(stderr, "fscc_set_registers failed with %d\n", e);
    return EXIT_FAILURE;
  }
  if (toggle == 0 )
    printf("Port %i receiver disabled\n", oport);
  else if (toggle == 1 )
    printf("Port %i receiver enabled\n", oport);
  return EXIT_SUCCESS;
}


void fscc_status(int port, fscc_handle *ph, struct fscc_registers *pregs)
{ 
  unsigned rx_multiple_status;

  pregs->CCR0 = FSCC_UPDATE_VALUE;
  pregs->CCR1 = FSCC_UPDATE_VALUE;
  pregs->CCR2 = FSCC_UPDATE_VALUE;

  pregs->SSR = FSCC_UPDATE_VALUE;
  pregs->SMR = FSCC_UPDATE_VALUE;

  pregs->TSR = FSCC_UPDATE_VALUE;
  pregs->TMR = FSCC_UPDATE_VALUE;

  fscc_get_registers(*ph, pregs); 

  fscc_get_rx_multiple(*ph, &rx_multiple_status);

  printf("Status of FSCC-LVDS registers for port %i:\n", port);
  printf("CCR0: %" PRIX64 "\n", pregs->CCR0);
  printf("CCR1: %" PRIX64 "\n", pregs->CCR1);
  printf("CCR2: %" PRIX64 "\n", pregs->CCR2);
  printf("\n");
  printf("SSR: %" PRIX64 "\n", pregs->SSR);
  printf("SMR: %" PRIX64 "\n", pregs->SMR);
  printf("\n");
  printf("TSR: %" PRIX64 "\n", pregs->TSR);
  printf("TMR: %" PRIX64 "\n", pregs->TMR);
  printf("\n");
  printf("Rx-Multiple status: %u\n", rx_multiple_status);
}

int main(int argc, char *argv[])
{

if ( argc < 2 ) {
    printf("Usage: ./disable_port_receive PORTNUM\n");
    return EXIT_SUCCESS;
  }
  fscc_handle h;
  struct fscc_registers regs;
  
  FSCC_REGISTERS_INIT(regs);

  int port = atoi(argv[1]);
  int e = 0;

  if (( port != 0) && (port != 1)){
    fprintf(stderr,"Invalid port being used--should be '0' or '1'.\n"); 
    return EXIT_FAILURE;
  }

  e = fscc_connect(port, &h);
  if (e != 0) {
    fprintf(stderr, "fscc_connect failed with %d\n", e);
    return EXIT_FAILURE;
  }

  printf("BEFORE TOGGLE:\n");
  fscc_status(port,&h,&regs);

  toggle_receive(port,&h,&regs,0);

  printf("AFTER TOGGLE:\n");
  fscc_status(port,&h,&regs);

  /* Purge */
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

  fscc_disconnect(h);
    
  return EXIT_SUCCESS;
}
