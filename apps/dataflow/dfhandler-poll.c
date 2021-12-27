#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#define DEBUG


#include "woofc.h"
#include "df.h"

int dfhandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	DFOPERAND *operand = (DFOPERAND *)ptr;
	char *prog = operand->prog_woof;
	DFNODE node;
	unsigned long seqno;
	unsigned long c_seqno;
	unsigned long d_seqno;
	int err;
	double result;
	DFOPERAND result_operand;
	DFNODE claim_node;
	int wait_complete;
	unsigned long w_seqno;

#ifdef DEBUG
printf("dfhandler: prog: %s, dst_no %d, value: %f\n",
	prog, operand->dst_no, operand->value);
#endif  

	claim_node.state = CLAIM;
	claim_node.node_no = operand->dst_no;

	/*
	 * write a claim at the end of the prog woof
	 */
	c_seqno = WooFPut(prog,NULL,&claim_node);
	if(WooFInvalid(c_seqno)) {
		fprintf(stderr,
		"ERROR -- dfhandler: claim seqno failed from %s\n",
		prog);
		return(1);
	}
#ifdef DEBUG
printf("CLAIM prog: %s, node_no: %d dst_no %d\n",
	prog, node.node_no, operand->dst_no);
#endif  
		
	

	/*
	 * find the node that this operand matches by dst address
	 */
	seqno = WooFGetLatestSeqno(prog);
	if(WooFInvalid(seqno)) {
		fprintf(stderr,
		"ERROR -- dfhandler: latest seqno failed from %s\n",
		prog);
		return(1);
	}


	/*
	 * scan the program from the log tail
	 */
	while(!WooFInvalid(seqno)) {
		/*
		 * skip my own claim
		 */
		if(seqno == c_seqno) {
			seqno--;
			continue;
		}
		err = WooFGet(prog,&node,seqno);
		if(err < 0) {
			fprintf(stderr,
			"ERROR -- dfhandler: get of %s failed at %lu\n",
			prog,seqno);
			return(1);
		}
#ifdef DEBUG
printf("scanning: prog: %s, node_no: %d dst_no %d state: %d\n",
	prog, node.node_no, operand->dst_no, node.state);
#endif  

		/*
		 * skip done records
		 */
		if(node.state == DONE) {
			seqno--;
			continue;
		}

		/*
		 * is this the operation?
		 */
		if(node.node_no == operand->dst_no) {
#ifdef DEBUG
printf("found: prog: %s, node_no: %d dst_no %d state: %d\n",
	prog, node.node_no, operand->dst_no, node.state);
#endif  

			/*
			 * if this is claimed, and my claim seqno is bigger, I spin wait
			 */
			if((node.state == CLAIM) &&
			   (c_seqno > seqno)) {
#ifdef DEBUG
printf("waiting: prog: %s, node_no: %d dst_no %d\n",
	prog, node.node_no, operand->dst_no);
#endif  
				wait_complete = 0;
				while(1) {
					w_seqno = WooFGetLatestSeqno(prog);
					if(WooFInvalid(w_seqno)) {
						fprintf(stderr,
						"ERROR -- dfhandler: waiting get of %s failed at %lu\n",
						prog,w_seqno);
						return(1);
					}
					while(w_seqno >= seqno) {
						err = WooFGet(prog,&node,w_seqno);
						if(err < 0) {
							fprintf(stderr,
					"ERROR -- dfhandler: get of %s failed at %lu\n",
							prog,seqno);
							return(1);
						}
						if((node.node_no == operand->dst_no) &&
						   (node.state == DONE)) {
							wait_complete = 1;
							break;
						}
						w_seqno--;
					}
					if(wait_complete == 1) {
						break;
					}
				}
				/*
				 * here we were waiting, w_seqno is the DONE
				 * record, the waiting record should be above it in the
				 * log.  start here and continue
				 */
#ifdef DEBUG
printf("waitingcomplete: prog: %s, node_no: %d dst_no %d, %lu\n",
	prog, node.node_no, operand->dst_no, w_seqno);
#endif  
				seqno = w_seqno;
				continue;
			}

			if(node.state == CLAIM) {
				seqno--;
				continue;
			}
						
			/*
			 * if the ready_count is 2, the node has fired
			 */
			if(node.ready_count >= 2) {
				fprintf(stderr,
			"ERROR -- dfhandler trying to refire from %s\n",
				prog);
#ifdef DEBUG
printf("done: prog: %s, node_no: %d dst_no %d\n",
	prog, node.node_no, operand->dst_no);
#endif  
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
#ifdef DEBUG
printf("first arrived: prog: %s, node_no: %d dst_no %d value%f\n",
	prog, node.node_no, operand->dst_no, operand->value);
#endif  
				/*
				 * write a DONE record to match the claim
				 */
				claim_node.state = DONE;
				d_seqno = WooFPut(prog,NULL,&claim_node);
				if(WooFInvalid(d_seqno)) {
					fprintf(stderr,
				"ERROR -- could not DONE to %s\n",
					prog);
					return(1);
				}
#ifdef DEBUG
printf("DONE first arrived: prog: %s, node_no: %d dst_no %d, d_seqno: %lu\n",
	prog, node.node_no, operand->dst_no, d_seqno);
#endif  
			
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
				
#ifdef DEBUG
printf("second arrived retiring: prog: %s, node_no: %d dst_no %d value %f first val: %f result: %f state: %d\n",
	prog, node.node_no, operand->dst_no, operand->value, node.value,result,node.state);
#endif  
			/*
			 * retire this node
			 */
			node.ready_count = 2;
			d_seqno = WooFPut(prog,NULL,&node);
			if(WooFInvalid(d_seqno)) {
				fprintf(stderr,
			"ERROR -- dfhandler could not retire in %s\n",
				prog);
				return(1);
			}
			/*
			 * write a DONE record to match the claim
			 */
			claim_node.state = DONE;
			d_seqno = WooFPut(prog,NULL,&claim_node);
			if(WooFInvalid(d_seqno)) {
				fprintf(stderr,
				"ERROR -- could not DONE retire to %s\n",
					prog);
					return(1);
			}
#ifdef DEBUG
printf("DONE second arrived: prog: %s, node_no: %d dst_no %d\n",
	prog, node.node_no, operand->dst_no);
#endif  
			
			/*
			 * transmit result to next node
			 */

			memset(&result_operand,0,sizeof(result_operand));
			result_operand.value = result;
			result_operand.dst_no = node.dst_no;
			result_operand.order = node.dst_order;
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
#ifdef DEBUG
printf("second arrived fired: prog: %s, node_no: %d dst_no %d value%f\n",
	prog, node.node_no, operand->dst_no, operand->value);
#endif  
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

			
