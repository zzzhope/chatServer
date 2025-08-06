#include "json.hpp"
#include<iostream>
#include<vector>
#include<map>
#include<string>

using json=nlohmann::json;
using namespace std;

string func1()
{
    json js;
    js["msg_type"]=2;
    js["from"]="zhang san";
    js["to"]="li si";
    js["msg"]="hello, what are you doing now?";
    string sendBuffer=js.dump();
    // cout<<sendBuffer<<endl;
    return sendBuffer;
}

string func2()
{
    json js;
    js["id"]={1,2,3,4,5};
    js["name"]="zhang san";
    js["msg"]["zhang san"]="hello world";
    js["msg"]["li si"]="hello china";
    // js["msg"]={{"zhang san","hello world"},{"li si","hello china"}};k
    return js.dump();
}

string func3()
{
    json js;
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"]=vec;

    map<int,string> m;
    m.insert(make_pair(1,"huangshan"));
    m.insert(make_pair(2,"huashan"));
    m.insert(make_pair(3,"taishan"));
    js["path"]=m;

    string sendBuffer=js.dump();

    return sendBuffer;
}

int main()
{
    // string recvbuffer=func1();
    // json jsbuffer=json::parse(recvbuffer);
    // cout<<jsbuffer["msg_type"]<<endl;
    // cout<<jsbuffer["from"]<<endl;
    // cout<<jsbuffer["to"]<<endl;
    // cout<<jsbuffer["msg"]<<endl;
    
    // string recvbuffer=func2();
    // json jsbuffer=json::parse(recvbuffer);
    // cout<<jsbuffer["id"]<<endl;
    // auto arr=jsbuffer["id"];
    // cout<<arr[2]<<endl;
    // cout<<jsbuffer["name"]<<endl;  
    // auto msg=jsbuffer["msg"];
    // cout<<msg["zhang san"]<<endl;
    // cout<<msg["li si"]<<endl;

    string recvbuffer=func3();
    json jsbuffer=json::parse(recvbuffer);

    vector<int> vec=jsbuffer["list"];
    for(auto it=vec.begin();it!=vec.end();++it)
    {
        cout<<*it<<" ";
    }
    cout<<endl;
    map<int,string> m=jsbuffer["path"];
    for(auto it=m.begin();it!=m.end();++it)
    {
        cout<<it->first<<" "<<it->second<<endl;
    }
    cout<<endl;

    return 0;
}