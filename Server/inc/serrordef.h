#ifndef _SERRORDEF_H
#define _SERRORDEF_H

//SessionServer层的错误码

#define ERR_SOCKET_EXCEPTION 100		//socket连接错误
#define ERR_VALIDATE_HEAD 101			//校验头不正确
#define ERR_SYS_ERROR 102				//系统错误
#define ERR_OPERATOR_DENY 103			//操作被拒绝
#define ERR_CMDERR 104					//命令错误(无效的命令)
#define ERR_FEAD_FILE_EOF 105			//socket流已经到末尾
#define ERR_AUTH_ERR 106				//权限错误
#define ERR_USER_HAS_EXISTED 107		//添加用户时用户名已经存在
#define ERR_UNAME_OR_PSD_ERROR 108		//登陆时用户名或者密码错误

#endif