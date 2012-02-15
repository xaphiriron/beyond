/* This file is part of "beyond (or whatever it's going to eventually be called) game thing".
 * copyright 2012 xax
 * "beyond (or whatever it's going to eventually be called) game thing" is free
 * software: for full terms and conditions, and disclaimers, see COPYING and
 * src/beyond.c, respectively.
 */

#ifndef XPH_COMP_OPTLAYOUT_H
#define XPH_COMP_OPTLAYOUT_H

#include "entity.h"

#include "component_input.h"
#include "comp_clickable.h"

struct optlayout
{
	Dynarr
		options;
	int
		lines,
		hasFocus;
	Entity
		confirm,
		cancel;
};

struct option
{
	char
		name[32],
		info[256];

	bool
		textFocus;
	char
		dataAsString[32];
	int
		dataCursor;
	enum option_data_types
	{
		OPT_STRING,
		OPT_NUM,
		OPT_FLAG,
		OPT_KEY,
	} type;
	union option_data_extvals
	{
		struct opt_num
		{
			signed long
				num,
				minLimit,
				maxLimit;
		} num;
		struct opt_bool
		{
			bool
				val;
		} flag;
	} ext;
};

void optlayout_define (EntComponent comp, EntSpeech speech);

void optlayout_confirm (Entity this, actionCallback callback);

void optlayout_addOption (Entity this, const char * name, enum option_data_types type, const char * defaultVal, const char * info);

const char * optlayout_optionStrValue (Entity this, const char * name);
signed long optlayout_optionNumValue (Entity this, const char * name);
bool optlayout_optionIsNumeric (Entity this, const char * name);

unsigned long hash (const char * str);

#endif /* XPH_COMP_OPTLAYOUT_H */