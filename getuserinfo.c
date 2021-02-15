#include<stdio.h>
#include <pwd.h>
#include <sys/types.h>
int main(int argc, char** argv)
{
	struct passwd *user;
	
    if( argc != 2){
        printf("usage: ./getuserinfo <username>\n");
        return -1;
    }
	user = getpwnam(argv[1]);
	if(user)	{
		printf("name:%s\n", user->pw_name);
		printf("uid:%d\n", user->pw_uid);
		printf("home:%s\n", user->pw_dir);
	}
	else
		printf("get user:%s info error\n", argv[1]);
}
