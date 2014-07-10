/*
 * fscc_helpers.h
 *
 *  Ripped off: Jun 5, 2014
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

int init_fscc(int, struct fscc_opt *, struct fscc_port *);

int toggle_receive(int, struct fscc_port *, int);


#endif /* EPP_HELPERS_H_ */
