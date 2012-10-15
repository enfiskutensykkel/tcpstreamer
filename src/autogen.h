#ifndef __AUTOGEN__
#define __AUTOGEN__

#include "streamer.h"

/* Deal with the auto-generated code for entry points */
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
