#include <iostream>
#include <sys/stat.h>
#include "parser.h"
#include <boost/filesystem.hpp>
#include <omp.h>

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
    struct stat buffer;   
    if ((stat (( argv[1] ), &buffer) == 0)){

            /* stringstream cmd; */
            /* cmd<<SCRIPT<<string( argv[1] )<<">"<<string( argv[1] )<<".csv"; */
            /* system(cmd.str().c_str()); */
            /* Parser* parser = new Parser(string(argv[1])); */
            /* parser->config(argv+2,3); */
            /* parser->start(); */

            /* return 0; */


            stringstream cmd;

            /* create a folder to save the tmp files. */
            boost::filesystem::path dir("./psv_tmp");
            if(boost::filesystem::exists(dir))
            {
                    boost::filesystem::remove_all(dir);
            }
            if(boost::filesystem::create_directory(dir)) {
                            /* cerr << "create_directory Success" << "\n"; */
            }


            boost::filesystem::path data_path(argv[1]);
            cmd<<"editcap -c 500000 "<<data_path.string()<<" ./psv_tmp/"<<data_path.filename();
            system(cmd.str().c_str());

            boost::filesystem::recursive_directory_iterator it(dir);
            boost::filesystem::recursive_directory_iterator endit;

            /* Create a container to hold the splited files; */
            vector<string> pcap_files;
            while(it != endit)
            {
                    if(boost::filesystem::is_regular_file(*it) && it->path().extension() == ".pcap"){
                            pcap_files.push_back(it->path().string());
                    }
                    ++it;
            }
            
            omp_set_num_threads(8);   
#pragma omp parallel
            {
#pragma omp for
                    for(uint16_t i = 0;i<pcap_files.size();i++){
                            string file = pcap_files[i];
                            D("Parsing "<<file);
                            stringstream cmd1;
                            cmd1<<SCRIPT<<file<<">"<<file<<".csv";
                            D("Command to dump csv "<<cmd1.str());
                            system(cmd1.str().c_str());
                            Parser* parser = new Parser(string(file));
                            parser->config(argv+2,3);
                            parser->start();
                    }
            }
            boost::filesystem::remove_all(dir);  

    }else{
            cerr<<"**ERROR: No file "<<argv[1]<<" exsist"<<endl;
    }
    return 0;
}

