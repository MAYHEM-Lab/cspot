#ifndef MM_MATRIX_H
#define MM_MATRIX_H

struct mm_matrix_stc
{
	int dim;
	int i;
	int j;
	double val;
};

typedef struct mm_matrix_stc MM_MAT;

struct mm_mult_stc
{
	int dim;
	int A_i;
	int A_j;
	int B_i;
	int B_j;
	int R_i;
	int R_j;
};

typedef struct mm_mult_stc MM_MUL;

struct mm_add_stc
{
	int dim;
	int A_i;
	int A_j;
	int B_i;
	int B_j;
	int R_i;
	int R_j;
	double val;
};

typedef struct mm_add_stc MM_ADD;

struct mm_ctrl_stc
{
	int dim;
	int i;
	int j;
};

typedef struct mm_ctrl_stc MM_CTRL;

struct mm_result_stc
{
	int dim;
	int R_i;
	int R_j;
	double val;
};

typedef struct mm_result_stc MM_RES;

#endif
	
	
	
