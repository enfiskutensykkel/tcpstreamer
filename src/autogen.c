#include "streamer.h"
#include "autogen.h"
#include <stdlib.h>
#include <string.h>


/* Handle the auto-generated code */
extern struct {
	char const *name;
	void (*callback)(
			streamer_t*,
			void (**)(streamer_t),
			void (**)(streamer_t),
			void (**)(streamer_t)
			);
} __streamers[];



/* Bootstrap all streamers and create streamer table */
int streamer_tbl_create(tblent_t **tbl)
{
	int i, n;

	/* Count the number of streamers */
	for (n = 0; __streamers[n].callback != NULL; ++n);

	/* Allocate memory for streamer table */
	*tbl = malloc(sizeof(tblent_t) * (n+1));
	if (*tbl == NULL)
		return -1;

	/* Bootstrap all streamers */
	for (i = 0; i < n; ++i) {
		(*tbl[i]).name = __streamers[i].name;
		__streamers[i].callback( &((*tbl[i]).streamer), &((*tbl[i]).sighndlr), &((*tbl[i]).init), &((*tbl[i]).uninit) );
	}

	/* Set the last table entry to be zero */
	memset((*tbl)+n, 0, sizeof(tblent_t));

	/* Return numbers of table entries */
	return n;
}



/* Destroy the streamer table */
void streamer_tbl_destroy(tblent_t *tbl)
{
	if (tbl != NULL) {
		free(tbl);
	}
}
