#ifndef DFINTERFACE_H
#define DFINTERFACE_H

#include "df.h"
#include "woofc.h"

/**
 * Initializes a woof.
 *
 * Must be called before invoking other functions.
 *
 * @param woof Name of woof to be created.
 * @param size Number of entries in woof.
 */
void init(char* woof, int size);

/**
 * Add an operand.
 *
 * An operand holds a constant value that is passed to a node.
 *
 * @param woof Name of woof in which the operand will exist.
 * @param val Value of the operand.
 * @param dst_id ID of destination node.
 * @param dst_port_id ID of destination port.
 */
void add_operand(char* woof, double val, int dst_id, int dst_port_id);

/**
 * Add a node.
 *
 * A node consumes values, performs some computation (determined by opcode), and
 * then passes the result to another node.
 *
 * @param woof Name of woof in which the node will exist.
 * @param id ID of the node (must be unique).
 * @param op Opcode, which specifies computation to be performed.
 * @param dst_id ID of destination node.
 * @param dst_port_id ID of destination port.
 * @param n arity of the node.
 */
void add_node(char* woof, int id, int op, int dst_id, int dst_port_id, int n);

#endif // DFINTERFACE_H