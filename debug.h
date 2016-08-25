#include <iostream>
#include <iomanip>
#define DEBUG

#ifdef DEBUG
#define D(msg) do {\
        std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") "<< \
       " at "<<__func__<<": "<< msg << std::endl;} while(0)
#else
#define D(msg)  
#endif
