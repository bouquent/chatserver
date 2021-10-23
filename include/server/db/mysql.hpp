#ifndef MYSQL_H
#define MYSQL_H
#include <string>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>
using std::string;
#define SQL_MAX_LENGTH 1024  //sql语句的最大长度

class MySql
{
public:
    //初始化数据库
    MySql();
    //释放数据库资源
    ~MySql();
    
    //连接数据库
    bool connect();

    //更新操作
    bool update(const string& sql);

    //查询操作
    MYSQL_RES* query(const string& sql);

    MYSQL* getConnection() {return conn_;}
private:
    MYSQL *conn_;
};

#endif