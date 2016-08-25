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

void Parser::start()
{
        ifstream infile(mName.c_str());
        string line;
        bool first=true;
        Window_data* t_window_data = NULL;
        double pre_Time = 0.0;
        //TODO: Add time order check!! For Lixing Wed 24 Aug 2016 11:28:12 AM EDT.
        
        while(getline(infile, line))
        {
                string line_value;
                stringstream ss(line);
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
                                t_window_data = new Window_data(mStartTime);
                                t_window_data->addData(line_c);
                        }
                        else if(mStartTime != pre_Time){
                                t_window_data->parse();
                                mWindow_datas.push_back(t_window_data);
                                t_window_data = new Window_data(mStartTime);
                                t_window_data->addData(line_c);
                        }
                        else{
                                assert(mStartTime == t_window_data->getTime());
                                t_window_data->addData(line_c);
                        }

                        pre_Time = mStartTime;
                        /* D(" Index "<<setprecision(15)<<line_c->getTime()<<" "<<getIndex(line_c)); */

                }
        }

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
                     std::cout << it->first << " => " << it->second << '\n';
}

double Line_cont::getTime()
{
        return mTime;

}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////Window_data /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

Window_data::Window_data(double time)
{
        mTime = time;
        mAPs.clear();
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
                D(( it->first )<<" has "<<( it->second )->getPacketN()<<" pkts");
                ( it->second )->calc_Airtime();
                D(( it->first )<<" has "<<( it->second )->getAirTime()<<" ms");
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
                /* D(" type subtype "<<( (*it)->get_field(Line_cont::F_TYPE_SUBTYPE))); */
                if(is_downlink(*it)){
                        string t_addr = (*it)->get_field(Line_cont::F_TA);
                        t_addr = t_addr.substr(0,ADD_LEN);
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr);
                }
                else if(is_uplink(*it)){
                        string t_addr = (*it)->get_field(Line_cont::F_RA);
                        t_addr = t_addr.substr(0,ADD_LEN);
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr);

                }
                else if(is_beacon(*it)){
                        string t_addr = (*it)->get_field(Line_cont::F_TA);
                        t_addr = t_addr.substr(0,ADD_LEN);
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr);
                }
                else{
                        string t_addr("Other");
                        if(mAPs.find(t_addr) == mAPs.end())
                                mAPs[t_addr] = new AP_stat(t_addr);

                }
                /* string t_addr = getAP(line_c); */

        }
}

void Window_data::assignData()
{
        for(vector<Line_cont*>::iterator it=mLines.begin(); it != mLines.end();++it ){
                string ta_addr = (*it)->get_field(Line_cont::F_TA);
                ta_addr = ta_addr.substr(0,ADD_LEN);
                string ra_addr = (*it)->get_field(Line_cont::F_TA);
                ra_addr = ra_addr.substr(0,ADD_LEN);
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
bool Window_data::is_downlink(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_DS) == "2" ); 
}

bool Window_data::is_uplink(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_DS) == "1" ); 
}
bool Window_data::is_beacon(Line_cont* l)
{
       return ( l->get_field(Line_cont::F_TYPE_SUBTYPE) == "8" ); 
}
/* string Window_data::getAP(Line_cont* l) */
/* { */

/* } */



////////////////////////////////////////////////////////////////////////////////////
////////////////////////////AP_Stat /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

AP_stat::AP_stat(string mac)
{
        mAddr=mac;
        mAirTime = 0;
        mPackets.clear();

}

void AP_stat::addPacket(Line_cont* l)
{
        mPackets.push_back(l);
}

float AP_stat::getAirTime()
{
        return mAirTime;
}

uint32_t AP_stat::getPacketN()
{
        return mPackets.size();
}


vector<Line_cont*>* AP_stat::getPkts()
{
        return &mPackets;
}

void AP_stat::calc_Airtime()
{

        double pre_t = 0;
        for(vector<Line_cont* >::iterator it = mPackets.begin(); it != mPackets.end();++it){
                string s_byte = (*it)->get_field(Line_cont::F_LEN);
                uint16_t t_bits = atoi(s_byte.c_str())*8;

                string s_nav = (*it)->get_field(Line_cont::F_NAV);
                double t_nav = atoi(s_nav.c_str())/(double)1000;

                string s_rate = (*it)->get_field(Line_cont::F_RATE);
                // in bits / ms
                double t_rate = atoi(s_rate.c_str())*1000;
                D(setprecision(16)<<(*it)->getTime()<<" NAV:"<<t_nav
                                <<" GAP: "<<( (*it)->getTime() - pre_t )*1000
                                <<" bit/rate: "<<t_bits/t_rate);

                mAirTime += max(min(t_nav,( (*it)->getTime() - pre_t )*1000),t_bits/t_rate);

                pre_t = (*it)->getTime();
                    
        }
}























