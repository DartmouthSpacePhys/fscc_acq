/*
 * fscc_acq.h
 *
 *  Ripped off: Jun 3, 2014
 *      Author: wibble
 *      Thief: SMH
 */

#ifndef EPP_ACQ_H_
#define EPP_ACQ_H_

#define CF_HEADER_SIZE 200 // Colonel Frame header size
#define TIME_OFFSET 946684800 // Time to 2000.1.1 from Epoch

#include "fscc_struct.h"

void fscc_mport(struct fscc_opt);
void check_acq_seq(fscc_handle, int, bool *);
static void do_depart(int);
void *fscc_data_pt(void *);


#endif /* EPP_ACQ_H_ */
