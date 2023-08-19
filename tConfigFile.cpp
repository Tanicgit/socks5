#include "tConfigFile.h"

//#define LOG(a) cout << (a) <<endl
#define LOG(a) 


/*
str_is_file: ture,str是文件名;false,str否则是字符串
*/
tConfigFile::tConfigFile(string str,bool readOnly,bool str_is_file)
{
	
	file_change = false;
    coinfig_is_file = str_is_file;
	configs.clear();
	ro = readOnly;
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
	if(file_change&&coinfig_is_file&&(!ro))
	{
		file_change = false;
		SaveToFile();
	}
}

string &tConfigFile::trim(string &str,const string &chars=" \r\t")
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
        map<string,ValueInfo> config;
        string name;
        ValueInfo vi;

        string line;
        while(getline(ss,line))
        {
                LOG("#line=#" + line + "#");
                if(line.empty())
                        continue;
                
                if(line[0]=='[')
                {
                	unsigned int  a =  line.find(']');
						
                	if(a != string::npos)
                	{
						if(!name.empty())
					   {
							   configs[name] = config;
					   }
					   
					   name.clear();
					   config.clear();

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
                              
                                if(line[0]=='#')
                                {   
                                      vi.info.push_back(line.substr(1)); 
                                      LOG("info=[" + line.substr(1) + "]");                           
                                }
                                else                        
                                {
                                        unsigned int a = line.find('=');
                                        if(a != string::npos)
                                        {
                                               
                                                string key = line.substr(0,a);
                                                string value = line.substr(a+1);
                                                trim(key);
                                                trim(value);
                                                if(!key.empty())
                                                {
                                                        vi.value = value;
                                                        config[key] = vi;
                                                        LOG("name=[" + key + "]");
                                                        LOG("value=[" + value + "]");

                                                        vi.info.clear();
                                                        vi.value.clear();
                                                }  
                                        }
                                         
                                }   
                        }
                }
        } 

        if(!name.empty())
        {
                configs[name] = config;        
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

        map<string,map<string,ValueInfo> >::iterator itr;
        for(itr=configs.begin();itr!=configs.end();itr++)
        {
                ofs << endl << "[" << itr->first << "]" << endl; 
                LOG("[" + itr->first + "]"); 
                map<string,ValueInfo>::iterator itr1;
                for(itr1=itr->second.begin();itr1!=itr->second.end();itr1++)
                {
                        if(!itr1->second.info.empty())
                        {
                                for(unsigned int i=0;i<itr1->second.info.size();i++)
                                {
                                        ofs << "#" << itr1->second.info[i] << endl; 
                                        LOG(itr1->second.info[i]);
                                }                      
                        }
                        ofs << itr1->first << "=" << itr1->second.value << endl;  
                        LOG(itr1->first + "=" + itr1->second.value);           
                }

        }
        ofs.close();
        return true;
}


/*
没有则创建,有则替换
[config_name]
#info[0]
#info[.]
#info[n]
key=value
*/
bool tConfigFile::set(string config_name,string key,string value,vector<string> &info)
{
        if(config_name.empty() || key.empty())
        {
                return false;
        }
		
		if(configs.empty())
		{
			ValueInfo vi; 
            vi.info = info;
            vi.value = value;  
            map<string,ValueInfo> block;
            block[key] = vi;
            configs[config_name]  = block;
			file_change = true;
			return true;
		}
		
        map<string,map<string,ValueInfo> >::iterator itr =  configs.find(config_name);
        if(itr==configs.end())
        {
                ValueInfo vi; 
                vi.info = info;
                vi.value = value;  
                map<string,ValueInfo> block;
                block[key] = vi;
                configs[config_name]  = block;
        }
        else
        {    
                ValueInfo vi;
                vi.info = info;
                vi.value = value;

				if(itr->second.empty())
				{
					itr->second[key] = vi; 
				}
				else
				{
	                map<string,ValueInfo>::iterator itr1=itr->second.find(key);
	                if(itr1==itr->second.end())
	                {
	                        itr->second[key] = vi; 
	                }
	                else
	                {
	                        itr1->second = vi;    
	                }
				}
        }  
		file_change = true;
		return true;
}

/*
没有则创建,有则替换
[config_name]
key=value
*/
bool tConfigFile::set(string config_name,string key,string value)
{
        if(config_name.empty() || key.empty())
        {
                return false;
        }

		if(configs.empty())
		{
			ValueInfo vi; 
            vi.value = value;  
            map<string,ValueInfo> block;
            block[key] = vi;
            configs[config_name]  = block;
			file_change = true;
			return true;
		}
		
        map<string,map<string,ValueInfo> >::iterator itr =  configs.find(config_name);
        if(itr==configs.end())
        {
                ValueInfo vi; 
                vi.value = value;  
                map<string,ValueInfo> block;
                block[key] = vi;
                configs[config_name]  = block;
        }
        else
        {    
                ValueInfo vi;
                vi.value = value;
				if(itr->second.empty())
				{
					itr->second[key] = vi; 
				}
				else
				{
	                map<string,ValueInfo>::iterator itr1=itr->second.find(key);
	                if(itr1==itr->second.end())
	                {
	                        itr->second[key] = vi; 
	                }
	                else
	                {
	                        itr1->second.value = value;     
	                }
				}
        }  
		file_change = true;
		return true;
}

/*
&cc : 对应config
*/
bool tConfigFile::findConfig(string config_name,list<tCF_KeyValue>	  &cc)
{
		map<string,ValueInfo> empty;
		if(config_name.empty())
		{
				return false;
		}
		
		if(configs.empty())
			return false;
		
		map<string,map<string,ValueInfo> >::iterator itr_config = configs.find(config_name);
		if(itr_config==configs.end())
		{
				return false;
		}

		map<string,ValueInfo>::iterator  it=itr_config->second.begin();
		for(;it!=itr_config->second.end();it++)
		{
			tCF_KeyValue kv;
			kv.key = it->first;
			kv.value = it->second.value;
			cc.push_back(kv);
		}
		return true;
}

/*
tongguo   key-value 找到一个config,目前找到第一个就退出,后续会把符合的都找出
*/
bool tConfigFile::findConfigByKeyValue(string key,string value,string &config_name)
{
    if(key.empty())
    {
            return false;
    }	

		
    map<string,map<string,ValueInfo> >::iterator itr =  configs.begin();

	for(;itr != configs.end();itr++)
	{
		 map<string,ValueInfo>::iterator itr1=itr->second.find(key);
		 if(itr1 != itr->second.end())
		 {
			if(itr1->second.value == value)
			{
				config_name = itr->first;
				return true;
			}
		 }
	}
	return false;	
}


/*
&itr_key:对应config中对应key-value 迭代器
*/
bool tConfigFile::findConfigKey(string block_name,string key,map<string,ValueInfo>::iterator &itr_key)
{
       ValueInfo empty;
        if(configs.empty())
			return false;
        map<string,map<string,ValueInfo> >::iterator itr = configs.find(block_name);
        if(itr==configs.end())
        {
                return false;
        }
		if(itr->second.empty())
			return false;
        itr_key=itr->second.find(key);
        if(itr_key==itr->second.end())
        {
                return false;
        }
        return true;;
}

/*
查找map中的如下结构,返回value
[config_name]
key=value
*/
bool tConfigFile::get(string config_name,string key,string &value)
{
       ValueInfo empty;
		if(configs.empty())
			return false;
		
        map<string,map<string,ValueInfo> >::iterator itr = configs.find(config_name);
        if(itr==configs.end())
        {
        	return false;
        }
		if(itr->second.empty())
			return false;
		
        map<string,ValueInfo>::iterator itr_key=itr->second.find(key);
        if(itr_key==itr->second.end())
        {
                return false;
        }
		value = itr_key->second.value;
        return true;
}


/*
查找map中的如下结构,返回info
[config_name]
#
key=value
*/
bool tConfigFile::get(string config_name,string key,string value,vector<string> &info)
{
	ValueInfo empty;

	if(configs.empty())
		return false;
	
	map<string,map<string,ValueInfo> >::iterator itr = configs.find(config_name);
	if(itr==configs.end())
	{
	        return false;
	}

	if(itr->second.empty())
		return false;
	
	map<string,ValueInfo>::iterator itr_key=itr->second.find(key);
	if(itr_key==itr->second.end())
	{
	        return false;
	}
	value = itr_key->second.value;
	info = itr_key->second.info;
	return true;;	
}


bool tConfigFile::del(string config_name)
{
	if(configs.empty())
		return false;

	map<string,map<string,ValueInfo> >::iterator itr = configs.find(config_name);
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

	map<string,map<string,ValueInfo> >::iterator itr = configs.find(config_name);
	if(itr==configs.end())
	{
	        return false;
	}

	if(itr->second.empty())
		return false;
	
	map<string,ValueInfo>::iterator itr_key=itr->second.find(key);
	if(itr_key==itr->second.end())
	{
	        return false;
	}

	itr->second.erase(itr_key);
	return true;
}


void tConfigFile::fileShow()
{
        map<string,map<string,ValueInfo> >::iterator itr=configs.begin();
        for(;itr!=configs.end();itr++)
        {
            configShow(itr);
        }
}

void tConfigFile::configShow(map<string,map<string,ValueInfo> >::iterator itr_config)
{
		cout << "["<<itr_config->first << "]"<< endl;
		map<string,ValueInfo>::iterator itr_key;
		for(itr_key=itr_config->second.begin();itr_key!=itr_config->second.end();itr_key++)
		{
			keyShow(itr_key);
		}
}

void tConfigFile::keyShow(map<string,ValueInfo>::iterator &itr_key)
{
	
		for(unsigned int i=0;i<itr_key->second.info.size();i++)
		{
			cout << "#" <<itr_key->second.info[i] << endl;
		}
		cout <<itr_key->first << " = " <<itr_key->second.value << endl;	
}



void testConfigDemo()
{
	tConfigFile cc("new-config.txt");
	cc.set("config1","key1","abc");

	string value1;
	cc.get("config1","key1",value1);
	cout << "value1=" << value1 << endl;
}



