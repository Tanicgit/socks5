#ifndef __T_CONFIG_FILE2_H
#define __T_CONFIG_FILE2_H

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
#注释
#注释
key3=value3
[config2]
#注释
key5=value5
key6=value6
bbb=ccc

*/
#define   TF_READONLY	1
#define   TF_READWRITE	2

typedef struct
{
    string key;
    string value;
}tCF_KeyValue;

typedef struct
{
    string key;
    list<tCF_KeyValue> value;
}tCF_ConfKv;

class tConfigFile
{
private:
    /* data */
	bool file_change;
    bool coinfig_is_file;
    string fileName;
    string strBuf;
	int fmode;

    list<tCF_ConfKv> configs;//<confName,<key,value>>
	
    bool Parse(basic_istream<char> &ss);
    bool ParseFile();
	bool ParseString();	

    string &trim(string &str,const string &chars);
	void configShow(tCF_ConfKv &itr_config);
	void keyShow(tCF_KeyValue &itr_key);	
public:
	tConfigFile(string str,int mode=TF_READONLY,bool str_is_file=true);
    ~tConfigFile();
	
	bool set(string config_name,string key,string value);
	bool get(string config_name,string key,string &value);

	bool del(string config_name);
	bool del(string config_name,string key);

	const  list<tCF_ConfKv>& getConfigs()
	{
		return configs;
	}
	
    void fileShow();
	bool SaveToFile();
};
void testConfigDemo();



#endif