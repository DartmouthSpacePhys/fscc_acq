#include <stdio.h>
#include <stdlib.h>
#include <fscc.h> /* fscc_* */

int main(void)
{
    fscc_handle h;
    unsigned matches;
    //    unsigned timeout = 1;

    fscc_connect(0, &h);

    /* RFS interrupt */
    /* fscc_track_interrupts_with_timeout(h, 0x4, &matches, timeout); */
    /* printf("Received %d 'receive frame start' interrupts in %d ms.\n",matches,timeout); */

    fscc_track_interrupts_with_blocking(h, 0x4, &matches);

    printf("Received %d 'receive frame start' interrupts.\n",matches);

    fscc_disconnect(h);

    return 0;
}
