 /*
 * fscc_struct.h
 *
 *  Ripped off: Jun 3, 2014
 *      Author: wibble
 *      Thief: SMH
 *
 */

#ifndef EPP_STRUCT_H_
#define EPP_STRUCT_H_

#include "defaults.h"

/* Program options */
struct fscc_opt {
  int acqsize;
  int maxacq;
  int ports[MAXPORTS];
  int mport;
  bool oldsport;
  char *prefix;
  char *outdir;
  
  int fifosize;
  
  int rtdsize;
  char *rtdfile;
  double dt;
  int rtdavg;
  
  bool debug;
  bool verbose;
  bool xsync_mode;
};

struct module_list {
  int portc;
  char portv[127][9];
};

/* colonel frame structure */
struct colonel_frame {
  long int size;
  char *base;
  char *tail;
  int np;
};

struct fscc_ptargs {
  struct fscc_opt o;

  int np;
  time_t time;
  bool *running;
  int retval;
  char *port;
  
  short int *rtdframe;
  pthread_mutex_t *rlock;
};

/* define the header structure for monitor file */
struct header_info {
  int hkey;
  char site_id[12];
  int num_channels;
  char channel_flags;
  unsigned int num_samples;
  unsigned int num_read;
  unsigned int averages;
  float sample_frequency;
  float time_between_acquisitions;
  int byte_packing;
  time_t start_time;
  struct timeval start_timeval;
  float code_version;
};

/*
 * Define frame sync structure.  Pragma compiler directives
 * ensure this structure is properly-sized.
 */

#pragma pack(push,2)

struct frame_sync {
  char pattern[4];
  uint32_t t_sec;
  uint32_t t_usec;
  uint32_t size;
};

#pragma pack(pop)

#endif /* EPP_STRUCT_H_ */
