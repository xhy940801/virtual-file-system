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
		SessionClient* session = initSession();
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
			session = initSession();
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