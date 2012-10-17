#ifndef __STREAMER_TBL__
#define __STREAMER_TBL__

#include "streamer.h"



/* Streamer table entry */
typedef struct {
	char const *name;
	streamer_t streamer;
	void (*sighndlr)(streamer_t);
	void (*init)(streamer_t);
	void (*uninit)(streamer_t);
} tblent_t;



/* Bootstrap all streamers and create streamer table */
int streamer_tbl_create(tblent_t **tbl);



/* Destroy the streamer table */
void streamer_tbl_destroy(tblent_t *tbl);

#endif
