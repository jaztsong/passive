#include <iostream>
#include "parser.h"

using namespace std;
int main(int argc, char* argv[])
{
    // Check the number of parameters
    if (argc < 5) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " file_name"
                <<" offset"<<" window"
                " period"<< std::endl;
        return 1;
    }
    //call the script
    stringstream cmd;
    cmd<<SCRIPT<<string( argv[1] )<<">"<<string( argv[1] )<<".csv";
    system(cmd.str().c_str());
    Parser* parser = new Parser(string(argv[1]));
    parser->config(argv+2,3);
    parser->start();

    return 0;
}

