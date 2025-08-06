#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

// Group表的ORM类
class Group
{
private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;

public:
    Group(int _id = -1, string _name = "", string _desc = "") : id(_id), name(_name), desc(_desc) {}
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser> &getUsers() { return this->users; }
};

#endif