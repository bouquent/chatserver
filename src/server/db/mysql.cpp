#include "db/mysql.hpp"

using std::string;
string server = "localhost"; 
string user = "root";
string password = "chenzezheng666";
string dbname = "chat";


//初始化数据库
MySql::MySql()
{
    conn_ = mysql_init(nullptr);
}
//释放数据库资源
MySql::~MySql()
{
    if (conn_ != nullptr) {
        mysql_close(conn_);
    }
}


//连接数据库
bool MySql::connect()
{
    MYSQL *p = mysql_real_connect(conn_, server.c_str(), user.c_str(), password.c_str(),
                                  dbname.c_str(), 0, nullptr, 0);
    if (p == nullptr) {
        printf("mysql_real_connect error, [%s]", mysql_error(conn_));
    }
    else {
        // c/c++默认使用assic码进行编制，如果不设置，从数据库拉取得中文可能编程乱码
        // LOG_INFO << "connect the databases success!\n";
        mysql_query(conn_, "set names gbk");
    }
    return p;
}

//更新操作
bool MySql::update(const string &sql)
{
    //这个函数成功返回0，失败返回非0
    if (mysql_query(conn_, sql.c_str()) != 0) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!" << mysql_error(conn_);
        return false;
    }
    return true;
}

//查询操作
MYSQL_RES* MySql::query(const string &sql)
{
    if (mysql_query(conn_, sql.c_str())) {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败! " << mysql_error(conn_);
        printf("[%s]\n", mysql_error(conn_));
        return nullptr;
    }
    return mysql_store_result(conn_);
}