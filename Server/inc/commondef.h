#ifndef _COMMONDEF_H
#define _COMMONDEF_H

//以下是XHYFIleManager系统会发生的错误代码

#define ERR_FLOCKED 1000				//文件被锁住
#define ERR_FLODER_NOTEPT 1010			//(删除文件夹时)文件夹不为空
#define ERR_PATH_NOTEXT 1002			//路径不存在
#define ERR_CN_DEL_ROOT 1003			//尝试删除根目录
#define ERR_SYS_LOADFAIL 1004			//文件系统内部错误导致读失败
#define ERR_SYS_SAVEFAIL 1005			//文件系统内部错误导致写失败
#define ERR_MEM_FULL 1006				//文件系统文件已满
#define ERR_NME_CONFLICT 1007			//名字冲突
#define ERR_PATH_IS_NOT_FLODER 1008		//路径指向的不是一个文件夹
#define ERR_PATH_IS_NOT_FILE 1009		//路径指向的不是一个文件
#define ERR_PMS_DENY 1010				//权限不足拒绝操作
#define ERR_EOF 1011					//文件结束
#define ERR_NAME_OVERLENGTH 1012		//名字过长(15字节及以下)


//打开模式
#define OPTYPE_READ 0x01				//读模式
#define OPTYPE_WRITE 0x02				//写模式
//锁模式
#define OPTYPE_WTE_MTX_LOCK 0x10		//写互斥锁模式
#define OPTYPE_WTE_SHR_LOCK 0x20		//写共享锁模式
#define OPTYPE_RAD_MTX_LOCK 0x40		//读互斥锁模式
#define OPTYPE_RAD_SHR_LOCK 0x80		//写互斥锁模式

//读模式打开时,未指定加锁模式时默认锁模式是读互斥锁模式
//写模式打开时,未指定加锁模式时默认锁模式是写互斥锁模式

#endif