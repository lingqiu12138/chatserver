#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>

// json序列化示例1
std::string func1() {
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    std::string sendBuf = js.dump();
    // std::cout << sendBuf.c_str() <<std::endl;
    return sendBuf;
}

// json序列化示例2
std::string func2() {
    json js;
    // 添加数组
    js["id"] = {1, 2, 3, 4, 5};
    // 添加key-value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}};
    // std::cout << js << std::endl;
    return js.dump();
}

// json序列化示例代码3
std::string func3() {
    json js;

    // 直接序列化一个vector容器
    std::vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    js["list"] = vec;

    // 直接序列化一个map容器
    std::map<int, std::string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});

    js["path"] = m;

    std::string sendBuf = js.dump(); // json数据对象 =》序列化 json字符串
    // std::cout << sendBuf <<std::endl;
    return sendBuf;
}

int main() {
    std::string recvBuf = func3();
    // 数据的反序列化   json字符串 =》反序列化 数据对象（看作容器，方便访问）
    json jsbuf = json::parse(recvBuf);
    // std::cout << jsbuf["msg_type"] << std::endl;
    // std::cout << jsbuf["from"] << std::endl;
    // std::cout << jsbuf["to"] << std::endl;
    // std::cout << jsbuf["msg"] << std::endl;

    // std::cout << jsbuf["id"] << std::endl;
    // auto arr = jsbuf["id"];
    // std::cout << arr[2] << std::endl;

    // auto msgjs = jsbuf["msg"];
    // std::cout << msgjs["zhang san"] << std::endl;
    // std::cout << msgjs["liu shuo"] << std::endl;

    std::vector<int> vec = jsbuf["list"];
    for (int &v : vec) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    std::map<int, std::string> mymap = jsbuf["path"];
    for (auto &p : mymap) {
        std::cout << p.first << " " << p.second << std::endl;
    }
    std::cout << std::endl;

    return 0;
}