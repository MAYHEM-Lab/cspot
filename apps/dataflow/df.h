#ifndef DF_H
#define DF_H

/* opcodes */

#define ADD (1)
#define SUB (2)
#define MUL (3)
#define DIV (4)
#define SQR (5)

struct df_node_stc
{
	int opcode; 	/* this op code */
	int ready_count /* how many have we received */
	double value;	/* value of first operand to arrive */
	int order;	/* is waiting first or second in non-commute */
	int node_no;	/* which node is it */
	int dst_opcode; /* next node type */
	int dst_no; 	/* next node address */
	int dst_order;	/* for non-commutative operations */
};

typedef struct df_node_stc DFNODE;

struct df_operand_stc
{
	double value;		/* operand value */
	int dst_no;		/* dest node address */
	int order;		/* for non-commute */
	char prog_woof[1024];	/* name of woof holding the program */
};

typedef struct df_operand_stc DFOPERAND;

extern double DFOperation(int opcode, double op1, double op2);

#endif
	
	
