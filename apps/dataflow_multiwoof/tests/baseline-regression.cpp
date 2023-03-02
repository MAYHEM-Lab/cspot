// Example program
#include <iostream>
#include <cmath>
#include <chrono>

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

int main() {
    int iters = 100;
    
    Regression r;
    
    for (int i = 0; i < iters; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        r.update(i * 1.1, i * 2.2);
        auto end = std::chrono::high_resolution_clock::now();
        // std::cout << "y = " << r.slope << "x + " << r.intercept << std::endl;

        std::cout << "start" << ": "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        start.time_since_epoch())
                        .count()
                << "ns" << std::endl;

        std::cout << "end" << ": "
                << std::chrono::duration_cast<std::chrono::nanoseconds>(
                        end.time_since_epoch())
                        .count()
                << "ns" << std::endl;
    }
}