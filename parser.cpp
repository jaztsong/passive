#include "parser.h"

using namespace std;
float calc_std(vector<uint16_t> v)
{
    assert(v.size() > 0);
    float sum = 0.0, mean, standardDeviation = 0.0;

    for(auto d:v){
        sum += d;
    }

    mean = sum/float(v.size());

    for(auto d:v)
        standardDeviation += pow(d - mean, 2);

    return sqrt(standardDeviation / v.size());
}
Parser::Parser(string name)
{
        mName = name;
}

void Parser::config(char* argv[], uint8_t l)
{
        mOffset = (uint16_t) atoi(argv[0]);
        mWindow = (uint16_t) atoi(argv[1]);
        mPeriod = (uint16_t) atoi(argv[2]);
        mStartTime = 0.0;
        m_pass_offset = false;
}
uint16_t Parser::getOffset(){
        return mOffset;
}
uint16_t Parser::getWindowSize(){
        return mWindow;
}
uint16_t Parser::getPeriod(){
        return mPeriod;
}
void Parser::start()
{
        ifstream infile(( mName + ".csv" ).c_str());
        D(" Start to parse csv file "<<( mName + ".csv" ).c_str());
        string line;
        bool first=true;
        Window_data* t_window_data = NULL;
        double pre_Time = 0.0;
        //TODO: Add time order check!! For Lixing Wed 24 Aug 2016 11:28:12 AM EDT.
        
        Line_cont* pre_line = NULL;
        while(getline(infile, line))
        {
                string line_value;
                istringstream ss(line);
                uint8_t count=0;
                Line_cont* line_c = NULL;
                while(getline(ss, line_value, '|'))
                {
                        if(count == 0 && first){
                                mStartTime = atof(line_value.c_str());
                                first=false;
                        }
                        if(count == 0 ){
                                if(is_in_range(line_value)){
                                        line_c = new Line_cont(line_value);
                                        D(" Start take lines "<<setprecision(18)<<mStartTime
                                                        << " "<<mOffset<<
                                                        " "<<line_c->getTime());
                                }
                                else
                                        line_c = NULL;
                        }
                        if (line_c != NULL)
                                line_c->read_field(line_value,count);
                        else
                                break;
                        count++;
                }
                
                if (line_c != NULL){
                        if(pre_Time == 0.0){
                                t_window_data = new Window_data(mStartTime,this);
                                t_window_data->addData(line_c);
                        }
                        else if(mStartTime != pre_Time){
                                t_window_data->addData(line_c);
                                t_window_data->parse();
                                line_c = NULL;
                                /* mWindow_datas.push_back(t_window_data); */
                                delete t_window_data;
                                t_window_data = new Window_data(mStartTime,this);
                        }
                        else{
                                assert(mStartTime == t_window_data->getTime());
                                t_window_data->addData(line_c);
                        }
                        /* if(pre_line != NULL){ */
                        /*         pre_line->next_line = line_c; */
                        /* } */


                        pre_line = line_c;

                        pre_Time = mStartTime;
                        /* D(" Index "<<setprecision(15)<<line_c->getTime()<<" "<<getIndex(line_c)); */

                }
        }
        t_window_data->parse();
        /* mWindow_datas.push_back(t_window_data); */
        delete t_window_data;

}

bool Parser::is_in_range(string s)
{
        /* D(setprecision(12)<<atof(s.c_str())*1000<<" "<<( mStartTime*1000 + mOffset )); */
        if( !m_pass_offset){
                if(atof(s.c_str())*1000 > ( mStartTime*1000 + mOffset )){
                        m_pass_offset = true;
                        mStartTime = atof(s.c_str());
                        return  true;
                }else{
                        return false;
                }
        }
        else{
                if(atof(s.c_str())*1000 < ( mStartTime*1000 + mWindow ) &&
                    atof(s.c_str()) >  mStartTime){
                        return true;
                }
                else if(atof(s.c_str())*1000 > ( mStartTime*1000 + mWindow )){
                        /* mStartTime += ( mWindow + mPeriod )/(float) 1000; */
                        /* The start of new window */
                        mStartTime = atof(s.c_str()) + mPeriod/(float)1000;
                        return false;
                }
                else{
                        return false;
                }

        }

}



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////Line_cont (Line Container)/////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

Line_cont::Line_cont(string t)
{
        mTime=atof(t.c_str());
        next_line = NULL;
}

void Line_cont::read_field(string s, uint8_t index)
{
        mData[( (Fields) index )]=s;
}
string Line_cont::get_field(Fields f)
{
        return mData[f];
}
void Line_cont::print_fields()
{
         for (map<Fields,string>::iterator it=mData.begin(); it!=mData.end(); ++it)
                     std::cout << it->first << "=>" << it->second <<'\t' ;
         std::cout<<endl;
}

double Line_cont::getTime()
{
        return mTime;

}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////Window_data /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

Window_data::Window_data(double time,Parser* p)
{
        mTime = time;
        mAPs.clear();
        mLines.clear();
        mOtherAP_pkts.clear();
        this->setParser(p);

        mUtil = 0.0;
        mAirtime = 0.0;
        mLoss = 0.0;
        mN_AP = 0;
        mN_client = 0;
        mMPDU_num = 0;
}

Parser* Window_data::getParser()
{
        if(mParser == NULL){
                cerr<<"*ERROR: get NULL parser from Window_data"<<endl;
        }
        return mParser;
}
void Window_data::setParser(Parser* p){
        mParser = p;
}
double Window_data::getTime()
{
        return mTime;
}

void Window_data::addData(Line_cont* l)
{
        mLines.push_back(l);
}

void Window_data::parse()
{

        D("Total Data size in this window is "<<setprecision(15)<<this->getTime()<<" "<<mLines.size());
        getAP_pool();
        D("Total Number of APs is "<<mAPs.size());
        assignData();
        for(map<string,AP_stat*>::iterator it = mAPs.begin();it != mAPs.end();++it){
                ( it->second )->setTime(this->getTime());
                D(( it->first )<<" has "<<( it->second )->getPacketN()<<" pkts");
                ( it->second )->go_calc();
                ( it->second )->report_AP();

                if(( it->second )->getN_clients() > 0) mN_AP++;
                mN_client += ( it->second )->getN_clients();
                mAirtime += ( it->second )->getAirTime_AP();
                mMPDU_num += ( it->second )->getN_MPDU_AP();
                mUtil += ( it->second )->getN_MPDU_AP()*( it->second )->getUtl_AP();
                mLoss += ( it->second )->getLoss_AP();

        }
        if(mMPDU_num > 0)
                mUtil = mUtil/float(mMPDU_num);
        /* D("Check the Other packets "); */
        /* vector<Line_cont*>* tPkts = mAPs["Other"]->getPkts(); */
        /* for(vector<Line_cont* >::iterator it = ( *tPkts ).begin(); it != (*tPkts).end();++it){ */
        /*     D((*it)->get_field(Line_cont::F_TIME)<<" " */
        /*                     <<(*it)->get_field(Line_cont::F_TA)); */
        /* } */

        this->report_channel();
                
}

void Window_data::clean_mem_chan()
{
       for(map<string,AP_stat*>::iterator it = mAPs.begin();it != mAPs.end();++it)
               delete  it->second;
       for(auto l:mLines){
               D("delete packet "<<l->getTime());
               delete l;
       }
       mLines.clear();
       mAPs.clear();
       mOtherAP_pkts.clear();
}

void Window_data::report_channel()
{
        printf("%-5s %10.6f %4d %4d %4d %6.3f %6.3f %6.3f\n",
                        "CHAN", //level
                        mTime, //time
                        mN_AP,//AP number
                        mN_client,//client number
                        mMPDU_num,//number of MPDUs
                        mLoss,  //mean of loss per A-MPDU
                        mAirtime,  //Airtime
                        mUtil  //Utilization
              );
        this->clean_mem_chan();
}

void Window_data::getAP_pool()
{
        for(vector<Line_cont*>::iterator it=mLines.begin(); it != mLines.end();++it ){
                /* (*it)->print_fields(); */
                /* D("Add Packet "<<( (*it)->get_field(Line_cont::F_TYPE_SUBTYPE))); */
                if(is_downlink(*it)){
                        string t_addr = (*it)->get_field(Line_cont::F_TA);
                        t_addr = t_addr.substr(0,ADD_LEN);
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr,this->getParser());
                }
                else if(is_uplink(*it)){
                        string t_addr = (*it)->get_field(Line_cont::F_RA);
                        t_addr = t_addr.substr(0,ADD_LEN);
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr,this->getParser());

                }
                else if(is_beacon_or_PR(*it)){
                        string t_addr = (*it)->get_field(Line_cont::F_TA);
                        t_addr = t_addr.substr(0,ADD_LEN);
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr,this->getParser());
                }
        }
        /* Lab Test setting: we know what the APs are */
        /* Please remove the chunk of code when doing none lab test */
        ////////////////////////////////////////////////////////////////////
        if(mAPs.find("ec:08:6b:71:d2:dc") == mAPs.end())
                mAPs["ec:08:6b:71:d2:dc"] = new AP_stat("ec:08:6b:71:d2:dc",this->getParser());

        if(mAPs.find("ec:08:6b:71:d2:dd") == mAPs.end())
                mAPs["ec:08:6b:71:d2:dd"] = new AP_stat("ec:08:6b:71:d2:dd",this->getParser());
        ///////////////////////////////////////////////////////////////////
        /* set a client pool */
        set<string> t_clients;
        for(vector<Line_cont*>::iterator it=mLines.begin(); it != mLines.end();++it ){
                /* Single direction packets do not count */
                string t_addr = (*it)->get_field(Line_cont::F_TA);
                string t1_addr = (*it)->get_field(Line_cont::F_RA);

                if(mAPs.find(t_addr) != mAPs.end()&& t1_addr.length()>0){
                        t_clients.insert(t1_addr);
                }
                else if(mAPs.find(t1_addr) != mAPs.end()&& t_addr.length()>0){
                        t_clients.insert(t_addr);
                }
                else if(t_addr.length()>0 && t1_addr.length()>0 ){
                        if(t_clients.find(t_addr) == t_clients.end() &&
                                        t_clients.find(t1_addr) == t_clients.end())
                                mOtherAP_pkts.push_back(*it);
                }
                /* if(mAPs.find(t_addr) == mAPs.end()) */
                /*         mAPs[t_addr] = new AP_stat(t_addr,this->getParser()); */

                /* string t_addr = getAP(line_c); */
        }
        Analyse_OtherAP();
}
void Window_data::Analyse_OtherAP()
{
        map<string,int> addr_count;
        map<string,set<string>> addr_map;
        for(auto p:mOtherAP_pkts){
                string t_addr = p->get_field(Line_cont::F_TA);
                string t1_addr = p->get_field(Line_cont::F_RA);
                /* if(mAPs.find(t_addr) != mAPs.end() || */
                /*                 mAPs.find(t1_addr) != mAPs.end()|| */
                /*                 t_addr.length() < 1) continue; */
                if(addr_count.find(t_addr) == addr_count.end()){
                        addr_count[t_addr] = 1;
                        addr_map[t_addr] = set<string> ();

                }
                else{
                        addr_count[t_addr] += 1;
                }
                addr_map[t_addr].insert(t1_addr);

                t_addr = p->get_field(Line_cont::F_RA);
                t1_addr = p->get_field(Line_cont::F_TA);

                if(addr_count.find(t_addr) == addr_count.end()){
                        addr_count[t_addr] = 1;
                        addr_map[t_addr] = set<string> ();
                }
                else{

                        addr_count[t_addr] += 1;
                }
                addr_map[t_addr].insert(t1_addr);

        }
        vector<pair<string, int>> sort_v(addr_count.begin(),addr_count.end());
        sort(sort_v.begin(), sort_v.end(), [](const std::pair<string,int> &left, const std::pair<string,int> &right) {
                        return left.second > right.second;
                        });
        int i = 0;
        while(i<sort_v.size() && addr_count.size()>0){
                /* cout<<i<<endl; */
                if(addr_count.find(sort_v[i].first) != addr_count.end()){
                        /* cout<<" haha addr "<<sort_v[i].first<<" has "<<sort_v[i].second<<" current addr_count size "<<addr_count.size()<<endl; */
                        if(mAPs.find(sort_v[i].first) == mAPs.end())
                                mAPs[sort_v[i].first] = new AP_stat(sort_v[i].first,this->getParser());
                        for(auto m:addr_map[sort_v[i].first]){
                                if(addr_count.find(m) != addr_count.end())
                                        addr_count.erase(addr_count.find(m));
                        }
                }
                i++;
        }

        
}
void Window_data::assignData()
{
        //The function is built for two purpose: 1) Assign data according to the AP
        //2) Assign data packets to each AMPDU according to the reference number
        //The second function has been aborted since our approach has been changed to only use 
        //Block ACK to determine A-MPDU stats For Lixing Mon 16 Jan 2017 10:34:13 AM EST.
        


        //TODO: One of the bugs of the current approach to add packets is that
        //The ACK packets which are destined to clients can not be classified into any of the AP group
        //since none of their TA and RA is AP address.
        //As the experiments mostly will focus on downlink case, the bug is not crucial for in-lab scenarios.
        //For Lixing Mon 16 Jan 2017 10:45:12 AM EST.
        
        
        for(vector<Line_cont*>::iterator it=mLines.begin(); it != mLines.end();++it ){
                string ta_addr = (*it)->get_field(Line_cont::F_TA);
                ta_addr = ta_addr.substr(0,ADD_LEN);
                string ra_addr = (*it)->get_field(Line_cont::F_RA);
                ra_addr = ra_addr.substr(0,ADD_LEN);
                //TODO: This is a crude filter
                //Please improve it For Lixing Tue 30 Aug 2016 12:26:40 PM EDT.
                
                if( ta_addr != "80:56:f2:20:c8:6" ||true ){
                        if(mAPs.find(ta_addr) != mAPs.end()){
                                mAPs[ta_addr]->addPacket(*it);
                        }
                        else if ( mAPs.find(ra_addr) != mAPs.end()){
                                mAPs[ra_addr]->addPacket(*it);
                        }
                        /* else */
                        /*         mAPs["Other"]->addPacket(*it); */
                }
        }
}
bool Window_data::is_downlink(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_DS) == "2" ) || ( l->get_field(Line_cont::F_DS) == "0x02" ); 
}

bool Window_data::is_uplink(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_DS) == "1" ) || ( l->get_field(Line_cont::F_DS) == "0x01" ); 
}
bool Window_data::is_beacon_or_PR(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "8" ) 
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x08" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "5" ) 
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x05" ); 
}
/* string Window_data::getAP(Line_cont* l) */
/* { */

/* } */



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////AP_Stat /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

AP_stat::AP_stat(string mac,Parser* p)
{
        mAddr=mac;
        mTime = 0;
        mMBytes = 0;
        mPackets.clear();
        mRetrys.clear();
        m_nACK=0;
        m_nBlockACK=0;
        m_nBlockACKreq=0;
        m_nMLframe = 0;
        mMPDU_num = 0;
        mClient_pool.clear();
        mBlkACKs.clear();

        mN_client = 0;
        mAirtime = 0.0;
        mUtil = 0.0;
        mLoss = 0.0;
        this->setParser(p);

}
Parser* AP_stat::getParser()
{
        if(mParser == NULL){
                cerr<<"*ERROR: get NULL parser from AP_stat"<<endl;
        }
        return mParser;
}
void AP_stat::setParser(Parser* p){
        mParser = p;
}
void AP_stat::addPacket(Line_cont* l)
{
        mPackets.push_back(l);
}


uint32_t AP_stat::getPacketN()
{
        return mPackets.size();
}

uint16_t AP_stat::getN_clients()
{
        return mN_client;

}

uint16_t AP_stat::getN_MPDU_AP()
{
        return mMPDU_num;

}
vector<Line_cont*>* AP_stat::getPkts()
{
        return &mPackets;
}

void AP_stat::classifyPkts(){
        m_nDowns = 0;
        m_nUPs = 0;
        for(vector<Line_cont* >::iterator it = mPackets.begin(); it != mPackets.end();++it){
                string s_byte = ( *it )->get_field(Line_cont::F_LEN);
                uint16_t t_bits = atoi(s_byte.c_str())*8;
                mMBytes += t_bits/(float)8000000;
                if(is_blockACK(*it)){
                        m_nBlockACK++;
                }else if(is_ACK(*it)){
                        m_nACK++;
                }else if(is_blockACKreq(*it)){
                        m_nBlockACKreq++;
                }else if(is_beacon_or_PR(*it)){
                        m_nMLframe++;
                }

                if(is_downlink(*it)){
                        m_nDowns++;
                }
                else{
                        m_nUPs++;
                }
                //Capture the retrys
                string t_seq = (*it)->get_field(Line_cont::F_SEQ);
                uint64_t i_seq = atoi(t_seq.c_str());
                if ((*it)->get_field(Line_cont::F_RETRY) == "1"){
                        if(mRetrys.find(i_seq) == mRetrys.end()){
                                mRetrys[i_seq]=1;
                        }
                        else{
                                if(mRetrys[i_seq]<7)
                                        mRetrys[i_seq]++;
                        }
                        D(" Retrans "<<mRetrys[i_seq]);
                }
        }
}
void AP_stat::prepare_BAMPDU()
{
        for(vector<Line_cont* >::iterator it = mPackets.begin(); it != mPackets.end();++it){
                //Populate BlkACK stats
                if(is_blockACK(*it)){
                        BlkACK* t_b =  new BlkACK(*it);
                        if(mBlkACKs.find(t_b->addr) == mBlkACKs.end()){
                                        BlkACK_stat* t_blkack_stat = new BlkACK_stat(t_b->addr,this);
                                        mBlkACKs[t_b->addr]=t_blkack_stat;
                        }
                        mBlkACKs[t_b->addr]->addACK(t_b);
                        /* check if the blockAck is responding a blockAckreq */
                        /* The reason to do this is that, as observed, when a blockACK is purposed to respond */
                        /* a blockACKreq, the bitmap is meaningless. In this case, the blockACK does not infer any */
                        /* data transmission. */
                        if(!m_queue_blockACKreq.empty()){
                                Line_cont* t_line = m_queue_blockACKreq.front();
                                string t_addr1 = t_line->get_field(Line_cont::F_TA);
                                string t_addr2 = t_line->get_field(Line_cont::F_RA);
                                if(t_addr2 + t_addr1 == t_b->addr){
                                        /* cout<<"Find matched BlkACKreq."<<endl; */
                                        m_queue_blockACKreq.pop();
                                        continue;
                                }
                        }
                        /* The core function is here: we do parsing along with addACK */
                        mBlkACKs[t_b->addr]->parse_AMPDU();

                }
                /* collect blockACKreq */ 
                else if(is_blockACKreq(*it)){
                        /* cout<<"Glean BlkACKreq."<<endl; */
                        m_queue_blockACKreq.push(*it);
                }
        }
}

void AP_stat::setTime(double t)
{
        mTime = t;
}

double AP_stat::getTime()
{
        return mTime;
}
void AP_stat::go_calc()
{
        /* this->getAMPDU_stat(); */
        /* this->classifyPkts(); */
        this->prepare_BAMPDU();
        /* this->getBAMPDU_stats();// This must be after calculate the blk-based aritime, since the AMPDU len */
        // is provided when calculating airtime
        this->calc_clients();


        for(auto const it:mClient_pool){
                it.second->calc_stats();
                it.second->report_client();
                
                mAirtime += it.second->getAirTime_client();
                mLoss += it.second->getLoss_client();
                mUtil += it.second->getN_MPDU_client()*it.second->getUtl_client();
                mMPDU_num += it.second->getN_MPDU_client();

                mN_client++;

        }
        if(mMPDU_num > 0)
                mUtil = mUtil/float(mMPDU_num);

        this->calc_ack_airtime();
}

void AP_stat::calc_ack_airtime()
{
        for(auto p:mPackets){
                if(is_ACK(p)){
                        mAirtime += 0.2*1000*atof(p->get_field(Line_cont::F_TIME_DELTA).c_str());
                }
        }
        
}
float AP_stat::getAirTime_AP()
{
        return mAirtime;
}

float AP_stat::getUtl_AP()
{
        return mUtil;
}
float AP_stat::getLoss_AP()
{
        return mLoss;
}
void AP_stat::report_AP()
{
        //format:
        //time AP_address packet_number unweighted_airtime, weighted_airtime, AMPDU_num,
        //block_ack_num regular_ack_num
        printf("%-5s %10.6f %-16s %4d %4d %6.3f %6.3f %6.3f\n",
                        "AP", //level
                        mTime, //time
                        mAddr.c_str(),  //AP addr
                        mN_client,//client number
                        mMPDU_num,//number of MPDUs
                        mLoss,  //mean of loss per A-MPDU
                        mAirtime,  //Airtime
                        mUtil  //Utilization
              );
        this->clean_mem_AP();
}
void AP_stat::clean_mem_AP()
{
       for(map<string,BlkACK_stat*>::iterator it = mBlkACKs.begin();it != mBlkACKs.end();++it) 
               delete it->second;
       for(map<string,Client_stat*>::iterator it = mClient_pool.begin();it != mClient_pool.end();++it) 
               delete it->second;

        mPackets.clear();
        mRetrys.clear();
        mClient_pool.clear();
        mBlkACKs.clear();

}

void AP_stat::calc_clients()
{
        if(mBlkACKs.size()>0){
                for(map<string, BlkACK_stat*>::iterator it = mBlkACKs.begin();it!=mBlkACKs.end();++it){
                        string client_addr;
                        if(it->first.substr(17,17) != mAddr){
                                client_addr = it->first.substr(17,17);
                        }
                        else{
                                /* a boring assertion */
                                assert(it->first.substr(0,17) != mAddr);
                                client_addr = it->first.substr(0,17);
                        }
                        /* If the current client has not been added to the pool, then add it */
                        if(mClient_pool.find(client_addr) == mClient_pool.end()){
                                Client_stat* t_client = new Client_stat(client_addr,this);
                                mClient_pool[client_addr] = t_client;
                        }
                        mClient_pool[client_addr]->addBlk_stat(it->second);
                        
                }
        }
}

string AP_stat::getAddr()
{
        return mAddr;
}
bool AP_stat::is_ACK(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "29" ) || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x1d" ); 
}

bool AP_stat::is_blockACK(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "25" ) || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x19" ); 
}


bool AP_stat::is_blockACKreq(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "24" ) || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x18" ); 
}

bool AP_stat::is_data(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "40" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x28" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "44" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x2c" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "32" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x20" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "36" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x24" )
               ; 
}

bool AP_stat::is_beacon_or_PR(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "8" ) 
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x08" )
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "5" ) 
               || ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "0x05" ); 
}

bool AP_stat::is_downlink(Line_cont* l)
{
        string t_addr = l->get_field(Line_cont::F_TA);
        t_addr = t_addr.substr(0,ADD_LEN);
        if ( t_addr == mAddr){
                return true;
        }
        else{
                return false;
        }

        
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////Client_stat//////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
Client_stat::Client_stat(string addr, AP_stat* ap)
{
        mAddr = addr;
        mAP_stat = ap;
        mAirtime = 0.0;
        mUtil = 0.0;
        mLoss = 0.0;
        mMPDU_num = 0;
        mTime_delta_median = 0.0;
        mAMPDU_max = 0.0;
        mRate = 0.0;
        mBlkACKs_client.clear();
}

void Client_stat::report_client()
{
        printf("%-5s %10.6f %-16s %-16s %4d %6.3f %6.3f %6.3f %6.3f %6.3f\n",
                        "CLIENT", //level
                        mAP_stat->getTime(), //level
                        mAddr.c_str(),  //client addr
                        mAP_stat->getAddr().c_str(),  //AP addr
                        mMPDU_num,//number of MPDUs
                        mLoss,  //mean of loss per A-MPDU
                        mTime_delta_median,  //median blockACK gap
                        getAirTime_client(),  //minimum blockACK gap
                        getUtl_client(),  //Utilization
                        getRate_client()
              );
        this->clean_mem_client();

}
void Client_stat::clean_mem_client()
{
       mBlkACKs_client.clear(); 
}

void Client_stat::addBlk_stat(BlkACK_stat* b)
{
        mBlkACKs_client.push_back(b);
}
float Client_stat::getRate_client()
{
        return mRate;
}
void Client_stat::calc_stats()
{
        if(mBlkACKs_client.size() > 0){
                for(auto blk_stat:mBlkACKs_client)
                        blk_stat->calc_stats();
                /* The size inference part */
                ////////////////////////////////////////////
                //The code here is quite ulgy. The reason is the dependency between each of the
                //functions that setSize need the A-MPDU mean from calc_stats, and calc_rate requires setSize finished.
                if(mBlkACKs_client.size() == 2){

                        if(mBlkACKs_client[0]->getAMPDU_mean() > mBlkACKs_client[1]->getAMPDU_mean()){
                                mBlkACKs_client[0]->setPktSize(MTU);
                                mBlkACKs_client[1]->setPktSize(ACK_LEN);
                                mBlkACKs_client[0]->calc_rate();
                                mBlkACKs_client[1]->calc_rate();
                                mRate = mBlkACKs_client[0]->getRate_flow();
                                
                                /* cout<<mBlkACKs_client[0]->getAddr()<<" haha get MTU with "<<mBlkACKs_client[0]->getAMPDU_mean()<<" to "<<mBlkACKs_client[1]->getAMPDU_mean()<<endl; */
                        }else{

                                mBlkACKs_client[1]->setPktSize(MTU);
                                mBlkACKs_client[0]->setPktSize(ACK_LEN);
                                mBlkACKs_client[1]->calc_rate();
                                mBlkACKs_client[0]->calc_rate();
                                mRate = mBlkACKs_client[1]->getRate_flow();
                                /* cout<<mBlkACKs_client[1]->getAddr()<<" haha get MTU with "<<mBlkACKs_client[1]->getAMPDU_mean()<<" to "<<mBlkACKs_client[0]->getAMPDU_mean()<<endl; */
                        }
                }
                else {
                        mBlkACKs_client[0]->setPktSize(MTU);
                        mBlkACKs_client[0]->calc_rate();
                        mRate = mBlkACKs_client[0]->getRate_flow();
                        /* cout<<mBlkACKs_client[0]->getAddr()<<" haha get MTU by default"<<endl; */

                }
                /////////////////////////////////////////////
                for(auto blk_stat:mBlkACKs_client){
                        /* Report at the directional flow level */
                        /* blk_stat->report_flow(); */

                        mTime_delta_median = max(mTime_delta_median,blk_stat->getGap_mean_flow());
                        /* mAMPDU_max = max(mAMPDU_max,blk_stat->getAMPDU_max()); */
                        mAirtime += blk_stat->getAirTime_flow();
                        if(blk_stat->getPktSize() == MTU){
                                mMPDU_num +=  blk_stat->getN_MPDU_flow();
                                mLoss += blk_stat->getLoss_flow();
                                mUtil += blk_stat->getUtl_flow();
                        }
                        /* Report at the directional flow level */
                        blk_stat->report_flow();
                }
        }
}

uint16_t Client_stat::getN_MPDU_client()
{
        return mMPDU_num;
}
float Client_stat::getUtl_client()
{
        return mUtil;
}
float Client_stat::getLoss_client()
{
        return mLoss;
}
float Client_stat::getAirTime_client()
{
        
        return mAirtime;
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////BlkACK//////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
BlkACK::BlkACK(Line_cont* l)
{
        string t_addr1 = l->get_field(Line_cont::F_TA);
        string t_addr2 = l->get_field(Line_cont::F_RA);
        string t_addr = t_addr1 + t_addr2;
        /* double t_time = atof( ( l->get_field(Line_cont::F_TIME) ).c_str() ); */
        uint16_t t_ssn = atoi( ( l->get_field(Line_cont::F_BLKACK_SSN) ).c_str() );
        int t_rssi = atoi( ( l->get_field(Line_cont::F_RSSI) ).c_str() );
        istringstream ss(l->get_field(Line_cont::F_BLKACK_BM));
        string byte;
        uint8_t index=0;
        
        this->addr = t_addr;
        this->addr_rev = t_addr1.compare(t_addr2) < 0;
        this->line = l;
        this->SSN = t_ssn;
        this->RSSI = t_rssi;
        this->Miss.clear();
        while(getline(ss, byte, ':'))
        {
                unsigned long tt=strtoul(byte.c_str(),NULL,16);
                for( int j=0;j<8;j++ ){
                    if(!( (unsigned char)( tt >> j )  & ( 0x1 ) ))
                            this->Miss.push_back(this->SSN + index*8 + j );
                    /* cout<<j+index*8<<" "<<( (tt>>j)&(0x1 ) )<<" "; */
                }
                index++;
        }

}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////BlkACK_stat /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
BlkACK_stat::BlkACK_stat(string s,AP_stat* ap)
{
        mAddr = s;
        mAP_stat = ap;
        mACKs.clear();
        mvector_Loss.clear();
        mAMPDU_mean = 0.0;
        mLoss = 0.0;
        mAMPDU_max = 0;
        mMPDU_num = 0;
        mAirtime = 0.0;
        mUtil = 0.0;
        mPkt_Size = 0;
        mFREQ = 0;
        mRSSI_mean = -100;
        mAMPDU_std = 0;
        mTime_delta = 0.0;
        mTime_delta_median = 0.0;
        mAMPDU_tuple.clear();
        mRate = 0.0;
}
void BlkACK_stat::addACK(BlkACK* b)
{
        mACKs.push_back(b);
}
string BlkACK_stat::getAddr()
{
       return mAddr; 
}
                
bool BlkACK_stat::parse_AMPDU()
{
        uint16_t t_len=0;
        uint16_t t_len_miss=0;
        if(mACKs.size()>1){
                //The Blk ACK vector has more than 1 acks, 
                //then we need consider the relationship between the current one and previous one
                if(mACKs.back()->Miss.size()>0){
                        t_len_miss = set_diff(mACKs.back()->Miss,mACKs[mACKs.size()-2]->Miss);
                        /* //Current blk ack indicates loss */
                        /* vector<uint16_t> t_diff; */
                        /* t_diff = set_diff(mACKs[mACKs.size()-2]->Miss,mACKs.back()->Miss); */
                        /* t_len_miss = t_diff.size(); */  
                        /* Debug */
                        /* cout<<" t_len under loss "<<t_len_miss<<" "<<endl; */

                        if(mACKs[mACKs.size()-2]->Miss.size() < 1){
                                mvector_Loss.push_back(t_len_miss);
                        }

                }

                /* Compute the SSN distance. */
                if(mACKs.back()->SSN < mACKs[mACKs.size()-2]->SSN){
                        t_len = mACKs.back()->SSN + 4096 - mACKs[mACKs.size()-2]->SSN;
                }else{
                        t_len = mACKs.back()->SSN - mACKs[mACKs.size()-2]->SSN;
                }
                t_len += set_diff(mACKs[mACKs.size()-2]->Miss,mACKs.back()->Miss);

                /* Abnormal case hard filter */
                if(t_len > 64)
                        t_len = 1;

                t_len_miss = min(t_len, t_len_miss);
                
                /* mACKs.back()->line->print_fields(); */
                /* cout<<t_len<<" "<<t_len_miss<<endl; */

        }
        else{
                return false;
        }

        /* As we set the display filter to only show the non-data packets, we can use the time_delta_displayed metric to measure */
        /* the time gap of the ACK distancing from previous non-data packet. The gap might help infer the data rate. */
        float time_delta = 0.0;
        float t_ack_gap = 1000*atof(mACKs.back()->line->get_field(Line_cont::F_TIME_DELTA).c_str());
        float pre_own_ack = 1000*( mACKs.back()->line->getTime() - mACKs[mACKs.size()-2]->line->getTime());
        bool if_continue = abs(t_ack_gap-pre_own_ack) < 0.001;
        //TODO: The threshold need to be double checked.
        //For Lixing Fri 03 Feb 2017 02:02:51 PM EST.
        
        /* t_ack_gap = max(t_ack_gap,(float)0.048); */
        if(t_len  > 0)
                time_delta = t_ack_gap/float(t_len) ; // convert into millisecond.
        /* time_delta = max(time_delta,(float)0.016); */

        /* The variable name mBAMPDU means AMPDU based on BlkACK. */
        mAMPDU_tuple.push_back(make_tuple(t_len,t_len_miss,mACKs.back()->RSSI,time_delta,if_continue));

        report_pkt();

        return true;

        /* if(t_len > 0){ */
        /*         //add the AMPDU information for AP_stat to report */
        /*         mAP_stat->addBAMPDU(t_len); */
        /*         return t_len; */
        /* } */
        /* else{ */
        /*         return 0; */
        /* } */
}
void BlkACK_stat::report_pkt()
{
        if(mAMPDU_tuple.size() > 0){
                printf("%-5s %10.6f %10.6f %-16s %4d %4d %3d %6.3f\n",
                                "PKT", //level
                                mACKs.back()->line->getTime(),//pkt time
                                mAP_stat->getTime(), //AP time
                                mAddr.c_str(),  //addr
                                get<0>( mAMPDU_tuple.back() ),  //AI:A-MPDU Intensity
                                get<1>( mAMPDU_tuple.back() ),  //loss
                                get<2>( mAMPDU_tuple.back() ),  //RSSI
                                get<3>( mAMPDU_tuple.back() )  //blockACK gap
                      );
        } 
}

void BlkACK_stat::calc_stats()
{
        int sum = 0, n = 0, RSSI_sum = 0, loss_sum = 0 , n1= 0;
        float sum_time_delat = 0.0;
        vector<uint16_t> t_vector;//for computing the std
        if(mAMPDU_tuple.size()>0){
                priority_queue<uint16_t> percentile_q;//it is used to help quickly get the percentile;
                priority_queue<float,vector<float>,std::greater<float>> percentile_q1;//it is used to help quickly get the percentile;
                for(vector<tuple<uint16_t,uint16_t,int,float,bool> >::iterator it = mAMPDU_tuple.begin();it!=mAMPDU_tuple.end();++it){
                        sum += get<0>(*it);
                        loss_sum += get<1>(*it);
                        RSSI_sum += get<2>(*it);
                        if( get<0>(*it) > 1 ) sum_time_delat += get<3>(*it)*get<0>(*it);
                        if(get<0>(*it) < 65 ) percentile_q.push(get<0>(*it));
                        if(get<3>(*it) > 0.016 && get<0>(*it) > 1 ) {
                                percentile_q1.push(get<3>(*it));
                                n1++;
                        }
                        t_vector.push_back(get<0>(*it));
                        n++;
                        
                }
                mAMPDU_mean = sum/float(n);
                mMPDU_num = sum;
                mAMPDU_std = calc_std(t_vector);
                /* Deal with loss */
                mLoss = loss_sum;
                mRSSI_mean = RSSI_sum/float(n);
                /* Didn't use this metric so far */
                mTime_delta = sum_time_delat/float(n);
                /* Get the frequency. */
                mFREQ = atoi(mACKs[0]->line->get_field(Line_cont::F_FREQ ).c_str());
                /* The part is quite essential to our research: calculating the maximum of the A-MPDU Intensity */
                /* Get the percentile of the AMPDU as max to filter out some abnormal cases. */
                uint16_t percent_N = 1;
                while(percent_N < mAMPDU_tuple.size()*0.05 && !percentile_q.empty()){
                        /* cout<<percentile_q.top()<<endl; */
                        percentile_q.pop();
                        percent_N++;
                }
                if(!percentile_q.empty()) mAMPDU_max = percentile_q.top();
                /* The part is quite essential to our research: calculating the minimum of the Block-ACK gap */
                percent_N = 1;
                /* Adjust the percentile to take as valid gap based on 802.11ac or 802.11n */
                float gap_cap = 0.1;
                if(mFREQ > 0 && mFREQ < 5000) gap_cap = 0.4;
                while(percent_N < n1*gap_cap && !percentile_q1.empty()){
                        /* cout<<"haha pop min gap "<<percentile_q.top()<<endl; */
                        percentile_q1.pop();
                        percent_N++;
                }
                if(!percentile_q1.empty()) mTime_delta_median = percentile_q1.top();



                /* cout<<"haha checkout the max transmission time "<<max_trans_time<<endl; */
                /* calculate the potential max AI first */
                /* uint16_t t_max_ai = 0; */
                /* if(mTime_delta_median > 0){ */
                /*         /1* mAMPDU_max = min((int)max((float)mAMPDU_max,MAX_TRANS_TIME/mTime_delta_median),BITMAP_LEN); *1/ */
                /* } */
                /* Adjust minimum time gap based on the t_max_ai */
                if(mAMPDU_max>0){
                        mTime_delta_median = min(mTime_delta_median, MAX_TRANS_TIME/float(mAMPDU_max));
                        /* Calculate Airtime */
                        /* mAirtime = (  mMPDU_num + 0*mLoss )*mTime_delta_median + n*0.0; */
                        mAirtime = sum_time_delat;
                        /* Calculate Utilization */
                        mUtil = mAMPDU_mean/float(mAMPDU_max);
                }
        }

        /* cout<<mAddr<<" "<<mAMPDU_mean<<" "<<mAMPDU_std<<" "<<mAMPDU_max<<" "<<mRSSI_mean<<" "<<mTime_delta<<" "<<mMPDU_num<<" "<<mFREQ<<endl; */
        /* cout<<mAddr<<" "<<mAMPDU_max<<" "<<mTime_delta_median<<endl; */
        

}

void BlkACK_stat::calc_rate()
{
        if(mTime_delta_median > 0){
                /* mAMPDU_max = min((int)max((float)mAMPDU_max,MAX_TRANS_TIME/mTime_delta_median),BITMAP_LEN); */
                mRate =  mPkt_Size*8/float(mTime_delta_median*1000);
        }
}
float BlkACK_stat::getLoss_flow()
{
        return mLoss;
}
float BlkACK_stat::getGap_mean_flow()
{
        return mTime_delta_median;
}

float BlkACK_stat::getAMPDU_mean()
{
        return mAMPDU_mean;
}

uint16_t BlkACK_stat::getAMPDU_max()
{
        return mAMPDU_max;
}

uint16_t BlkACK_stat::getN_MPDU_flow()
{
        return mMPDU_num;
}

float BlkACK_stat::getAirTime_flow()
{
        
        return mAirtime;
}
float BlkACK_stat::getUtl_flow()
{
        return mUtil;
}
void BlkACK_stat::report_flow()
{
        
        printf("%-5s %10.6f %-16s %4d %6.3f %4d %6.3f %6.3f %6.3f %6.3f %6.3f\n",
                        "FLOW", //level
                        mAP_stat->getTime(), //level
                        mAddr.c_str(),  //addr
                        mMPDU_num,  //number of MPDUs
                        mAMPDU_mean, //mean of AI
                        mAMPDU_max, //max of AI
                        mLoss,  //mean of loss per A-MPDU
                        mTime_delta_median,  //median blockACK gap
                        mAirtime,  //minimum blockACK gap
                        mUtil,  //Utilization
                        mRate  //Transmission
              );
        this->clean_mem_flow();
}

void BlkACK_stat::clean_mem_flow()
{
        
        for(auto it:mACKs)
                delete it;
        mAMPDU_tuple.clear();
        mvector_Loss.clear();
}
float BlkACK_stat::getRate_flow()
{
        return mRate;
}
void BlkACK_stat::setPktSize(uint16_t s)
{
        mPkt_Size = s;
}

uint16_t BlkACK_stat::getPktSize()
{
       return mPkt_Size;
}
/* float BlkACK_stat::getRate(BlkACK* l) */
/* { */
/*         assert(l->Miss.size() == 0); */
/*         if(mData.find(l->SSN + BITMAP_LEN - 1) != mData.end()) */
/*                 return atof( ( mData[l->SSN + BITMAP_LEN - 1]->get_field(Line_cont::F_RATE) ).c_str() ); */
/*         else{ */
/*                 //TODO:Select rate based on RSSI and current capacity */
/*                 //For Lixing Fri 09 Sep 2016 03:45:37 PM EDT. */
/*                 return 72.4; */
                
/*         } */

/* } */

/* float BlkACK_stat::getRate(uint16_t seq) */
/* { */
/*         if(mData.find(seq) != mData.end()) */
/*                 return atof( ( mData[seq]->get_field(Line_cont::F_RATE) ).c_str() ); */
/*         else{ */
/*                 //TODO:Select rate based on RSSI and current capacity */
/*                 //For Lixing Fri 09 Sep 2016 03:45:37 PM EDT. */
/*                 return 72.4; */
                
/*         } */

/* } */
int BlkACK_stat::set_diff(vector<uint16_t>& a,vector<uint16_t>& b)
{
        /* //By calculating the set diff of two consecutive ACKs, we can get which */ 
        /* std::vector<uint16_t> v; */                      
        /* cout<<" calculate set_diff a.size() "<<a.size() */
        /*         <<" b.size() "<<b.size()<<endl; */
        /* std::vector<uint16_t>::iterator it=b.begin(); */
        /* vector<uint16_t>::iterator t=a.begin(); */
        /* while( ( t!=a.end() ) && ( it != b.end() )){ */
        /*         if(*t > *it){ */
        /*                 ++it; */
        /*         } */
        /*         else if(*it > *t){ */
        /*                 ++t; */
        /*         } */
        /*         else{ */
        /*                 v.push_back(*t); */
        /*                 ++t; */
        /*                 ++it; */
        /*         } */
        /* } */
        /* vector<uint16_t> v1(a.size()); */
        /* it = std::set_difference(a.begin(),a.end(),v.begin(),v.end(),v1.begin()); */
        /* v1.resize(it - v1.begin()); */
        /* return v1; */
        
        /* The new method of calculating set difference. */
        vector<uint16_t> diff;
        set_difference(a.begin(),a.end(),b.begin(),b.end(),inserter(diff,diff.begin()));
        /* cout<<"claculate set_diff result "<<diff.size()<<endl; */

        return diff.size();
}
        








