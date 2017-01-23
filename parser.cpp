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
                                        /* D(" Start take lines "<<setprecision(15)<<mStartTime */
                                        /*                 << " "<<mOffset<< */
                                        /*                 " "<<line_c->getTime()); */
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
                                t_window_data->parse();
                                mWindow_datas.push_back(t_window_data);
                                t_window_data = new Window_data(mStartTime,this);
                                t_window_data->addData(line_c);
                        }
                        else{
                                assert(mStartTime == t_window_data->getTime());
                                t_window_data->addData(line_c);
                        }
                        if(pre_line != NULL){
                                pre_line->next_line = line_c;
                        }


                        pre_line = line_c;

                        pre_Time = mStartTime;
                        /* D(" Index "<<setprecision(15)<<line_c->getTime()<<" "<<getIndex(line_c)); */

                }
        }
        t_window_data->parse();
        mWindow_datas.push_back(t_window_data);

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
        this->setParser(p);
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
                ( it->second )->report();
        }
        /* D("Check the Other packets "); */
        /* vector<Line_cont*>* tPkts = mAPs["Other"]->getPkts(); */
        /* for(vector<Line_cont* >::iterator it = ( *tPkts ).begin(); it != (*tPkts).end();++it){ */
        /*     D((*it)->get_field(Line_cont::F_TIME)<<" " */
        /*                     <<(*it)->get_field(Line_cont::F_TA)); */
        /* } */
                


 
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
                else{
                        string t_addr("Other");
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr,this->getParser());

                }
                /* string t_addr = getAP(line_c); */

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
                        else
                                mAPs["Other"]->addPacket(*it);
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

void AP_stat::go_calc()
{
        /* this->getAMPDU_stat(); */
        this->classifyPkts();
        this->prepare_BAMPDU();
        this->getBAMPDU_stats();// This must be after calculate the blk-based aritime, since the AMPDU len
        // is provided when calculating airtime
}

void AP_stat::report()
{
        //format:
        //time AP_address packet_number unweighted_airtime, weighted_airtime, AMPDU_num,
        //block_ack_num regular_ack_num
        if(REPORT_LEVEL>0){
                printf("%4d %10.6f %-16s %4d %10.3f %4d %10.3f %4d(ML) %4d(Up) %4d(Down) %4d(BlkACK) %4d(ACK) %4d(BlkACKreq)\n",
                                mParser->getWindowSize(),
                                mTime,mAddr.c_str(),this->getPacketN(),mMBytes,
                                mB_nAMPDU,mB_AMPDU_len_mean,
                                m_nMLframe,
                                m_nUPs,m_nDowns,
                                m_nBlockACK,m_nACK,m_nBlockACKreq);
        }else{
                /* printf("%4d %10.6f %-16s %10.3f %10.3f %4d %10.3f %10.3f %4d\n", */
                /*                 mParser->getWindowSize(), */
                /*                 mTime,mAddr.c_str(), */
                /*                 mAirTime + mAMPDUinc, */
                /*                 m_AMPDU_len_mean,m_nAMPDU, */
                /*                 mBlkAirTime, */
                /*                 mB_AMPDU_len_mean,mB_nAMPDU */
                /*       ); */

        }
}



void AP_stat::getBAMPDU_stats(){
        if(mBlkACKs.size()>0){
                float sum=0;
                uint16_t n_ampdu = 0;
                for(map<string, BlkACK_stat*>::iterator it = mBlkACKs.begin();it!=mBlkACKs.end();++it){
                        it->second->calc_stats();
                        /* The output helps list the AMPDU stats on a per-ACK basis */
                        for(vector<tuple<uint16_t,uint16_t,int,float,bool> >::iterator iit = ( it->second ->mAMPDU_tuple ).begin();iit!=( it->second->mAMPDU_tuple ).end();++iit){
                                cout<<it->first<<" "<<get<0>(*iit)<<" "<<get<1>(*iit)<<" "<<get<2>(*iit)<<" "<<get<3>(*iit)<<endl;
                                sum += get<0>(*iit);     
                                n_ampdu++;
                        }
                }
                /* The overall AMPDU stats across all clients seems like a little pointless as each node has different data rate and */ 
                /* max AMPDU. But for our one node experiment, the stats is good to use. */
                mB_nAMPDU=sum;
                mB_AMPDU_len_mean=sum/(float)n_ampdu;


                /* Set the packet size: if TCP, the bigger flow (data) is set to MTU, ACK flow is set to 64 */
                for(map<string, BlkACK_stat*>::iterator it = mBlkACKs.begin();it!=mBlkACKs.end();++it){
                        string rev_addr = it->first.substr(17,17) + it->first.substr(0,17);
                        if(mBlkACKs.find(rev_addr) != mBlkACKs.end()){
                                if(mBlkACKs[rev_addr]->getAMPDU_mean() > it->second->getAMPDU_mean() &&
                                                mBlkACKs[rev_addr]->getMPDU_num() > it->second->getMPDU_num() ){
                                        mBlkACKs[rev_addr]->setPktSize(MTU);
                                        it->second->setPktSize(ACK_LEN);
                                }else{

                                        mBlkACKs[rev_addr]->setPktSize(ACK_LEN);
                                        it->second->setPktSize(MTU);
                                }
                        }
                        else{
                                it->second->setPktSize(MTU);

                        }
                }
        }
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
        mLoss.clear();
        mAMPDU_mean = 0.0;
        mAMPDU_max = 0;
        mMPDU_num = 0;
        mPkt_Size = 0;
        mFREQ = 0;
        mRSSI_mean = -100;
        mAMPDU_std = 0;
        mTime_delta = 0.0;
        mAMPDU_tuple.clear();
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
                                mLoss.push_back(t_len_miss);
                        }

                }

                /* Compute the SSN distance. */
                if(mACKs.back()->SSN < mACKs[mACKs.size()-2]->SSN){
                        t_len = mACKs.back()->SSN + 4096 - mACKs[mACKs.size()-2]->SSN;
                }else{
                        t_len = mACKs.back()->SSN - mACKs[mACKs.size()-2]->SSN;
                }
                t_len += set_diff(mACKs[mACKs.size()-2]->Miss,mACKs.back()->Miss);
                
                /* mACKs.back()->line->print_fields(); */
                /* cout<<t_len<<" "<<t_len_miss<<endl; */

        }
        else{
                return false;
        }

        /* As we set the display filter to only show the non-data packets, we can use the time_delta_displayed metric to measure */
        /* the time gap of the ACK distancing from previous non-data packet. The gap might help infer the data rate. */
        float time_delta = 1000*atof(mACKs.back()->line->get_field(Line_cont::F_TIME_DELTA).c_str())/float(t_len) ; // convert into millisecond.

        /* The variable name mBAMPDU means AMPDU based on BlkACK. */
        mAMPDU_tuple.push_back(make_tuple(t_len,t_len_miss,mACKs.back()->RSSI,time_delta,mACKs.back()->addr_rev));

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

void BlkACK_stat::calc_stats()
{
        int sum = 0, n = 0, RSSI_sum = 0;
        float sum_time_delat = 0.0;
        vector<uint16_t> t_vector;
        if(mAMPDU_tuple.size()>0){
                priority_queue<uint16_t> percentile_q;//it is used to help quickly get the percentile;
                for(vector<tuple<uint16_t,uint16_t,int,float,bool> >::iterator it = mAMPDU_tuple.begin();it!=mAMPDU_tuple.end();++it){
                        sum += get<0>(*it);
                        RSSI_sum += get<2>(*it);
                        sum_time_delat += get<3>(*it);
                        n++;
                        percentile_q.push(get<0>(*it));
                        t_vector.push_back(get<0>(*it));
                }
                mAMPDU_mean = sum/float(n);
                mMPDU_num = sum;
                mAMPDU_std = calc_std(t_vector);
                /* Get the percentile of the AMPDU as max to filter out some abnormal cases. */
                uint16_t percent_N = 1;
                while(percent_N < mAMPDU_tuple.size()*0.1 && !percentile_q.empty()){
                        /* cout<<percentile_q.top()<<endl; */
                        percentile_q.pop();
                        percent_N++;
                }
                if(!percentile_q.empty()) mAMPDU_max = percentile_q.top();
                mRSSI_mean = RSSI_sum/float(n);
                mTime_delta = sum_time_delat/float(n);

                /* Get the frequency. */
                mFREQ = atoi(mACKs[0]->line->get_field(Line_cont::F_FREQ ).c_str());
        }

        /* cout<<mAddr<<" "<<mAMPDU_mean<<" "<<mAMPDU_std<<" "<<mAMPDU_max<<" "<<mRSSI_mean<<" "<<mTime_delta<<" "<<mMPDU_num<<" "<<mFREQ<<endl; */
        

        /* Computing the data rate. */



        //TODO:  For Lixing Tue 17 Jan 2017 01:35:13 PM EST.
        //LOSS need to be addressed.
        /* mLoss_mean = */ 
}

float BlkACK_stat::getAMPDU_mean()
{
        return mAMPDU_mean;
}

uint16_t BlkACK_stat::getMPDU_num()
{
        return mMPDU_num;
}

void BlkACK_stat::setPktSize(uint16_t s)
{
        mPkt_Size = s;
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
        








