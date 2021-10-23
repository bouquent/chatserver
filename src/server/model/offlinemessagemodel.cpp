#include "offlinemessagemodel.hpp"
#include "db/mysql.hpp"

using std::string;
using std::vector;

void OfflineMessage::insert(int userid, string msg)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "insert into offlineMessage(userid, message) values(%d, '%s')", userid, msg.c_str());

    MySql mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}

void OfflineMessage::remove(int userid)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "delete from offlineMessage where userid = %d", userid);

    MySql mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}

vector<string> OfflineMessage::query(int userid)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "select message from offlineMessage where userid = %d", userid);

    vector<string> ret;
    MySql mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                ret.push_back(row[0]);
            }
        }
        //释放结果集
        mysql_free_result(res); 
    }
    return ret;
}