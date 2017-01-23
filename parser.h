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
#include <math.h> 
#include <algorithm>
#include <bitset>
#include <tuple>
#include <queue>

#define ADD_LEN 17
#define SCRIPT "/home/netscale/A-MPDU/src/Passive/scripts/parse_pcap.sh "
#define OVERHEAD_PREAMBLE_S 28
#define OVERHEAD_PREAMBLE_L 40
#define BACKOFF_CW 15
#define SLOT_TIME 9
//Let's put the weight to zero for now.
#define WEIGHT 0.0
#define BITMAP_LEN 64
#define REPORT_LEVEL 1 
// 0 is the basic version only outputs the must-have information: traditional way RC AMPDU_len AMPDU_N vs new way
//1 adds some other comparison components for experimental evaluation
//2 is the full version that output all
//
#define MTU 1500
#define ACK_LEN 120

class Line_cont;
class Window_data;
class Parser
{
        public:
                Parser (std::string);
                /* virtual ~Parser (); */
                void config(char** ,uint8_t );
                void start();
                uint16_t getOffset();
                uint16_t getWindowSize();
                uint16_t getPeriod();

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
                        F_TIME_DELTA = 1,
                        F_LEN = 2,
                        F_TA = 3,
                        F_RA = 4,
                        F_TYPE_SUBTYPE = 5,
                        F_DS = 6,
                        F_RETRY = 7,
                        F_RSSI = 8,
                        F_FREQ = 9,
                        F_RATE = 10,
                        F_NAV = 11,
                        F_SEQ = 12,
                        F_AMPDU_KNOWN = 13,
                        F_AMPDU_LAST = 14,
                        F_AMPDU_REF = 15,
                        F_BLKACK_SSN = 16,
                        F_BLKACK_BM = 17
                };

                Line_cont (std::string);
                void read_field(std::string,uint8_t);
                std::string get_field(Fields);
                void print_fields();
                double getTime();
                Line_cont* next_line;

        private:
                double mTime;
                std::map<Fields, std::string> mData;
                /* data */

};

class AP_stat;
class Window_data
{
        public:
                Window_data (double,Parser*);

                void addData(Line_cont*);
                void parse();
                double getTime();
                Parser* getParser();
                void setParser(Parser*);
        private:
                std::vector<Line_cont*> mLines;
                std::map<std::string, AP_stat*> mAPs;
                Parser* mParser;
                double  mTime;
                void getAP_pool();
                void assignData();
                bool is_downlink(Line_cont*);
                bool is_uplink(Line_cont*);
                bool is_beacon_or_PR(Line_cont*);

                /* data */

};


class BlkACK {
        public:
                BlkACK(Line_cont*);
                std::string addr;
                bool addr_rev;
                Line_cont* line;
                uint16_t SSN;
                int RSSI;
                std::vector<uint16_t> Miss;
};
class BlkACK_stat;
class AP_stat
{
public:
        AP_stat (std::string,Parser*);
        // in millisecond
        std::vector<Line_cont*>* getPkts();



        float getAirTime();
        void addPacket(Line_cont*);
        uint32_t getPacketN();
        void prepare_BAMPDU();
        void go_calc();
        void report();
        void setTime(double);
        void classifyPkts();

        Parser* getParser();
        void setParser(Parser*);
private:

        /* data */
        Parser* mParser;
        std::string mAddr;
        float mMBytes;
        double mTime;
        uint16_t m_nACK;
        uint16_t m_nBlockACK;
        uint16_t m_nBlockACKreq;
        uint16_t m_nMLframe;
        uint16_t mB_nAMPDU;
        float mB_AMPDU_len_mean;
        uint16_t m_nUPs;
        uint16_t m_nDowns;
        std::map<std::string,BlkACK_stat*> mBlkACKs;
        std::queue<Line_cont*> m_queue_blockACKreq;


        std::vector<Line_cont*> mPackets;
        std::vector<int> mBAMPDU_len;
        std::map<uint64_t, int> mRetrys;


        bool is_ACK(Line_cont*);
        bool is_blockACK(Line_cont*);
        bool is_blockACKreq(Line_cont*);
        bool is_data(Line_cont*);
        bool is_downlink(Line_cont*);
        bool is_beacon_or_PR(Line_cont*);
        void getBAMPDU_stats();
        BlkACK parse_blkack(Line_cont*);

};

class BlkACK_stat
{
        public:
                BlkACK_stat (std::string,AP_stat*);
                std::string getAddr();
                void addACK(BlkACK*);
                std::vector<std::tuple<uint16_t,uint16_t,int,float,bool> > mAMPDU_tuple;
                bool parse_AMPDU();
                float getAMPDU_mean();
                uint16_t getMPDU_num();
                void setPktSize(uint16_t);
                void calc_stats();

        private:
                std::string mAddr;
                AP_stat* mAP_stat;
                std::vector<BlkACK*> mACKs;
                std::vector<uint16_t> mLoss;
                int set_diff(std::vector<uint16_t>&,std::vector<uint16_t>&);
                float mRSSI_mean;
                float mAMPDU_mean;
                float mAMPDU_std;
                float mTime_delta;
                uint16_t mAMPDU_max;
                uint16_t mMPDU_num;
                uint16_t mPkt_Size;
                uint16_t mFREQ;
                /* data */



};
