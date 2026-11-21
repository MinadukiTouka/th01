#include <stdint.h>
#include "th01/pc98/entry.hpp"

int main()
{
    return (int)(uintptr_t)&main_main;
}
