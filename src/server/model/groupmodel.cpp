#include "groupmodel.hpp"
#include "db/mysql.hpp"

//创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s', '%s')"
                , group.getGroupName().c_str(), group.getGroupDesc().c_str());
    
    MySql mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            group.setGroupid(mysql_insert_id(mysql.getConnection())); 
            //mysql_insert_id  取得上一步insert操作产生的id
            return true;
        }
    }
    return false;
}

//加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "insert into GroupUser(groupid, userid, grouprole) values(%d, %d, '%s')", 
                groupid, userid, role.c_str());

    MySql mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}

//查询用户的群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "select a.groupid, a.groupname, a.groupdesc from  \
                  AllGroup a inner join GroupUser b on a.groupid = b.groupid \
                  where b.userid = %d",  userid);

    MySql mysql;
    //添加群组信息
    vector<Group> vec;     
    if(mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                Group group;
                group.setGroupid(atoi(row[0]));
                group.setGroupName(row[1]);
                group.setGroupDesc(row[2]);
                vec.push_back(group);
            }
        }
        mysql_free_result(res); //释放结果集
    } else {  //如果没有连接上返回空数组
        return {};
    }

    //为每个群组添加它的所有用户信息
    for (auto &group : vec) {
        sprintf(sql, "select a.id, a.name, a.state, b.grouprole   from User a inner join GroupUser b \
                    on a.id = b.userid where groupid = %d", group.getId());
        MYSQL_RES *res;
        res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                GroupUser user;
                user.setId(atoi(row[1]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }

    }
    return vec;
}



//根据指定的groupid查找群组用户id列表(除自己)
vector<int> GroupModel::queryGroups(int userid, int groupid)
{
    char sql[SQL_MAX_LENGTH];
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", groupid, userid);

    MySql mysql;
    vector<int> vec;
    if (mysql.connect()) {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while (row = mysql_fetch_row(res)) {
                vec.push_back(atoi(row[0]));                
            }
        }
        mysql_free_result(res);
    }
    return vec;
}

