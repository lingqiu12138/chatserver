#ifndef USER_H
#define USER_H

#include <string>

// User表的ORM类
class User {
public:
    User(int id = -1, std::string name = "", std::string pwd = "", std::string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->passward = pwd;
        this->state = state;
    }

    void setId(int id) {this->id = id;}
    void setName(std::string name) {this->name = name;}
    void setPwd(std::string pwd) {this->passward = pwd;}
    void setState(std::string state) {this->state = state;}

    int getId() {return this->id;}
    std::string getName() {return this->name;}
    std::string getPwd() {return this->passward;}
    std::string getState() {return this->state;}

protected:
    int id;
    std::string name;
    std::string passward;
    std::string state;
};

#endif