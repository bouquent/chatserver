#include "usermodel.hpp"
#include "db/mysql.hpp"
#include <iostream>

bool UserModel::insert(User& user)
{
    // 组装sql语句
    char sql[SQL_MAX_LENGTH] = {0};
    sprintf(sql, "insert into User(name, password, state) values('%s', '%s', '%s')"
            , user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    
    //初始化并连接数据库，执行插入语句
    MySql mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//根据
User UserModel::query(int id)
{
     // 组装sql语句
    char sql[SQL_MAX_LENGTH] = {0};
    sprintf(sql, "select * from User where id = %d", id);
    
    //初始化并连接数据库，执行查询语句
    MySql mysql;
    if (mysql.connect()) {
        MYSQL_RES *res;
        if (res = mysql.query(sql)) {
           MYSQL_ROW row = mysql_fetch_row(res);
           if (row != nullptr) {
               User user;
               user.setId(atoi(row[0]));
               user.setName(row[1]);
               user.setPassword(row[2]);
               user.setState(row[3]);

               mysql_free_result(res);  //mysql需要释放查询结果
               return user;
           }
        } 
    }
    return User(); //如果查询有错误，则返回空User
}

bool UserModel::updateState(User& user)
{
    int id = user.getId();
    // 组装sql语句
    char sql[SQL_MAX_LENGTH] = {0};
    sprintf(sql, "update User set state = '%s'  where id = %d"
            , user.getState().c_str() , user.getId());
    
    //初始化并连接数据库，执行插入语句
    MySql mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }
    return false;
}

void UserModel::resetState()
{
    // 组装sql语句
    char sql[SQL_MAX_LENGTH] = {0};
    sprintf(sql, "update User set state = 'offline'  where state = 'online'");
    
    //初始化并连接数据库，执行插入语句
    MySql mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}