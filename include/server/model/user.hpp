#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// User表的ORM类
class User
{
private:
    int id;
    string name;
    string password;
    string state;

public:
    User(int _id = -1, string _name = "", string _password = "", string _state = "offline") : id(_id), name(_name), password(_password), state(_state) {}
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPwd(string password) { this->password = password; }
    void setState(string state) { this->state = state; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPwd() { return this->password; }
    string getState() { return this->state; }
};

#endif