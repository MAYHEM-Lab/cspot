#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>


#include "woofc.h"
#include "df.h"

int dfhandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	DFOPERAND *operand = (DFOPERAND *)ptr;
	char *prog = operand->prog_woof;
	DFNODE node;
	unsigned long seqno;
	unsigned long d_seqno;
	int err;
	double result;
	DFOPERAND result_operand;
	

	/*
	 * find the node that this operand matches by dst address
	 */
	seqno = WooFGetLatestSeqno(prog);
	if(WooFInvalid(seq_no)) {
		fprintf(stderr,
		"ERROR -- dfhandler: latest seq_no failed from %s\n",
		prog);
		return(1);
	}

	/*
	 * scan the program from the log tail
	 */
	while(!WooFInvalid(seqno)) {
		err = WooFGet(prog,&node,seqno);
		if(err < 0) {
			fprintf(stderr,
			"ERROR -- dfhandler: get of %s failed at %lu\n",
			prog,seq_no);
			return(1);
		}
		/*
		 * is this the operation?
		 */
		if(node.node_no == operand->dst_no) {
			/*
			 * if the ready_count is 2, the node has fired
			 */
			if(node.ready_count >= 2) {
				fprintf(stderr,
			"ERROR -- dfhandler trying to refire from %s\n",
				prog);
				return(1);
			}
			/*
			 * if the ready_count in this record is 0
			 * append and wait for next operand
			 */
			if(node.ready_count == 0) {
				node.value = operand->value;
				node.ready_count = 1;
				node.order = operand->order;
				d_seqno = WooFPut(prog,NULL,&node);
				if(WooFInvalid(d_seqno)) {
					fprintf(stderr,
				"ERROR -- could not operand to %s\n",
					prog);
					return(1);
				}
				/*
				 * wait for the next operand
				 */
				return(0);
			}
			/*
			 * here, if this is the second operand, fire the
			 * node and retire the node by setting ready_count
			 * to 2
			 * note that order determines non-commute
			 * operand order
			 */

			if(node.order == 1) {
				result = DFOperation(node.opcode,
					   node.value,
					   operand->value);
			} else {
				result = DFOperation(node.opcode,
					   operand->value,
					   node.value);
			}
				
			/*
			 * retire this node
			 */
			node.ready_count = 2;
			d_seqno = WoofPut(prog,NULL,&node);
			if(WooFInvalid(d_seqno)) {
				fprintf(stderr,
			"ERROR -- dfhandler could not retire in %s\n",
				prog);
				return(1);
			}
			/*
			 * transmit result to next node
			 */

			memset(&result_operand,0,sizeof(result_operand));
			result_operand.value = result;
			result_operand.dst_no = node.dst_no;
			result_operand.dst_order = node.dst_order;
			strcpy(result_operand.prog_woof,prog);
			/*
			 * if the dst_no == 0, this is a final result.  Don't fire the
			 * handler in this case
			 */
			if(node.dst_no == 0) {
				seqno = WooFPut(WooFGetFileName(wf),
					NULL,
					&result_operand);
			} else {
				seqno = WooFPut(WooFGetFileName(wf),
					"dfhandler",
					&result_operand);
			}
			if(WooFInvalid(seqno)) {
				fprintf(stderr,
		"ERROR -- dfhandler could not forward result to %s\n",
					WooFGetFileName(wf));
				return(1);
			}
			/*
			 * node fired
			 */
			return(0);
		}
		seqno--; /* check next instruction */
	}

	/*
	 * here we did not find the node address in the program
	 */
	fprintf(stderr,
		"ERROR -- dfhandler could not find node %d in %s\n",
			operand->dst_no,
			prog);
	return(1);
}

			
