#include <stdint.h>
#include "debug.h"
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <map>
#include <iomanip>
#include <assert.h>     

#define ADD_LEN 16
class Line_cont;
class Window_data;
class Parser
{
        public:
                Parser (std::string);
                /* virtual ~Parser (); */
                void config(char** ,uint8_t );
                void start();

        private:
                //All in millisecond
                std::string mName;
                double mStartTime;
                bool m_pass_offset;
                uint16_t mOffset;
                uint16_t mWindow;
                uint16_t mPeriod;
                std::vector<Window_data*> mWindow_datas;
                bool is_in_range(std::string);
                /* data */

};

class Line_cont
{
        public:
                enum Fields{
                        F_TIME = 0,
                        F_LEN = 1,
                        F_TA = 2,
                        F_RA = 3,
                        F_TYPE_SUBTYPE = 4,
                        F_DS = 5,
                        F_RETRY = 6,
                        F_RSSI = 7,
                        F_FREQ = 8,
                        F_RATE = 9,
                        F_NAV = 10,
                        F_AMPDU_KNOWN = 11,
                        F_AMPDU_LAST = 12,
                        F_AMPDU_REF = 13
                };

                Line_cont (std::string);
                void read_field(std::string,uint8_t);
                std::string get_field(Fields);
                void print_fields();
                double getTime();

        private:
                double mTime;
                std::map<Fields, std::string> mData;
                /* data */

};

class AP_stat;
class Window_data
{
        public:
                Window_data (double);

                void addData(Line_cont*);
                void parse();
                double getTime();
        private:
                std::vector<Line_cont*> mLines;
                std::map<std::string, AP_stat*> mAPs;
                double  mTime;
                void getAP_pool();
                void assignData();
                bool is_downlink(Line_cont*);
                bool is_uplink(Line_cont*);
                bool is_beacon(Line_cont*);

                /* data */

};


class AP_stat
{
public:
        AP_stat (std::string);
        // in millisecond
        float getAirTime();
        void addPacket(Line_cont*);
        uint32_t getPacketN();
        std::vector<Line_cont*>* getPkts();
        void calc_Airtime();

private:
        /* data */
        std::string mAddr;
        float mAirTime;
        std::vector<Line_cont*> mPackets;


};
