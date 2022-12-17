#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <stdbool.h>

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
	node->state = DONE;
	d_seqno = WooFPut(prog,NULL,node);
	if(WooFInvalid(d_seqno)) {
		fprintf(stderr,
	"ERROR -- dfhandler could not retire in %s\n",
		prog);
		return(1);
	}
#ifdef DEBUG
printf("DEBUG: DONE prog: %s, node_no: %d result : %f dst_no: %d dst_port: %d\n", prog, node->node_no, result, node->dst_no, node->dst_port);
#endif  
	/*
	 * transmit result to next node
	 */
	memset(&result_operand,0,sizeof(result_operand));
	result_operand.value = result;
	result_operand.dst_no = node->dst_no;
	result_operand.dst_port = node->dst_port;
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
printf("DEBUG: OPERAND put: prog: %s node_no: %d dst_no %d dst_port %d result %f\n",
	prog, node->node_no, node->dst_no, node->dst_port, result);
#endif  
	return(0);
}

int dfhandler(WOOF *wf, unsigned long wf_seq_no, void *ptr)
{
	DFOPERAND *operand = (DFOPERAND *)ptr;
	char *prog = operand->prog_woof;
	DFNODE node;
	DFNODE fnode;
	DFNODE claim_node;
	unsigned long seqno;
	unsigned long c_seqno;
	unsigned long d_seqno;
	unsigned long f_seqno;
	unsigned long p_seqno;
	int err;
	double result;

#ifdef DEBUG
printf("DEBUG: DFHANDLER: prog: %s, dst_no %d, dst_port %d, value: %f\n",
	prog, operand->dst_no, operand->dst_port, operand->value);
#endif  

	/*
	 * create a CLAIM node to mark the log with the arrival of the
	 * operand
	 */
	memset(&claim_node,0,sizeof(claim_node));
	claim_node.state = CLAIM;
	claim_node.node_no = operand->dst_no;
	claim_node.recvd_val_ct = 1;
	claim_node.values = NULL;
	claim_node.ip_value = operand->value;
	claim_node.ip_port = operand->dst_port;

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
printf("DEBUG: CLAIM CREATED prog: %s, node_no: %d ip_val %f ip_port %d \n",
	prog, claim_node.node_no, claim_node.ip_value, claim_node.ip_port);
#endif  

	/*
	 * c_seqno is the sequence number in the log of the CLAIM
	 * for this handler.  For this handler, this defines the tail
	 * of the log.  Search backwards from this point (SCAN 1)
	 *
	 * if we find another CLAIM or PARTIAL with an earlier sequence number and
	 * and the same dst_no, but we don't find a WAITING, then there is
	 * another handler in progress that will find this CLAIM and fire
	 * the node and this handler should just exit (SCAN 1 exit case)
	 *
	 * if we find a WAITING for this dst_no, then we fire the node (SCAN 1 fire case)
	 *
	 * then we create a partial and we then check to make sure another claim did not come
	 * in before we commited the partial to the log (SCAN 2)
	 *
	 * if we find CLAIMS in Scan 2 we add their PARTIALS and we start SCAN 2B from the last PARTIAL
	 * backwards till the PREVIOUS PARTIAL (SCAN 2B)
	 * SCAN 2B is repeated until no other CLAIM is found.
	 *
	 * now the handler checks the CLAIMS which are AFTER the latest PARTIAL, until the end of the WOOF
	 * it fires PARTIAL for all CLAIMS found. (SCAN 3)
	 * 
	 * After scan 3 the handler appends the PARTIAL_DONE node, so that a new handler can start firing the coming CLAIMS.
	 *
	 * After every claim is processed the number of input elements are checked
	 * if it is n then the node is fired and DONE. 
	 *
	 * main log scan loop (SCAN 1)
	 *
	 * must start with the seqno for my CLAIM since it will be a future
	 * if another handler completes a partial afterwards.
	 */
	d_seqno = -1;
	seqno = c_seqno - 1;
	while(!WooFInvalid(seqno)) {
		err = WooFGet(prog,&node,seqno);
		if(err < 0) {
			fprintf(stderr,
			"ERROR -- dfhandler: get of %s failed at %lu\n",
			prog,seqno);
			return(1);
		}
		/*
		 * if we find a CLAIM or PARTIAL node with a lower sequence number,
		 * then exit as that CLAIM or PARTIAL is in progress and will pick
		 * up this CLAIM as a future
		 */
		if((node.node_no == claim_node.node_no) &&
		   (node.state == CLAIM || node.state == PARTIAL) && 
		   (seqno < c_seqno)) {
#ifdef DEBUG
printf("DEBUG: SCAN 1 EXIT FOUND CLAIM(1)/PARTIAL(2): %d, prog: %s, node_no: %d ip_val: %f ip_port %d seq_no %ld\n",
	node.state, prog, node.node_no, claim_node.ip_value, claim_node.ip_port, seqno);
#endif  
			return(0);
		}

#ifdef DEBUG
printf("DEBUG : claim_node_no : %d, claim_node_port : %d claim_node_state %d curr_node_no: %d curr_node_state: %d curr_node_seq_no: %ld\n",
	claim_node.node_no, claim_node.ip_port, claim_node.state, node.node_no, node.state, seqno);
#endif
		/*
		 * we didn't see a younger CLAIM/PARTIAL before,  
		 * we will encounter WAITING    
		 * we take info from that node, add the new found info
		 */
		if((node.node_no == claim_node.node_no) && 
			(node.state == WAITING) && 
			(seqno < c_seqno)) {

#ifdef DEBUG
printf("DEBUG: SCAN 1 CREATE PARTIAL prog: %s, node_no: %d ip_port %d ip_val %f recvd %d total %d\n",
	prog, node.node_no, claim_node.ip_port, claim_node.ip_value, node.recvd_val_ct, node.total_val_ct);
#endif 			
			node.recvd_val_ct += 1;
			node.values[claim_node.ip_port] = claim_node.ip_value;

			// if all inputs are recieved fire the node. 
			if(node.recvd_val_ct == node.total_val_ct) {   
				result = DFOperation(node.opcode,
						node.values,
						node.total_val_ct);
#ifdef DEBUG
printf("DEBUG: SCAN 1 ALL INPUTS RECIEVED: prog: %s, node_no: %d result: %f\n",
	prog, node.node_no, result);
#endif  
				err = dfFireNode(wf,prog,&node,result);
				return(err);
			}
			
			/*
			 * if not all inputs are recieved then create partial and 
			 * start another check backwards from that partial to this claim
			 * to search for other claims in the history.  
			*/
			node.state = PARTIAL;
			d_seqno = WooFPut(prog,NULL,&node);
			if(WooFInvalid(d_seqno)) {
				fprintf(stderr,
			"dfhandler: failed to put partial to %s\n",prog);
				return(1);
			}
#ifdef DEBUG
printf("DEBUG: SCAN 1 CREATE PARTIAL prog: %s, node_no: %d ip_port %d ip_val %f recvd %d total %d\n",
	prog, node.node_no, claim_node.ip_port, claim_node.ip_value, node.recvd_val_ct, node.total_val_ct);
#endif  
			// done with SCAN 1 when we process one WAITING node
			break;
		}
		//log scan continue
		seqno--;
	}

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
	* 
	* start a while loop for checking repeated CLAIMS until
	* either no CLAIM is found
	* 
	* the outer while loop is for part 2B i.e new PARTIALS
	* arising out of this handler, have to keep backward checking
	* repeatedly, until no backward CLAIM is left. for each case, 
	* loop starts from newest partial to partial which caused it.
	*/

	// to check if any new partial is fired; i.e new backward CLAIM was found
	p_seqno = -1;
	while(!WooFInvalid(d_seqno)) {
		p_seqno = d_seqno;
		f_seqno = d_seqno - 1;
		d_seqno = -1;
		/*
		* walk back from the partial to the CLAIM (SCAN 2A) just once
		* walk back form new partial to old partial (SCAN 2B) until no backward claim
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
			* younger than the partial?  If so, fire or partial fire it
			*/
			if((fnode.node_no == node.node_no) &&
			(fnode.state == CLAIM)) {
#ifdef DEBUG
printf("DEBUG: SCAN 2 found CLAIM prog: %s, node_no: %d ip_val %f ip_port %d\n",
prog,node.node_no, fnode.ip_value, fnode.ip_port);
#endif
				node.recvd_val_ct += 1;
				node.values[fnode.dst_port] = fnode.ip_value;

				/*
				* if all inputs are recieved then fire the node. 
				*/
				if(node.recvd_val_ct == node.total_val_ct) {
					result = DFOperation(node.opcode,
						node.values,
						node.total_val_ct);
#ifdef DEBUG
printf("DEBUG: SCAN 2 ALL INPUTS RECIEVED: prog: %s, node_no: %d result: %f\n",
prog, node.node_no, result);
#endif 
					err = dfFireNode(wf,prog,&node,result);
					return(err);
				}

				/* 
				* create partial if all inputs are not recieved 
				* does one more round of checking as not all inputs are recieved. 
				* this is done by the updated d_seqno 
				*/
				d_seqno = WooFPut(prog,NULL,&node);
				if(WooFInvalid(d_seqno)) {
					fprintf(stderr,
				"dfhandler: failed to put partial to %s\n",prog);
					return(1);
				}
#ifdef DEBUG
printf("DEBUG: SCAN 2 CREATE PARTIAL prog: %s, node_no: %d ip_port %d ip_val %f recvd %d total %d\n",
prog, node.node_no, fnode.ip_port, fnode.ip_value, node.recvd_val_ct, node.total_val_ct);
#endif  			
			}
			f_seqno--; /* continue future scan */
		}
		// reduce the scan space by limiting it to the first PARTIAL as the lowest node
		c_seqno = p_seqno;
	}

	/*
		* all claims are in the backlog are processed now
		* check for new claims till the end of the list.
		* start from 'the checking PARTIAL' till end of list. 
		* add a WAITING at the end of the list. (SCAN 3)
	*/
	f_seqno = p_seqno + 1;
	while(!WooFInvalid(p_seqno) && f_seqno <= WooFGetLatestSeqno(prog)) {
		err = WooFGet(prog,&fnode,f_seqno);
		if(err < 0) {
			fprintf(stderr,
			"dfhandler ERROR -- couldn't get %lu from %s on future\n",
				f_seqno,prog);
			return(1);
		}

		/*
		* if CLAIM is found, create a partial
		* if PARTIAL is found, it must be created by 
		* the claims found in this loop
		* so ignore them 
		*/
		if((fnode.node_no == node.node_no) &&
			(fnode.state == CLAIM)) {
#ifdef DEBUG
printf("DEBUG: SCAN 3 found CLAIM prog: %s, node_no: %d ip_val %f ip_port %d\n",
prog,node.node_no, fnode.ip_value, fnode.ip_port);
#endif
			node.recvd_val_ct += 1;
			node.values[fnode.dst_port] = fnode.ip_value;

			/*
			* if all inputs are recieved then fire the node. 
			*/
			if(node.recvd_val_ct == node.total_val_ct) {
				result = DFOperation(node.opcode,
					node.values,
					node.total_val_ct);
#ifdef DEBUG
printf("DEBUG: SCAN 3 ALL INPUTS RECIEVED: prog: %s, node_no: %d result: %f\n",
prog, node.node_no, result);
#endif 
				err = dfFireNode(wf,prog,&node,result);
				return(err);
			}

			/* 
			* create partial if all inputs are not recieved 
			*/
			d_seqno = WooFPut(prog,NULL,&node);
			if(WooFInvalid(d_seqno)) {
				fprintf(stderr,
			"dfhandler: failed to put partial to %s\n",prog);
				return(1);
			}
#ifdef DEBUG
printf("DEBUG: SCAN 3 CREATE PARTIAL prog: %s, node_no: %d ip_port %d ip_val %f recvd %d total %d\n",
prog, node.node_no, fnode.ip_port, fnode.ip_value, node.recvd_val_ct, node.total_val_ct);
#endif  
		}
		f_seqno++;
	}

	/*
	* end of partial creation by all possible ways on top
	* need better solution here as there might be a (very rare case)
	* where a CLAIM might be added in between the loop ending and this woofput
	*/
	node.state = WAITING;
	d_seqno = WooFPut(prog,NULL,&node);
	if(WooFInvalid(d_seqno)) {
		fprintf(stderr,
	"dfhandler: failed to put partial to %s\n",prog);
		return(1);
	}
#ifdef DEBUG
printf("DEBUG: SCAN 3 CREATE WAITING prog: %s, node_no: %d recvd %d total %d\n",
prog, node.node_no, node.recvd_val_ct, node.total_val_ct);
#endif  
	/* 
	* one running partial finishes every possible claim in its lifetime 
	* adds a PARTIAL_DONE 
	*/
	return(0);
}