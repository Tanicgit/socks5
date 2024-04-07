/**
构造函数完成了文件的解析,并把内容保存在map中
析构时会重新把map写回文件
API只实时针对map修改
意味着:多线程进程操作不加锁可能会有意外的结果,即不可多线程操作同一文件

支持#开头的注释
*/
#include "tConfigFile.h"

//#define LOG(a) cout << (a) <<endl
#define LOG(a) 


/*
str_is_file: ture,str是文件名;false,str否则是字符串
*/
tConfigFile::tConfigFile(string str,int mode,bool str_is_file)
{
	
	file_change = false;
    coinfig_is_file = str_is_file;
	configs.clear();
	fmode = mode;
	if(str_is_file)
	{
        fileName =  str;
		ParseFile();
	}
	else
	{        
        strBuf = str;             
		ParseString();
	}
}

tConfigFile::~tConfigFile()
{
	if(file_change&&coinfig_is_file&&fmode==TF_READWRITE)
	{
		file_change = false;
		SaveToFile();
	}
}

string &tConfigFile::trim(string &str,const string &chars=" \r\n\t")
{
        string map(0xFF, 0); 
        for (unsigned int i=0;i<chars.size();i++) 
        { 
                map[chars[i]] = 1; 
        } 
        while( str.size() && map.at(str[str.size()-1])) str.erase(str.end()-1); 
        while( str.size() && map.at(str[0])) str.erase(0,1); 
        return str;
}

bool tConfigFile::Parse(basic_istream<char> &ss)
{
        configs.clear();
        list<tCF_KeyValue> l_kv; 
		tCF_KeyValue kv;
		tCF_ConfKv  conf;
		
		string name;
        string line;
        while(getline(ss,line))
        {
                LOG("#line=#" + line + "#");
                if(line.empty())
                        continue;
                
                if(line[0]=='[')
                {
                	size_t  a =  line.find(']');
						
                	if(a != string::npos)
                	{
						if(!name.empty())
					   {	
					   		conf.key = name;
							conf.value = l_kv;
							configs.push_back(conf);	
					   }
					   
					   name.clear();
					   l_kv.clear();
					   conf.value.clear();

					   string tmp = line.substr(1,a-1);
					   trim(tmp);
					   if(!tmp.empty())
					  	{
						   name = tmp;
						  
						   LOG("-----------");
						   LOG("name=[" + name + "]");
					   }
					   else
					   {
							 //[] 为空的行丢弃
					   }

					}    
					else
					{
						// 单[开头的行直接丢弃
						
					}
                }
                else
                {       
                        if(!name.empty())
                        {    
                                                                                   
                            size_t a = line.find('=');
                            if(a != string::npos)
                            {
                                   
                                    kv.key = line.substr(0,a);
                                    kv.value = line.substr(a+1);
                                    trim(kv.key);
                                    trim(kv.value);
                                    if(!kv.key.empty())
                                    {
                                        l_kv.push_back(kv);
                                    }  
                            }  
							else
							{
								kv.key = line;
                                kv.value = "";
                                trim(kv.key);
                                trim(kv.value);
                                if(!kv.key.empty())
                                {
                                    l_kv.push_back(kv);
                                }  
							}
                        }
                }
        } 

        if(!name.empty())
        {
                conf.key = name;
				conf.value = l_kv;
				configs.push_back(conf);       
        }
        LOG("-----------");
        return true;
}

bool tConfigFile::ParseString()
{
        istringstream ss(strBuf);
        return Parse(ss);
}

bool tConfigFile::ParseFile()
{
        LOG("Parse:");
        ifstream ifs;
        ifs.open(fileName.c_str()) ;
        if (!ifs.is_open()) {
                LOG("open " + fileName + " err");
                return false;
        }
        return Parse(ifs);
}

bool tConfigFile::SaveToFile()
{

        LOG("SaveToFile");
        ofstream ofs;
        ofs.open(fileName.c_str());
		if (!ofs.is_open()) {
			LOG("open " + fileName + " err");
		            return false;
		} 

        auto itr = configs.begin();
        for(;itr!=configs.end();itr++)
        {
                ofs << endl << "[" << itr->key << "]" << endl; 
                LOG("[" + itr->key + "]"); 
                auto itr1=itr->value.begin();
                for(;itr1!=itr->value.end();itr1++)
                {      
                		if(itr1->value.empty())
                		{
                			ofs << itr1->key << endl;  
                		}
						else
						{
                        	ofs << itr1->key << "=" << itr1->value << endl;  
						}
                        LOG(itr1->key + "=" + itr1->value);           
                }

        }
        ofs.close();
        return true;
}

/*
没有则创建,有则替换
[config_name]
key=value
*/
bool tConfigFile::set(string config_name,string key,string value)
{
	list<tCF_KeyValue> l_kv; 
	tCF_KeyValue kv;
	tCF_ConfKv	conf;
	

    if(config_name.empty() || key.empty())
    {
            return false;
    }


	auto itr = configs.begin();
	for(;itr!=configs.end();itr++)
	{
		if(itr->key == config_name)
		{
			break;
		}
	}

	if(itr==configs.end())
	{
		kv.key=key;
		kv.value=value;
		l_kv.push_back(kv);

		conf.key = config_name;
		conf.value = l_kv;

		configs.push_back(conf);
		file_change = true;
		return true;
	}

	
	auto itr1=itr->value.begin();
    for(;itr1!=itr->value.end();itr1++)
    {
		if(itr1->key == key)
		{
			break;
		}
	}

	if(itr1==itr->value.end())
	{
		kv.key=key;
		kv.value=value;

		itr->value.push_back(kv);
		file_change = true;
		return true;
	}


	itr1->value = value;

	file_change = true;
	return true;
}

/*
查找map中的如下结构,返回value
[config_name]
key=value
*/
bool tConfigFile::get(string config_name,string key,string &value)
{
 	list<tCF_KeyValue> l_kv; 
	tCF_KeyValue kv;
	tCF_ConfKv	conf;
	

    if(config_name.empty() || key.empty())
    {
            return false;
    }  


	auto itr = configs.begin();
	for(;itr!=configs.end();itr++)
	{
		if(itr->key == config_name)
		{
			break;
		}
	}
	if(itr==configs.end())
	{
		return false;
	}


	auto itr1=itr->value.begin();
    for(;itr1!=itr->value.end();itr1++)
    {
		if(itr1->key == key)
		{
			break;
		}
	}

	if(itr1==itr->value.end())
	{
		return false;
	}

	value = itr1->value;
	return true;
}


bool tConfigFile::del(string config_name)
{
	if(configs.empty())
		return false;

	auto itr = configs.begin();
	for(;itr!=configs.end();itr++)
	{
		if(itr->key == config_name)
		{
			break;
		}
	}
	if(itr==configs.end())
	{
		return false;
	}


	configs.erase(itr);
	return true;
}
bool tConfigFile::del(string config_name,string key)
{
	if(configs.empty())
		return false;

	auto itr = configs.begin();
	for(;itr!=configs.end();itr++)
	{
		if(itr->key == config_name)
		{
			break;
		}
	}
	if(itr==configs.end())
	{
		return false;
	}

	auto itr1=itr->value.begin();
    for(;itr1!=itr->value.end();itr1++)
    {
		if(itr1->key == key)
		{
			break;
		}
	}

	if(itr1==itr->value.end())
	{
		return false;
	}


	itr->value.erase(itr1);
	return true;
}


void tConfigFile::fileShow()
{
        auto itr = configs.begin();
		
		for(;itr!=configs.end();itr++)
		{
			configShow(*itr);
		}
}

void tConfigFile::configShow(tCF_ConfKv &conf)
{
		cout << "["<<conf.key << "]"<< endl;
		auto itr = conf.value.begin();
		for(;itr!=conf.value.end();itr++)
		{
			keyShow(*itr);
		}
}

void tConfigFile::keyShow(tCF_KeyValue &kv)
{	
		cout <<kv.key << " = " <<kv.value << endl;	
}



