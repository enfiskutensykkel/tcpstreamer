#ifndef __STREAMERCTL__
#define __STREAMERCTL__

#include "streamer.h"


/* Streamer table entry */
typedef struct {
	char const *name;
	streamer_t streamer;
	void (*init)(streamer_t);
	void (*uninit)(streamer_t);
} tblent_t;



/* Bootstrap all streamers and create streamer table */
int streamer_tbl_create(tblent_t** table);



/* Destroy the streamer table */
void streamer_tbl_destroy(tblent_t* table);



/* Run streamer intance */
int streamer(tblent_t* table_entry, unsigned duration, int connection, cond_t* condition, char const **arguments);

#endif
