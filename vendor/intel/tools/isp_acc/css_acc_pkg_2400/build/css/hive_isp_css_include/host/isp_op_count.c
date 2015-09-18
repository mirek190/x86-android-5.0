/*
 * Support for Intel Camera Imaging ISP subsystem.
 *
 * Copyright (c) 2010 - 2014 Intel Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "isp_op_count.h"
#include <assert_support.h>
#include <string.h>

static bbb_stat_t bbb_func_cnts[bbb_func_num_functions];
static bbb_stat_t core_func_cnts[core_func_num_functions];

#define NAMEPADDING 30
/* More safe to use indexed asignments, but this
   is not available for all compilers.
#define NAME(a) [a] = #a
*/
#define NAME(a)  #a

static char* core_names[core_func_num_functions] = {
	NAME(core_func_OP_and),
	NAME(core_func_OP_or),
	NAME(core_func_OP_xor),
	NAME(core_func_OP_inv),
	NAME(core_func_OP_add),
	NAME(core_func_OP_sub),
	NAME(core_func_OP_addsat),
	NAME(core_func_OP_subsat),
	NAME(core_func_OP_subasr1),
	NAME(core_func_OP_abs),
	NAME(core_func_OP_subabssat),
	NAME(core_func_OP_muld),
	NAME(core_func_OP_mul),
	NAME(core_func_OP_qrmul),
	NAME(core_func_OP_eq),
	NAME(core_func_OP_ne),
	NAME(core_func_OP_le),
	NAME(core_func_OP_lt),
	NAME(core_func_OP_ge),
	NAME(core_func_OP_gt),
	NAME(core_func_OP_asr),
	NAME(core_func_OP_asl),
	NAME(core_func_OP_asrrnd),
	NAME(core_func_OP_lsl),
	NAME(core_func_OP_lslsat),
	NAME(core_func_OP_lsr),
	NAME(core_func_OP_lsrrnd),
	NAME(core_func_OP_clip_asym),
	NAME(core_func_OP_clipz),
	NAME(core_func_OP_div),
	NAME(core_func_OP_mod),
	NAME(core_func_OP_sqrt),
	NAME(core_func_OP_mux),
	NAME(core_func_OP_avgrnd),
	NAME(core_func_OP_min),
	NAME(core_func_OP_max),
};

static char* bbb_names[bbb_func_num_functions] = {
	NAME(bbb_func_OP_1w_and),
	NAME(bbb_func_OP_1w_or),
	NAME(bbb_func_OP_1w_xor),
	NAME(bbb_func_OP_1w_inv),
	NAME(bbb_func_OP_1w_add),
	NAME(bbb_func_OP_1w_sub),
	NAME(bbb_func_OP_1w_addsat),
	NAME(bbb_func_OP_1w_subsat),
	NAME(bbb_func_OP_1w_subasr1),
	NAME(bbb_func_OP_1w_abs),
	NAME(bbb_func_OP_1w_subabssat),
	NAME(bbb_func_OP_1w_muld),
	NAME(bbb_func_OP_1w_mul),
	NAME(bbb_func_OP_1w_qmul),
	NAME(bbb_func_OP_1w_qrmul),
	NAME(bbb_func_OP_1w_eq),
	NAME(bbb_func_OP_1w_ne),
	NAME(bbb_func_OP_1w_le),
	NAME(bbb_func_OP_1w_lt),
	NAME(bbb_func_OP_1w_ge),
	NAME(bbb_func_OP_1w_gt),
	NAME(bbb_func_OP_1w_asr),
	NAME(bbb_func_OP_1w_asrrnd),
	NAME(bbb_func_OP_1w_asl),
	NAME(bbb_func_OP_1w_aslsat),
	NAME(bbb_func_OP_1w_lsl),
	NAME(bbb_func_OP_1w_lsr),
	NAME(bbb_func_OP_int_cast_to_1w ),
	NAME(bbb_func_OP_1w_cast_to_int ),
	NAME(bbb_func_OP_1w_cast_to_2w ),
	NAME(bbb_func_OP_2w_cast_to_1w ),
	NAME(bbb_func_OP_2w_sat_cast_to_1w ),
	NAME(bbb_func_OP_1w_clip_asym),
	NAME(bbb_func_OP_1w_clipz),
	NAME(bbb_func_OP_1w_div),
	NAME(bbb_func_OP_1w_qdiv),
	NAME(bbb_func_OP_1w_mod),
	NAME(bbb_func_OP_1w_sqrt_u),
	NAME(bbb_func_OP_1w_mux),
	NAME(bbb_func_OP_1w_avgrnd),
	NAME(bbb_func_OP_1w_min),
	NAME(bbb_func_OP_1w_max),
	NAME(bbb_func_OP_2w_and),
	NAME(bbb_func_OP_2w_or),
	NAME(bbb_func_OP_2w_xor),
	NAME(bbb_func_OP_2w_inv),
	NAME(bbb_func_OP_2w_add),
	NAME(bbb_func_OP_2w_sub),
	NAME(bbb_func_OP_2w_addsat),
	NAME(bbb_func_OP_2w_subsat),
	NAME(bbb_func_OP_2w_subasr1),
	NAME(bbb_func_OP_2w_abs),
	NAME(bbb_func_OP_2w_subabssat),
	NAME(bbb_func_OP_2w_mul),
	NAME(bbb_func_OP_2w_qmul),
	NAME(bbb_func_OP_2w_qrmul),
	NAME(bbb_func_OP_2w_eq),
	NAME(bbb_func_OP_2w_ne),
	NAME(bbb_func_OP_2w_le),
	NAME(bbb_func_OP_2w_lt),
	NAME(bbb_func_OP_2w_ge),
	NAME(bbb_func_OP_2w_gt),
	NAME(bbb_func_OP_2w_asr),
	NAME(bbb_func_OP_2w_asrrnd),
	NAME(bbb_func_OP_2w_asl),
	NAME(bbb_func_OP_2w_aslsat),
	NAME(bbb_func_OP_2w_lsl),
	NAME(bbb_func_OP_2w_lsr),
	NAME(bbb_func_OP_2w_clip_asym),
	NAME(bbb_func_OP_2w_clipz),
	NAME(bbb_func_OP_2w_div),
	NAME(bbb_func_OP_2w_divh),
	NAME(bbb_func_OP_2w_mod),
	NAME(bbb_func_OP_2w_sqrt_u),
	NAME(bbb_func_OP_2w_mux),
	NAME(bbb_func_OP_2w_avgrnd),
	NAME(bbb_func_OP_2w_min),
	NAME(bbb_func_OP_2w_max),
	NAME(bbb_func_OP_1w_mul_realigning)
};

static int no_count;
void
inc_core_count_n(
	core_functions_t func,
	unsigned n)
{
	core_func_cnts[func].bbb_cnt += n;
}

void
_enable_bbb_count(void)
{
	no_count = 0;
}

void
_disable_bbb_count(void)
{
	no_count = 1;
}

void
_inc_bbb_count(
	bbb_functions_t func)
{
	if (no_count)
		return;

	bbb_func_cnts[func].bbb_cnt++;
	bbb_func_cnts[func].bbb_op = 1;
	bbb_func_cnts[func].total_cnt++;
}

void
_inc_bbb_count_ext(
	bbb_functions_t func,
	int op_count)
{
	if (no_count)
		return;

	bbb_func_cnts[func].bbb_cnt++;
	bbb_func_cnts[func].bbb_op = op_count;
	bbb_func_cnts[func].total_cnt += op_count;
}

void
bbb_func_reset_count(void)
{
	memset(bbb_func_cnts, 0, sizeof(bbb_func_cnts));
	no_count = 0;
}

void
bbb_func_print_totals(FILE* fp, unsigned non_zero_only)
{
	unsigned id;
	unsigned sum = 0;
	fprintf(fp, " id, %-*s,  count, op count,  total\n", NAMEPADDING, "function name");
	for (id=0; id< bbb_func_num_functions; id++)
	{
		if (!non_zero_only || (bbb_func_cnts[id].bbb_cnt > 0)) {
			fprintf(fp, "%3d, %-*s, %6d,   %6d, %6d\n",
				id,
				NAMEPADDING,
				bbb_names[id],
				bbb_func_cnts[id].bbb_cnt,
				bbb_func_cnts[id].bbb_op,
				bbb_func_cnts[id].total_cnt);

			sum += bbb_func_cnts[id].total_cnt;

			if ((bbb_func_cnts[id].bbb_cnt > 0) && (bbb_func_cnts[id].bbb_op == 0)) {
				fprintf(fp, "WARNING: bbb operation count for %s is not known!\n", bbb_names[id]);
			}
			if (bbb_names[id] == NULL){
				fprintf(fp, "WARNING: bbb name not initialized\n");
				assert(0);
			}
		}
	}
	fprintf(fp, "\nTotal operation count: %d\n", sum);
}

void
core_func_print_totals(FILE* fp, unsigned non_zero_only)
{
	unsigned id;
	unsigned sum = 0;
	fprintf(fp, " id, %-*s,  count, op count,  total\n", NAMEPADDING, "core function name");
	for (id=0; id < core_func_num_functions; id++)
	{
		if (!non_zero_only || (core_func_cnts[id].bbb_cnt > 0)) {
			fprintf(fp, "%3d, %-*s, %6d,   %6d, %6d\n",
				id,
				NAMEPADDING,
				core_names[id],
				core_func_cnts[id].bbb_cnt,
				core_func_cnts[id].bbb_op,
				core_func_cnts[id].total_cnt);

			sum += core_func_cnts[id].total_cnt;

			if ((core_func_cnts[id].bbb_cnt > 0) && (core_func_cnts[id].bbb_op == 0)) {
				fprintf(fp, "WARNING: bbb operation count for %s is not known!\n", core_names[id]);
			}
			if (core_names[id] == NULL){
				fprintf(fp, "WARNING: bbb name not initialized\n");
				assert(0);
			}
		}
	}
	fprintf(fp, "\nTotal operation count: %d\n", sum);
}

