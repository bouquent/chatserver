#include "friendmodel.hpp"
#include "db/mysql.hpp"
#include <vector>
using std::vector;

//添加好友
void FriendModel::insert(int userid, int friendid)
{
    //组装sql语句
    char sql[SQL_MAX_LENGTH] = {0};
    sprintf(sql, "insert into Friend(userid, friendid) values(%d, %d)", userid, friendid);

    //插入好友关系
    MySql mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }

}

//用户的好友列表
vector<User> FriendModel::query(int userid)
{
    char sql[SQL_MAX_LENGTH] = {0};
    sprintf(sql, "select id, name, state  from User where id in (select friendid from Friend where userid = %d)"
                , userid);
    
    vector<User> vec;
    MySql mysql;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);                                                                   
            }
        }
        //释放结果集
        mysql_free_result(res);
    }
    return vec;
}