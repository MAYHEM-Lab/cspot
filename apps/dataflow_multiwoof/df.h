#ifndef DF_H
#define DF_H

#include <cmath>

// opcodes

#define OPCODES \
    OP(OPERAND) \
    OP(ADD)     \
    OP(SUB)     \
    OP(MUL)     \
    OP(DIV)     \
    OP(SQR)     \
    OP(LT)      \
    OP(GT)      \
    OP(EQ)      \
    OP(ABS)     \
    OP(NOT)     \
    OP(SEL)     \
    OP(FILTER)  \
    OP(OFFSET)  \
    OP(KNN)     \
    OP(LINREG)

enum Opcode {
#define OP(name) name,
  OPCODES
#undef OP
    OPCODES_N
};

static const char* OPCODE_STR[OPCODES_N] = {
#define OP(name) [name] = #name,
    OPCODES
#undef OP
};

struct operand {
    double value;
    unsigned long seq;

    operand(double value=0.0, unsigned long seq=1) : value(value), seq(seq) {}
};

struct subscriber {
    int ns;  // namespace
    int id;
    int port;

    subscriber(int dst_ns = 0, int dst_id = 0, int dst_port = 0)
        : ns(dst_ns), id(dst_id), port(dst_port) {}

    bool operator<(const subscriber& other) const { return id < other.id; }
};

struct subscription {
    int ns;
    int id;
    int port;

    subscription(int src_ns = 0, int src_id = 0, int dst_port = 0)
        : ns(src_ns), id(src_id), port(dst_port) {}

    bool operator<(const subscription& other) const {
        return port < other.port;
    }
};

struct subscription_event {
    int ns;
    int id;
    int port;
    unsigned long seq;

    subscription_event(int ns=0, int id=0, int port=0, unsigned long seqno=0)
        : ns(ns), id(id), port(port), seq(seqno) {}
};

struct node {
    int id;
    int opcode;

    node(int id=0, int opcode=0) : id(id), opcode(opcode) {}

    bool operator<(const node& other) const {
        return id < other.id;
    }
};

// Struct to hold each data point. Includes coordinates (x, y) and class (label)
struct Point {
  double x;
  double y;
  int label;
  
  Point(double x = 0.0, double y = 0.0, int label = -1) : x(x), y(y), label(label) {}
};

// Struct to hold / perform online linear regression
struct Regression {
    double num;
    double x;
    double y;
    double xx;
    double xy;
    double slope;
    double intercept;
    
    Regression(double x = 0, double y = 0, double xx = 0, double xy = 0,
               double slope = 0, double intercept = 0)
        : x(x), y(y), xx(xx), xy(xy), slope(slope), intercept(intercept) {
        num = 0.0;        
    }
    
    void update(double new_x, double new_y) {
        double dt = 1e-2;   // Time step (delta t)
        double T = 5e-2;    // Time constant
        double decay_factor = exp(-dt / T);
        
        // Decay values
        num *= decay_factor;
        x *= decay_factor;
        y *= decay_factor;
        xx *= decay_factor;
        xy *= decay_factor;
        
        // Add new datapoint
        num += 1;
        x += new_x;
        y += new_y;
        xx += new_x * new_x;
        xy += new_x * new_y;
        
        // Calculate determinant and new slope / intercept
        double det = num * xx - pow(x, 2);
        if (det > 1e-10) {
            intercept = (xx * y - xy * x) / det;
            slope = (xy * num - x * y) / det;
        }
    }
};


#endif
