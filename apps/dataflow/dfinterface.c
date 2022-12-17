#include <math.h>
 #include <pthread.h>
 #include <semaphore.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/time.h>
 #include <time.h>
 #include <unistd.h>

 #include "dfinterface.h"

 void init(char *woof, int size) {
   int err;
   char op_woof[4096] = "";
   char prog_woof[4096] = "";

   // [WOOF].dfprogram
   strncpy(prog_woof, woof, sizeof(prog_woof));
   strncat(prog_woof, ".dfprogram", sizeof(prog_woof) - strlen(prog_woof) - 1);

   // [WOOF].dfoperand
   strncpy(op_woof, woof, sizeof(op_woof));
   strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

   WooFInit(); /* attach local namespace for create*/

   err = WooFCreate(prog_woof, sizeof(DFNODE), size);
   if (err < 0) {
     fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
     exit(1);
   }

   err = WooFCreate(op_woof, sizeof(DFOPERAND), size);
   if (err < 0) {
     fprintf(stderr, "ERROR -- create of %s failed\n", prog_woof);
     exit(1);
   }
 }

 void add_operand(char *woof, double val, int dst_id, int dst_port_id) {
   unsigned long seqno;
   char op_woof[1024] = "";

   DFOPERAND operand = {
       .value = val,
       .dst_no = dst_id,
       .dst_port = dst_port_id,
       .prog_woof = "",
   };

   // [WOOF].dfprogram
   strncpy(operand.prog_woof, woof, sizeof(operand.prog_woof));
   strncat(operand.prog_woof, ".dfprogram",
           sizeof(operand.prog_woof) - strlen(operand.prog_woof) - 1);

   // [WOOF].dfoperand
   strncpy(op_woof, woof, sizeof(op_woof));
   strncat(op_woof, ".dfoperand", sizeof(op_woof) - strlen(op_woof) - 1);

   WooFInit(); /* local only for now */

   seqno = WooFPut(op_woof, "dfhandler", &operand);

   if (WooFInvalid(seqno)) {
     fprintf(stderr, "ERROR -- put to %s of operand with dest %d failed\n",
             op_woof, operand.dst_no);
     exit(1);
   }
 }

 void add_node(char *woof, int id, int op, int dst_id, int dst_port_id, int n) {
  unsigned long seqno;
  char prog_woof[4096] = "";
  
  DFNODE node = {
    .node_no = id,
    .opcode = op,
    .total_val_ct = n,
    .recvd_val_ct = 0,
    .ip_value = 0.0,
    .ip_port = -1,
    .values = (double*)malloc(sizeof(double)*n),
    .dst_no = dst_id, 
    .dst_port = dst_port_id,
    .state = WAITING
  };

   // [WOOF].dfprogram
   strncpy(prog_woof, woof, sizeof(prog_woof));
   strncat(prog_woof, ".dfprogram", sizeof(prog_woof) - strlen(prog_woof) - 1);

   WooFInit(); /* local only for now */

   seqno = WooFPut(prog_woof, NULL, &node);

   if (WooFInvalid(seqno)) {
     fprintf(stderr, "ERROR -- put to %s of node %d, opcode %d failed\n",
             prog_woof, node.node_no, node.opcode);
     exit(1);
   }
 }