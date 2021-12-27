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

int dfFireNode(WOOF *wf, char *prog, DFNODE *node, double result)
{
	unsigned long d_seqno;
	DFOPERAND result_operand;
	/*
	 * retire this node
	 */
	node->ready_count = 2;
	node->state = DONE;
	d_seqno = WooFPut(prog,NULL,node);
	if(WooFInvalid(d_seqno)) {
		fprintf(stderr,
	"ERROR -- dfhandler could not retire in %s\n",
		prog);
		return(1);
	}
#ifdef DEBUG
printf("DONE prog: %s, node_no: %d\n", prog, node->node_no);
#endif  
	/*
	 * transmit result to next node
	 */
	memset(&result_operand,0,sizeof(result_operand));
	result_operand.value = result;
	result_operand.dst_no = node->dst_no;
	result_operand.order = node->dst_order;
	strcpy(result_operand.prog_woof,prog);
	/*
	 * if the dst_no == 0, this is a final result.  Don't fire the
	 * handler in this case
	 */
	if(node->dst_no == 0) {
		d_seqno = WooFPut(WooFGetFileName(wf),
			NULL,
			&result_operand);
	} else {
		d_seqno = WooFPut(WooFGetFileName(wf),
			"dfhandler",
			&result_operand);
	}
	if(WooFInvalid(d_seqno)) {
		fprintf(stderr,
			"ERROR -- dfhandler could not forward result to %s\n",
					WooFGetFileName(wf));
		return(1);
	}
#ifdef DEBUG
printf("result operand put: prog: %s, node_no: %d dst_no %d result %f\n",
	prog, node->node_no, node->dst_no, result);
#endif  
	return(0);
}

int dfhandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	DFOPERAND *operand = (DFOPERAND *)ptr;
	char *prog = operand->prog_woof;
	DFNODE node;
	DFNODE fnode;
	unsigned long seqno;
	unsigned long c_seqno;
	unsigned long d_seqno;
	unsigned long f_seqno;
	int err;
	double result;
	DFNODE claim_node;

#ifdef DEBUG
printf("dfhandler: prog: %s, dst_no %d, value: %f\n",
	prog, operand->dst_no, operand->value);
#endif  

	/*
	 * create a CLAIM node to mark the log with the arrival of the
	 * operand
	 */
	memset(&claim_node,0,sizeof(claim_node));
	claim_node.state = CLAIM;
	claim_node.node_no = operand->dst_no;
	claim_node.value = operand->value;
	claim_node.ready_count = 0;
	claim_node.order = operand->order;

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
printf("CLAIM prog: %s, node_no: %d dst_no %d value: %f\n",
	prog, claim_node.node_no, operand->dst_no, operand->value);
#endif  

	/*
	 * c_seqno is the sequence number in the log of the CLAIM
	 * for this handler.  For this handler, this defines the tail
	 * of the log.  Search backwards from this point
	 *
	 * if we find another CLAIM with an earlier sequence number and
	 * and the same dst_no, but we don't find a partial, then there is
	 * another handler in progress that will find this CLAIM and fire
	 * the node after its partial is done and this handler should
	 * just exit
	 *
	 * if we find a partial for this dst_no, then we fire the node
	 *
	 * if we find neither another CLAIM nor a partial, then we create
	 * a partial and we then check to make sure another claim did not come
	 * in before we commited the partial to the log
	 *
	 * a CLAIM that comes in after the partial is committed will
	 * find it and complete it
	 */
		
	

	/*
	 * main log scan loop
	 *
	 * must start with the seqno for my CLAIM since it will be a future
	 * if another handler completes a partial afterwards.
	 */
	seqno = c_seqno - 1;
	while(!WooFInvalid(seqno)) {
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
		 * sanity check.  There should never be a node with a
		 * ready_count of 2 that has this dst_no since all
		 * nodes take two inputs
		 */
		if((node.node_no == operand->dst_no) &&
		   (node.ready_count >= 2)) {
			fprintf(stderr,
			"dfhandler ERROR: more than two inputs ");
			fprintf(stderr,
				"prog %s, node: %d dst_no: %d state: %d, ",
			  prog, node.node_no, operand->dst_no, node.state);
			fprintf(stderr,
				"ready_count: %d\n",node.ready_count);
			fprintf(stderr,"dfhandler ERROR exiting\n");
			exit(1);
		}

		/*
		 * is this a partial of the operation?
		 * if so, fire the node since we didn't see a CLAIM
		 */
		if((node.node_no == operand->dst_no) &&
		   (node.ready_count == 1)) {
#ifdef DEBUG
printf("firing partial: prog: %s, node_no: %d dst_no %d state: %d\n",
	prog, node.node_no, operand->dst_no, node.state);
#endif  
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
			err = dfFireNode(wf,prog,&node,result);
			return(err);
		}

		/*
		 * if we find a CLAIM node with a lower sequence number,
		 * then exit as that CLAIM is in progress and will pick
		 * up this CLAIM as a future
		 */
		if((node.node_no == operand->dst_no) &&
		   (node.state == CLAIM) && 
		   (seqno < c_seqno)) {
#ifdef DEBUG
printf("CLAIM found younger CLAIM prog: %s, node_no: %d dst_no %d\n",
	prog, node.node_no, operand->dst_no);
#endif  
			return(0);
		}

		/*
		 * if we find the node with a ready_count of 0 and
		 * we didn't see a younger CLAIM, make a partial
		 * and exit
		 */
		if((node.node_no == operand->dst_no) &&
		   (node.state != CLAIM) && /* CLAIMs have rc - 0 */
		   (node.ready_count == 0)) {
			node.ready_count = 1;
			node.value = operand->value;
			node.order = operand->order;
			d_seqno = WooFPut(prog,NULL,&node);
			if(WooFInvalid(d_seqno)) {
				fprintf(stderr,
			"dfhandler: failed to put partial to %s\n",prog);
				return(1);
			}
#ifdef DEBUG
printf("PARTIAL created prog: %s, node_no: %d dst_no %d\n",
	prog, node.node_no, operand->dst_no);
#endif  
			/*
			 * now search for a future that may have come
			 * in while we were creating the partial
			 *
			 * note that the future will be represented by
			 * a CLAIM that is younger than this partial, but
			 * older than the CLAIM for this handler.  The partial
			 * seqno is d_seqno and the CLAIM for this handler
			 * is c_seqno
			 *
			 * start at the seqno immediately before this partial
			 */
			f_seqno = d_seqno - 1;
			/*
			 * walk back from the partial to the CLAIM
			 * for this handler
			 */
			while(f_seqno > c_seqno) {
				err = WooFGet(prog,&fnode,f_seqno);
				if(err < 0) {
					fprintf(stderr,
		"dfhandler ERROR -- couldn't get %lu from %s on future\n",
						f_seqno,prog);
					return(1);
				}
				/*
				 * is this another claim for the node that is
				 * younger than the partial?  If so, fire
				 * it
				 */
				if((fnode.node_no == operand->dst_no) &&
				   (fnode.state == CLAIM)) {
#ifdef DEBUG
printf("future: found CLAIM for partial prog: %s, node_no: %d dst_no %d value%f\n",
	prog,node.node_no,operand->dst_no,fnode.value);
#endif
					if(fnode.order == 1) {
					  result = DFOperation(node.opcode,
							   fnode.value,
							   operand->value);
					} else {
				          result = DFOperation(node.opcode,
					   operand->value,
					   fnode.value);
					}
					err = dfFireNode(wf,prog,&node,result);
					/*
			 		 * end of future process 
					 * with future fired 
					 */
					return(err);
				}
				f_seqno--; /* continue future scan */
			}
			/*
			 * end of partial creation without successful
			 * future
			 */
			return(0);
		}
		seqno--; /* main log scan */
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

			
