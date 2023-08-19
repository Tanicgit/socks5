#ifndef __T_CONFIG_FILE_H
#define __T_CONFIG_FILE_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <list>
using namespace std;
/*
[config1]
key1=value1
key1=value2
key3=value3

[config2]
key1=value1
key2=value2
#×¢ÊÍ
#×¢ÊÍ
key3=value3
[config2]
#×¢ÊÍ
key5=value5
key6=value6
bbb=ccc

*/
typedef struct
{
    vector<string> info;
    string value;
}ValueInfo;
typedef struct
{
    string key;
    string value;
}tCF_KeyValue;
class tConfigFile
{
private:
    /* data */
	bool file_change;
    bool coinfig_is_file;
	bool ro;
    string fileName;
    string strBuf;
    map<string,map<string,ValueInfo> > configs;//<name,<key,value>>
    bool Parse(basic_istream<char> &ss);
    bool ParseFile();
	bool ParseString();	
    bool findConfigKey(string config_name,string key,map<string,ValueInfo>::iterator &itr_key);
	bool getConfig(string config_name,map<string,ValueInfo> &config);
	
	
    string &trim(string &str,const string &chars);
	void configShow(map<string,map<string,ValueInfo> >::iterator itr_config);
	void keyShow(map<string,ValueInfo>::iterator &itr_key);	
public:
	tConfigFile(string str,bool readOnly=true,bool str_is_file=true);
    ~tConfigFile();

	bool set(string config_name,string key,string value,vector<string> &info);
	bool get(string config_name,string key,string value,vector<string> &info);
	
	bool set(string config_name,string key,string value);
	bool get(string config_name,string key,string &value);

	bool del(string config_name);
	bool del(string config_name,string key);

	bool findConfigByKeyValue(string key,string value,string &config_name);

	bool findConfig(string config_name,list<tCF_KeyValue>	  &cc);
    void fileShow();
	bool SaveToFile();
};
void testConfigDemo();



#endif

