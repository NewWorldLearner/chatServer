#include "../include/json.hpp"
#include <iostream>
#include <vector>

using json = nlohmann::json;

using namespace std;

// 构建json对象并测试
string test1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "mengmeng";
    js["to"] = "shy";
    js["msg"] = "I miss you!";

    // 输出一个json对象
    cout << js << endl;

    // 将json对象转换为字符串，方便进行网络传输
    string sendBuf = js.dump();
    cout << sendBuf.c_str() << endl;

    return sendBuf;
}

// 在json对象中嵌套数组，在对象中嵌套对象并测试
void test2()
{
    json js;

    // id为key，数组作为value
    js["id"] = {1, 2, 3, 4, 5};

    js["name"] = "mengmeng";

    // msg作为key，vaule是个对象
    js["msg"]["mengmeng"] = "I miss you!";
    js["msg"]["shy"] = "I miss you too!";

    // 上面的两句等价于，这里我感觉很奇怪，为什么不使用:而使用,呢？
    // 在C++中，{{key1, value1}, {key2, value2}}会被解析为包含多个键值对的初始化列表。
    js["msg"] = {{"mengmeng", "I miss you!"}, {"shy", "I miss you too!"}};

    cout << js << endl;
}

// 将list和map作为json的value
void test3()
{
    json js;
    vector<int> vec;
    vec.push_back(5);
    vec.push_back(2);
    vec.push_back(0);

    js["list"] = vec;

    map<int, string> m;

    m.insert({520, "mengmeng"});
    m.insert({1314, "shy"});

    js["name"] = m;

    cout << js << endl;
}


// json反序列化
void test4()
{
    string recv = test1();
    json js = json::parse(recv);

    cout << js << endl;
}

int main()
{
    test1();
    test2();
    test3();
    test4();
}