#include "parser.h"

using namespace std;
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
                D(( it->first )<<" has "<<( it->second )->getAirTime()<<" ms");
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

                        mBlkACKs[t_b->addr]->parse_AMPDU();

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
        this->getBAMPDU_stat();// This must be after calculate the blk-based aritime, since the AMPDU len
        // is provided when calculating airtime
}

void AP_stat::report()
{
        //format:
        //time AP_address packet_number unweighted_airtime, weighted_airtime, AMPDU_num,
        //block_ack_num regular_ack_num
        if(REPORT_LEVEL>0){
                printf("%4d %10.6f %-16s %4d %10.3f %4d %10.3f %4d %4d %4d %4d %4d %4d\n",
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



void AP_stat::getBAMPDU_stat(){
        if(mBlkACKs.size()>0){
                float sum=0;
                uint16_t n_ampdu = 0;
                for(map<string, BlkACK_stat*>::iterator it = mBlkACKs.begin();it!=mBlkACKs.end();++it){
                        for(vector<uint16_t>::iterator iit = ( it->second ->mBAMPDUs ).begin();iit!=( it->second->mBAMPDUs ).end();++iit){
                                cout<<it->first<<" "<<*iit<<endl;
                                sum += *iit;     
                                n_ampdu++;
                        }
                }
                mB_nAMPDU=sum;
                mB_AMPDU_len_mean=sum/(float)n_ampdu;
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
        string t_addr = l->get_field(Line_cont::F_TA);
        t_addr += l->get_field(Line_cont::F_RA);
        /* double t_time = atof( ( l->get_field(Line_cont::F_TIME) ).c_str() ); */
        uint16_t t_ssn = atoi( ( l->get_field(Line_cont::F_BLKACK_SSN) ).c_str() );
        istringstream ss(l->get_field(Line_cont::F_BLKACK_BM));
        string byte;
        uint8_t index=0;
        
        this->addr = t_addr;
        this->line = l;
        this->SSN = t_ssn;
        this->Miss.clear();
        while(getline(ss, byte, ':'))
        {
                unsigned long tt=strtoul(byte.c_str(),NULL,16);
                for( int j=0;j<8;j++ ){
                    if(!(unsigned char)( tt >> j )  & ( 0x1 ))
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
        mBAMPDUs.clear();
}
void BlkACK_stat::addACK(BlkACK* b)
{
        mACKs.push_back(b);
}
string BlkACK_stat::getAddr()
{
       return mAddr; 
}
                
void BlkACK_stat::parse_AMPDU()
{
        uint16_t t_len=0;
        if(mACKs.size()>1){
                //The Blk ACK vector has more than 1 acks, 
                //then we need consider the relationship between the current one and previous one
                if(mACKs.back()->Miss.size()>0){
                        //Current blk ack indicates loss
                        vector<uint16_t> t_diff;
                        t_diff = set_diff(mACKs[mACKs.size()-2]->Miss,mACKs.back()->Miss);
                        t_len = t_diff.size();  
                        if(mACKs[mACKs.size()-2]->Miss.size() < 1){
                                mLoss.push_back(mACKs.back()->Miss.size());
                        }

                }
                else{
                        //no loss
                        if(mACKs.back()->SSN < mACKs[mACKs.size()-2]->SSN){
                                t_len = mACKs.back()->SSN + 4096 - mACKs[mACKs.size()-2]->SSN + mACKs[mACKs.size()-2]->Miss.size();
                        }else{
                                t_len = mACKs.back()->SSN - mACKs[mACKs.size()-2]->SSN + mACKs[mACKs.size()-2]->Miss.size();
                        }
                }

        }
        else if (mACKs.size()>0){
                //The blk ack vector only has one ack, we have to take a guess
                if(mACKs.back()->Miss.size()>0){
                        //if loss, there is no way to approximate, so just take as none
                        t_len = 0;
                }
                        
        }
        else{
                //if no ack
                t_len = 0;

        }

        mBAMPDUs.push_back(t_len);

        /* if(t_len > 0){ */
        /*         //add the AMPDU information for AP_stat to report */
        /*         mAP_stat->addBAMPDU(t_len); */
        /*         return t_len; */
        /* } */
        /* else{ */
        /*         return 0; */
        /* } */
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
vector<uint16_t> BlkACK_stat::set_diff(vector<uint16_t>& a,vector<uint16_t>& b)
{
        //By calculating the set diff of two consecutive ACKs, we can get which 
        std::vector<uint16_t> v;                      
        /* cout<<" calculate set_diff a.size() "<<a.size() */
        /*         <<" b.size() "<<b.size()<<endl; */
        std::vector<uint16_t>::iterator it=b.begin();
        vector<uint16_t>::iterator t=a.begin();
        while( ( t!=a.end() ) && ( it != b.end() )){
                if(*t > *it){
                        ++it;
                }
                else if(*it > *t){
                        ++t;
                }
                else{
                        v.push_back(*t);
                        ++t;
                        ++it;
                }
        }
        vector<uint16_t> v1(a.size());
        it = std::set_difference(a.begin(),a.end(),v.begin(),v.end(),v1.begin());
        v1.resize(it - v1.begin());
        return v1;
}
        








