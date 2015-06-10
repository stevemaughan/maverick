//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "defs.h"
#include "data.h"
#include "procs.h"

void init_reduction()
{
	for (int depth = 0; depth < 64; depth++)
		for (int movecount = 0; movecount < 64; movecount++)
		{
			if (depth * movecount == 0)
				reduction[depth][movecount] = 0;
			else
				reduction[depth][movecount] = max(1, (int)(log((double)depth) * log((double)movecount) / 1.7));
		}
}