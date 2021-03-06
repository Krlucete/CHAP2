#include "main.h"
#include "regions.h"
#include "write_maf.h"
#include "read_algn.h"
#include "util.h"
#include "util_i.h"
#include "read_maf.h"

extern char S[BIG], T[BIG];
extern int debug_mode;

void write_maf(char *fname, struct DotList *algns, int num_algns, struct r_list *rp1, struct r_list *rp2, char *sp1_name, char *sp2_name, int len1, int len2, FILE *g)
{
	int i, j;
	int *cid;
	bool *is_written;
	FILE *f;
	int pid;
	struct b_list *binfo;
	char S1[BIG], T1[BIG];
	int num_col;
	int len;
	int e1 = 0, e2 = 0;

	cid = (int *) ckalloc(num_algns * sizeof(int));
	is_written = (bool *) ckalloc(num_algns * sizeof(bool));
	binfo = (struct b_list *) ckalloc(sizeof(struct b_list));

	for( i = 0; i < num_algns; i++ ) {
		cid[i] = -1;
		is_written[i] = false;
	}

	for( i = 0; i < num_algns; i++ ) {
		if( algns[i].c_id != -1 ) cid[algns[i].c_id] = algns[i].fid;
	}

	if( debug_mode == TRUE ) {
		for( i = 0; i < num_algns; i++ ) {
			if( cid[i] != -1 ) printf("%d %d\n", i, cid[i]);
			else printf("%d -1\n", i);
		}
	}

  binfo->b1 = 0;
  binfo->e1 = 1;
  binfo->b2 = 0;
  binfo->e2 = 1;
  binfo->strand = '+';
  binfo->pid = 0;
	binfo->len1 = len1;
	binfo->len2 = len2;

	f = ckopen(fname, "w");
	fprintf(f, "##maf version=1 scoring=lastz-pid\n");
	for( i = 0; i < num_algns; i++ ) {
		if( is_written[i] == false ) {
			num_col = get_algn_ch(S1, T1, algns[i], algns, g, rp1, rp2, binfo, cid, is_written);
			pid = (int)(cal_pid_maf(S1, T1, num_col) + 0.5);
			len = strlen(S1);
			e1 = 0;
			e2 = 0;
  		for( j = 0; j < len; j++ ) {
	  	  if( strchr("ACGTN", toupper(S1[i])) ) e1++;
		 	  if( strchr("ACGTN", toupper(T1[i])) ) e2++;
		  }

			if( (e1 >= MIN_GAP) && (e2 >= MIN_GAP) ) {
				binfo->e1 = e1;
				binfo->e2 = e2;
      	fprintf(f, "a score=%d\n", pid);
				fprintf(f, "s %s %d %d + %d %s", sp1_name, binfo->b1, binfo->e1, binfo->len1, S1);
				if( S1[len-1] != '\n' ) fprintf(f, "\n");
				fprintf(f, "s %s %d %d %c %d %s", sp2_name, binfo->b2, binfo->e2, binfo->strand, binfo->len2, T1);
				if( T1[len-1] != '\n' ) fprintf(f, "\n");
				fprintf(f, "\n");
			}
		}
	}

	free(binfo);
	free(cid);
	free(is_written);
	fclose(f);
}

void adjust_two_ch_algns(struct DotList *algns, int id1, int id2, int beg1, int beg2, FILE *fp, int *fst_end, int *sec_beg, bool is_x, struct r_list *rp1, struct r_list *rp2, int rp_id1, int rp_id2) // id1 is algns[id1].index
{
	char S1[BIG] = "", S2[BIG] = "", T1[BIG] = "", T2[BIG] = "";
	struct b_list *a1_info = NULL, *a2_info = NULL;
	int i = 0, j = 0, len = 0;
	int t_b = 0, t_e = 1;
	float pid1 = 0, pid2 = 0;
	int s1 = 0, s2 = 0, e1 = 1;
	int t1 = 1, t2 = 1;
	int cut_len1 = 0, cut_len2 = 0;
	int end_pos = 1, end_pos2 = 1;
	int fid1 = 0, fid2 = 0;

	fid1 = algns[id1].fid;
	fid2 = algns[id2].fid;
	a1_info = (struct b_list *) ckalloc(sizeof(struct b_list));
	a2_info = (struct b_list *) ckalloc(sizeof(struct b_list));

  a1_info->b1 = 0;
  a1_info->e1 = 1;
  a1_info->len1 = 0;
  a1_info->b2 = 0;
  a1_info->e2 = 1;
  a1_info->len2 = 0;
  a1_info->strand = '+';
  a1_info->pid = 0;
  a2_info->b1 = 0;
  a2_info->e1 = 1;
  a2_info->len1 = 0;
  a2_info->b2 = 0;
  a2_info->e2 = 1;
  a2_info->len2 = 0;
  a2_info->strand = '+';
  a2_info->pid = 0;

	get_nth_algn(S1, T1, fid1, beg1, fp, a1_info, REG); // algn[old_id]
	get_nth_algn(S2, T2, fid2, beg2, fp, a2_info, REG); // algn[cur_id]

	if( S1[strlen(S1)-1] == '\n' ) end_pos = strlen(S1)-1;
	else end_pos = strlen(S1);

	if( S2[strlen(S2)-1] == '\n' ) end_pos2 = strlen(S2) - 1;
	else end_pos2 = strlen(S2);

	if( is_x == true ) // get rid of overlapped segments in the first sequence
	{
		s1 = (*a1_info).b1;
		s2 = (*a2_info).b1;
		e1 = (*a1_info).b1 + (*a1_info).len1; // the endpoint + 1

		if( (*a1_info).strand == '+' ) {
			t1 = (*a1_info).b2 + (*a1_info).len2; // the endpoint + 1
			t2 = (*a2_info).b2;
		}
		else if( (*a1_info).strand == '-' ) {
			t1 = (*a2_info).b2; // the endpoint + 1
			t2 = (*a1_info).b2 - (*a1_info).len1;
		}

		if( rp_id2 == -1 ) {
			cut_len1 = 0;
			cut_len2 = 0;
		}
		else {
			if( t1 > rp2[rp_id2].start ) {
				if( (*a1_info).strand == '+' ) cut_len1 = t1 - rp2[rp_id2].start;
				else if( (*a1_info).strand == '-' ) cut_len2 = t1 - rp2[rp_id2].start;
			}
			else cut_len1 = 0;

			if( t2 < rp2[rp_id2].end ) {
				if( (*a1_info).strand == '+' ) cut_len2 = rp2[rp_id2].end - t2 + 1;
				else if( (*a1_info).strand == '-' ) cut_len1 = rp2[rp_id2].end - t2 + 1;
			}
			else cut_len2 = 0;
		}

		if( (s1 <= s2) && (e1 >= s2) ) {
			if( cut_len1 > 0 ) {
				j = end_pos-1;
				for( i = cut_len1; i > 0; i-- ) {
					while((j > 0) && (T1[j] == '-') ) j--;
					j--;
				}
				if( j < 0 ) j = 0;
				cut_len1 = j+1;
				e1 = find_xloc_one(algns[id1], fp, j+1, GAP_INC); // the endpoint + 1
			}
			else {	
				cut_len1 = end_pos;
			}

			if( cut_len2 > 0 ) {
				j = 0;
				for( i = 0; i < cut_len2; i++ ) {
					while( (j < end_pos2) && (T2[j] == '-') ) j++;
					j++;
				}
				if( j > end_pos2) j = end_pos2;
				cut_len2 = j;
				s2 = find_xloc_one(algns[id2], fp, j, GAP_INC) - 1; // the starting point of the following sequence
			}
			else cut_len2 = 0;

			if( e1 >= s2 )
			{
				len = e1 - s2 + 1;
				j = cut_len1 - 1;
				for( i = len; i > 0; i-- ) {
					while( (j > 0) && (S1[j] == '-') ) j--;
					j--;
				}
				if( j < 0 ) j = 0;
				t_b = j+1; // t_b is a position in the alignment
				if( j <= 0 ) pid1 = 0;
				else pid1 = cal_pid_maf_beg(S1, T1, t_b, cut_len1);

				j = cut_len2;
				for( i = 0; i < len; i++ ) {
					while( (j < end_pos2) && (S2[j] == '-') ) j++;
					j++;
				}
				if( j > end_pos2 ) j = end_pos2;
				t_e = j; // t_e is a count of nucleotides including gaps
				if( j >= end_pos2 ) pid2 = 0;
				else pid2 = cal_pid_maf(S2, T2, t_e);

				if( pid1 >= pid2 ) {
					if( pid1 < ((*a1_info).pid - 3) ) {
						*fst_end = t_b;
						*sec_beg = t_e;
					}
					else {
						*fst_end = cut_len1;
						*sec_beg = t_e;
					}
				}
				else {
					if( pid2 < ((*a2_info).pid - 3) ) {
						*fst_end = t_b;
						*sec_beg = t_e;
					}
					else {
						*fst_end = t_b;
						*sec_beg = cut_len2;
					}
				}
			}
			else {
				*fst_end = cut_len1;
				*sec_beg = cut_len2;
			}
		}
		else {
			printf("%d-%d,%d-%d and %d-%d,%d-%d\n", algns[id1].x.lower, algns[id1].x.upper, algns[id1].y.lower, algns[id1].y.upper, algns[id2].x.lower, algns[id2].x.upper, algns[id2].y.lower, algns[id2].y.upper);
			fatalf("unexpected chains 1\n");
		}	
	}
	else if( is_x == false ) // get rid of overlapped segments in the second sequence
	{
		if((*a1_info).strand == '+') {
			s1 = (*a1_info).b2;
			s2 = (*a2_info).b2;
			e1 = (*a1_info).b2 + (*a1_info).len2; // the endpoint + 1
		}
		else if((*a1_info).strand == '-') {
			s2 = (*a1_info).b2 - (*a1_info).len2;
			s1 = (*a2_info).b2 - (*a2_info).len2;
			e1 = (*a2_info).b2; // the endpoint + 1
		}

		t1 = (*a1_info).b1 + (*a1_info).len1; // the endpoint + 1
		t2 = (*a2_info).b1;

		if( rp_id1 == -1 ) {
			cut_len1 = 0;
			cut_len2 = 0;
		}
		else {
			if( t1 > rp1[rp_id1].start ) {
				cut_len1 = t1 - rp1[rp_id1].start;
			}
			else cut_len1 = 0;

			if( t2 < rp1[rp_id1].end ) {
				cut_len2 = rp1[rp_id1].end - t2 + 1;
			}
			else cut_len2 = 0;
		}

		if( (s1 <= s2) && (e1 >= s2) ) {
			if( cut_len1 > 0 ) {
				j = end_pos - 1;
				for( i = cut_len1; i > 0; i-- ) {
					while( (j > 0) && (S1[j] == '-') ) j--;
					j--;
				}
				if( j < 0 ) j = 0;
				cut_len1 = j+1; // the location of newline or null character in S1
				if( (*a1_info).strand == '+' ) e1 = find_yloc_one(algns[id1], fp, j+1, GAP_INC); // the endpoint + 1
				else if( (*a1_info).strand == '-' ) s2 = find_yloc_one(algns[id1], fp, j+1, GAP_INC) - 1; // the starting point of the second sequence (y) in the first alignment
			}
			else {
				cut_len1 = end_pos;
			}

			if( cut_len2 > 0 ) {
				j = 0;
				for( i = 0; i < cut_len2; i++ ) {
					while( (j < end_pos2) && (S2[j] == '-') ) j++;
					j++;
				}

				if( j > end_pos2 ) j = end_pos2;
				cut_len2 = j;
				if( (*a1_info).strand == '+' ) s2 = find_yloc_one(algns[id2], fp, j, GAP_INC) - 1; // the starting point of the following sequence
				else if( (*a1_info).strand == '-' ) e1 = find_yloc_one(algns[id2], fp, j, GAP_INC); // the end point + 1 
			}
			else cut_len2 = 0;

			if( e1 > s2 ) {
				len = e1 - s2;
				j = cut_len1 - 1;
				for( i = len; i > 0; i-- ) {
					while( (j > 0) && (T1[j] == '-') ) j--;
					while( T1[j] == '-' ) j--;
					j--;
				}

				if( j < 0 ) j = 0;
				t_b = j+1; // t_b is a position which null character is located in
				if( j <= 0 ) pid1 = 0;
				else pid1 = cal_pid_maf_beg(S1, T1, t_b, cut_len1);

				j = cut_len2;
				for( i = 0; i < len; i++ ) {
					while( (j < end_pos2) && (T2[j] == '-') ) j++;
					j++;
				}
				if( j > end_pos2 ) j = end_pos2;
				t_e = j; // t_e is a starting position of the following sequence
				if( j >= end_pos2 ) pid2 = 0;
				else pid2 = cal_pid_maf(S2, T2, t_e);

				if( pid1 >= pid2 ) {
					if( pid1 < ((*a1_info).pid - 3) ) {
						*fst_end = t_b;
						*sec_beg = t_e;
					}
					else {
						*fst_end = cut_len1;
						*sec_beg = t_e;
					}
				}
				else {
					if( pid2 < ((*a2_info).pid - 3) ) {
						*fst_end = t_b;
						*sec_beg = t_e;
					}
					else {
						*fst_end = t_b;
						*sec_beg = cut_len2;
					}
				}
			}
			else {
				*fst_end = cut_len1;
				*sec_beg = cut_len2;
			}
		}
		else {
			printf("%d-%d,%d-%d and %d-%d,%d-%d\n", algns[id1].x.lower, algns[id1].x.upper, algns[id1].y.lower, algns[id1].y.upper, algns[id2].x.lower, algns[id2].x.upper, algns[id2].y.lower, algns[id2].y.upper);
			fatalf("unexpected chains 2\n");
		}	
	}

	free(a2_info);
	free(a1_info);
}

int get_algn_ch(char *S1, char *T1, struct DotList algn, struct DotList *init_algns, FILE *fp, struct r_list *rp1, struct r_list *rp2, struct b_list *binfo, int *cid,  bool *is_written)
{
	int old_id, cur_id;
	int *sec_beg, *fst_end;
	int f_len; // f_len is the length of nucleotides already saved in S1 and T1 
	int b;
	struct I temp;
	int y_old, y_cur;
	struct b_list *a_info;
	int i;
	int beg1 = 0, beg2 = 0;
	int rp1_id, rp2_id;
	int gap_len1, gap_len2;
	int t_x1, t_x2, t_y1, t_y2;
	int e1, e2;

	a_info = (struct b_list *) ckalloc(sizeof(struct b_list));
	sec_beg = (int *) ckalloc(sizeof(int));
	fst_end = (int *) ckalloc(sizeof(int));

	*sec_beg = 0;
	*fst_end = 0;
  a_info->b1 = 0;
  a_info->e1 = 1;
  a_info->len1 = 0;
  a_info->b2 = 0;
  a_info->e2 = 1;
  a_info->len2 = 0;
  a_info->strand = '+';
  a_info->pid = 0;
	
	(*binfo).b1 = algn.x.lower-1; // convert 1-start into 0-start
	if( algn.sign == 0 ) {
		(*binfo).b2 = algn.y.lower-1;
		(*binfo).strand = '+';
	}
	else if( algn.sign == 1 ) {
		(*binfo).b2 = (*binfo).len2 - algn.y.upper + 1;
		(*binfo).strand = '-';
	}

	b = algn.x.lower;
	if( (cid[algn.fid] == -1) || ((cid[algn.fid] != -1) && (is_written[cid[algn.fid]] == true)) ) {
		get_nth_algn(S1, T1, algn.fid, 0, fp, a_info, REG);
		cur_id = -1;
	}
	else {
		f_len = 0;
		get_nth_algn(S1, T1, algn.fid, 0, fp, a_info, REG); 
		old_id = algn.fid;
		cur_id = cid[old_id];
	}

	while((cur_id != -1) && (is_written[cur_id] == false)) {
		is_written[cur_id] = true;
		*sec_beg = 0;

		rp1_id = init_algns[cur_id].rp1_id;
		rp2_id = init_algns[cur_id].rp2_id;
		if( debug_mode == TRUE ) {
			if( rp1_id != -1 ) {
				printf("A repeat in %d-%d inserted in seq1\n", rp1[rp1_id].start, rp1[rp1_id].end);
			}
			else if( init_algns[cur_id].rp2_id != -1 ) {
				printf("A repeat in %d-%d inserted in seq1\n", rp2[rp2_id].start, rp2[rp2_id].end);
			}
			else printf("No repeat insertead but chained\n");
		}

		if( init_algns[cur_id].x.lower < init_algns[old_id].x.lower ) {
			printf("%d-%d, %d-%d and %d-%d, %d-%d\n", init_algns[cur_id].x.lower, init_algns[cur_id].x.upper, init_algns[cur_id].y.lower, init_algns[cur_id].y.upper, init_algns[old_id].x.lower, init_algns[old_id].x.upper, init_algns[old_id].y.lower, init_algns[old_id].y.upper);
			fatalf("chaining info error 1\n");
		}
		else if( (proper_overlap(init_algns[old_id].x, init_algns[cur_id].x) == true) && (proper_overlap(init_algns[old_id].y, init_algns[cur_id].y) == true) ) 
		{
			temp = intersect(init_algns[old_id].x, init_algns[cur_id].x);
			if( init_algns[cur_id].sign == 0 ) {
				y_cur = init_algns[cur_id].y.lower;
				y_old = find_yloc_one(init_algns[old_id], fp, temp.lower-init_algns[old_id].x.lower, NO_GAP_INC);
			}
			else if( init_algns[cur_id].sign == 1 ) {
				y_cur = find_yloc_one(init_algns[old_id], fp, temp.lower-init_algns[old_id].x.lower, NO_GAP_INC);
				y_old = init_algns[cur_id].y.upper;
			}

			if(y_old <= y_cur) {
				adjust_two_ch_algns(init_algns, init_algns[old_id].fid, init_algns[cur_id].fid, beg1, beg2, fp, fst_end, sec_beg, true, rp1, rp2, rp1_id, rp2_id);
				S1[f_len + (*fst_end)] = '\0';
				T1[f_len + (*fst_end)] = '\0';
				t_x1 = find_xloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
				t_x2 = find_xloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
				t_y1 = find_yloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
				t_y2 = find_yloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
				gap_len1 = abs(t_x2 - t_x1 - 1);
				gap_len2 = abs(t_y2 - t_y1 - 1);
				fill_N(gap_len1, gap_len2, S1, T1);
			}
			else if( y_old > y_cur ) {
				adjust_two_ch_algns(init_algns, init_algns[old_id].fid, init_algns[cur_id].fid, beg1, beg2, fp, fst_end, sec_beg, false, rp1, rp2, rp1_id, rp2_id);											
				S1[f_len + (*fst_end)] = '\0';
				T1[f_len + (*fst_end)] = '\0';
				t_x1 = find_xloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
				t_x2 = find_xloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
				t_y1 = find_yloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
				t_y2 = find_yloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
				gap_len1 = abs(t_x2 - t_x1 - 1);
				gap_len2 = abs(t_y2 - t_y1 - 1);
				fill_N(gap_len1, gap_len2, S1, T1);
			}
		}
		else if( proper_overlap(init_algns[old_id].x, init_algns[cur_id].x) == true ) 
		{ 
			adjust_two_ch_algns(init_algns, init_algns[old_id].fid, init_algns[cur_id].fid, beg1, beg2, fp, fst_end, sec_beg, true, rp1, rp2, rp1_id, rp2_id);											
			S1[f_len + (*fst_end)] = '\0';
			T1[f_len + (*fst_end)] = '\0';
			t_x1 = find_xloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
			t_x2 = find_xloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
			t_y1 = find_yloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
			t_y2 = find_yloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
			gap_len1 = abs(t_x2 - t_x1 - 1);
			gap_len2 = abs(t_y2 - t_y1 - 1);
			fill_N(gap_len1, gap_len2, S1, T1);
		}
		else if( proper_overlap(init_algns[old_id].y, init_algns[cur_id].y) == true ) 
		{
			adjust_two_ch_algns(init_algns, init_algns[old_id].fid, init_algns[cur_id].fid, beg1, beg2, fp, fst_end, sec_beg, false, rp1, rp2, rp1_id, rp2_id);											
			S1[f_len + (*fst_end)] = '\0';
			T1[f_len + (*fst_end)] = '\0';
			t_x1 = find_xloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
			t_x2 = find_xloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
			t_y1 = find_yloc_one(init_algns[old_id], fp, (*fst_end), GAP_INC) - 1; // adjusted endpoint
			t_y2 = find_yloc_one(init_algns[cur_id], fp, (*sec_beg), GAP_INC); // adjusted start point
			gap_len1 = abs(t_x2 - t_x1 - 1);
			gap_len2 = abs(t_y2 - t_y1 - 1);
			fill_N(gap_len1, gap_len2, S1, T1);
		}
		else if( (init_algns[cur_id].sign == 0) && (init_algns[cur_id].x.lower >= init_algns[old_id].x.upper) && (init_algns[cur_id].y.lower >= init_algns[old_id].y.upper) ) {
		*sec_beg = 0;
			gap_len1 = abs(init_algns[cur_id].x.lower - init_algns[old_id].x.upper);
			gap_len2 = abs(init_algns[cur_id].y.lower - init_algns[old_id].y.upper);
			fill_N(gap_len1, gap_len2, S1, T1);
		}
		else if( (init_algns[cur_id].sign == 1) && (init_algns[cur_id].x.lower >= init_algns[old_id].x.upper) && (init_algns[old_id].y.lower >= init_algns[cur_id].y.upper) ) {
		*sec_beg = 0;
			gap_len1 = abs(init_algns[cur_id].x.lower - init_algns[old_id].x.upper);
			gap_len2 = abs(init_algns[old_id].y.lower - init_algns[cur_id].y.upper);
			fill_N(gap_len1, gap_len2, S1, T1);
		}
		else {
			printf("%d-%d, %d-%d and %d-%d, %d-%d\n", init_algns[cur_id].x.lower, init_algns[cur_id].x.upper, init_algns[cur_id].y.lower, init_algns[cur_id].y.upper, init_algns[old_id].x.lower, init_algns[old_id].x.upper, init_algns[old_id].y.lower, init_algns[old_id].y.upper);
			fatalf("chaining info error 2\n");
		}

		if( S1[strlen(S1)-1] == '\n' ) f_len = strlen(S1) - 1 - (*sec_beg);
		else f_len = strlen(S1) - (*sec_beg);
		get_nth_algn(S1, T1, init_algns[cur_id].fid, (*sec_beg), fp, a_info, EXT);
		old_id = cur_id;
		cur_id = cid[old_id];
		beg1 = find_xloc_one(init_algns[old_id], fp, *sec_beg, GAP_INC) - init_algns[old_id].x.lower;
		if( (cur_id != -1) && (init_algns[cur_id].x.lower < (init_algns[old_id].x.lower + beg1)) ) {
			beg2 = init_algns[old_id].x.lower + beg1 - init_algns[cur_id].x.lower;
		}
		else beg2 = 0;
	}

	free(fst_end);
	free(sec_beg);
	free(a_info);

	if( S1[strlen(S1)-1] == '\n' ) f_len = strlen(S1)-1;
	else f_len = strlen(S1);

	e1 = 0;
	e2 = 0;
	for( i = 0; i < f_len; i++ ) {
		if( strchr("ACGTN", toupper(S1[i])) ) e1++;
		if( strchr("ACGTN", toupper(T1[i])) ) e2++;
	}

	(*binfo).e1 = e1;
	(*binfo).e2 = e2;
	return(f_len);
}
