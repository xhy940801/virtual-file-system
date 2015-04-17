#include "connector.h"
#include "FMStructs.h"
#include "commondef.h"
#include "Shell.h"
#include "functions.h"
#include "Guard.h"

#include <iostream>
#include <string>
#include <cstring>
using namespace std;


int main()
{
	class _GD
	{
	public:
		void operator () (SessionClient* session) { releaseSession(session); }
	};
	while(true)
	{
		string ipaddr;
		cout << "please input the server's ip addr" << endl;
		cin >> ipaddr;
		SessionClient* session = initSession(ipaddr.c_str());
		Guard<SessionClient*, _GD> gd(session, _GD());
		string username, password;
		int uid;
		while(true)
		{
			cout << "please input username and password" << endl;
			cin >> username >> password;
			uid = session->login(username.c_str(), password.c_str());
			if(uid)
				break;
			cout << "login fail!\n" << session->getErrno() << endl;
			releaseSession(session);
			session = initSession(ipaddr.c_str());
		}
		Shell shell(session, uid);
		string str;
		while(getline(cin, str))
		{
			if(trim(str) == "exit")
				return 0;
			if(trim(str) == "logout")
				break;
			shell.parse(str);
			std::cout << "user: " << shell.getUid() << " : " << shell.getBasePath() << "$ " << std::flush;
		}
	}
	
}