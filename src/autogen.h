#ifndef __AUTOGEN__
#define __AUTOGEN__

#include "streamer.h"

/* Handle the auto-generated code for bootstrappers */
extern struct {
	char const* name;

	void (*bootstrapper)(
			streamer_t*,
			void (**)(streamer_t),
			void (**)(streamer_t),
			void (**)(streamer_t)
			);

} streamers[];

#endif
