/*
 * fscc_helpers.h
 *
 *  Ripped off: Feb 15, 2011
 *      Author: wibble
 *      Thief: SMH
 */    

#ifndef EPP_HELPERS_H_
#define EPP_HELPERS_H_

#include <stdbool.h>
#include <stdint.h>

#include "fscc_struct.h"
#include "defaults.h"

int int_cmp(const void *, const void *);
void open_cap(int);
void strfifo(char *, short *, int);
void init_opt(struct fscc_opt *);
int parse_opt(struct fscc_opt *, int, char **);
void fscc_log(char *, ...);
int fscc_status(int, fscc_handle);
void printe(char *, ...);

int init_fscc(int, struct fscc_opt *, fscc_handle);



#endif /* EPP_HELPERS_H_ */
