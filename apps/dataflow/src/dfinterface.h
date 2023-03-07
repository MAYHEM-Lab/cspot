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
 * @param destination_node_id ID of destination node.
 * @param destination_node_port ID of destination port.
 * @param value Value of the operand.
 */
void add_operand(char* woof, int destination_node_id, int destination_node_port, DF_VALUE value);

/**
 * Add a node.
 *
 * A node consumes values, performs some computation (determined by operation), and
 * then passes the result to another node.
 *
 * @param woof Name of woof in which the node will exist.
 * @param node_id ID of the node (must be unique).
 * @param input_count arity of the node.
 * @param opcode Opcode, which specifies computation to be performed.
 * @param destination_node_id ID of destination node.
 * @param destination_node_port ID of destination port.
 */
void add_node(char* woof,
              int node_id,
              int input_count,
              DF_OPERATION operation,
              int destination_node_id,
              int destination_node_port);

#endif // DFINTERFACE_H