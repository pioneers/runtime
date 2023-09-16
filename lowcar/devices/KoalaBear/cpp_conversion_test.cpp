// Online C++ compiler to run C++ program online
#include <iostream>
#include <string>
#define FIXED_POINT_FRACTIONAL_BITS 5

typedef uint32_t fixed_point_t;


class PIDtest {
    public:
    inline double fixed_to_double(fixed_point_t input){
        return ((double)input / (double)(1 << FIXED_POINT_FRACTIONAL_BITS));
    }
    inline fixed_point_t double_to_fixed(double input){
        return (fixed_point_t)(input * (1 << FIXED_POINT_FRACTIONAL_BITS));
    }
};


int main() {
    // Write C++ code here
    PIDtest c;
    fixed_point_t output = c.fixed_to_double(100);
    std::cout << output << std::endl;
    double d = c.double_to_fixed(output);
    std::cout << d;


    return 0;
}


