class Time_config
{
private: 
    Time_config() {}
    Time_config( const Time_config&);  
    Time_config& operator=( Time_config& );
    unsigned long pd[10] = {50000,20000,10000,20000,10,10,10,10,10,10};
    unsigned long sc[2] = {100000,10000}; 
public:
    static Time_config& getInstance() {
        static Time_config  instance;
        return instance;
    }
    unsigned long GetTimePd(int num) { return pd[num]; }
     unsigned long GetTime(int num) { return pd[num]; }
       
}; 
