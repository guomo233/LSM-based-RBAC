#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define USER2ROLE_PATH "/etc/GomoLSM/user2role"
#define ROLES_PATH "/etc/GomoLSM/roles"
#define CONTROL_PATH "/etc/GomoLSM/control"
#define MAX_ROLENAME 20
#define PERMISSION_COUNT 2 //inode_create, inode_rename

int write_user2role (int uid, const char *role)
{
	FILE *fp = fopen (USER2ROLE_PATH, "ab") ;
	
	if (!fp)
	{
		printf ("file open failed\n") ;
		return -1 ;
	}
	
	fwrite (&uid, sizeof(int), 1, fp) ;
	fwrite (role, sizeof(char), MAX_ROLENAME+1, fp) ;
	
	fclose (fp) ;
	return 1 ;
}

int user_exists (int uid)
{
	int _uid ;
	FILE *fp = fopen (USER2ROLE_PATH, "rb") ;
	
	if (!fp)
	{
//		printf ("file open failed\n") ;
		return -1 ;
	}
	
	while (fread(&_uid, sizeof(int), 1, fp))
	{
		if (_uid == uid)
		{
			fclose (fp) ;
			return 1 ;
		}
			
		fseek (fp, sizeof(char) * (MAX_ROLENAME + 1), SEEK_CUR) ;
	}
	
	fclose (fp) ;
	return 0 ;
}

int add_user2role (int uid, const char *role)
{
	int role_len = strlen (role) ;
	int _uid ;
	int i ;
	char _role[MAX_ROLENAME+1] ;
	
	if (role_len > MAX_ROLENAME)
	{
		printf ("Role name too long\n") ;
		return 0 ;
	}
	if (user_exists(uid) == 1)
	{
		printf ("The role of user already exists\n") ;
		return 0 ;
	}
	
	for (i=0; i<MAX_ROLENAME; i++)
	{
		if (i < role_len)
				_role[i] = role[i] ;
		else
				_role[i] = '\0' ;
	}
	
	return write_user2role(uid, _role) ;
}

int write_role (const char *role, const int *permission)
{
	FILE *fp = fopen (ROLES_PATH, "ab") ;
		
	if (!fp)
	{
		printf ("file open failed\n") ;
		return -1 ;
	}
	
	fwrite (role, sizeof(char), MAX_ROLENAME+1, fp) ;
	fwrite (permission, sizeof(int), PERMISSION_COUNT, fp) ;
	
	fclose (fp) ;
	return 1 ;
}

int role_exists (const char *role)
{
	char _role[MAX_ROLENAME+1] ;
	FILE *fp = fopen (ROLES_PATH, "rb") ;
	
	if (!fp)
	{
//		printf ("file open failed\n") ;
		return -1 ;
	}
	
	while (fread(_role, sizeof(char), MAX_ROLENAME+1, fp))
	{
		if (!strcmp (_role, role))
		{
			fclose (fp) ;
			return 1 ;
		}
			
		fseek (fp, sizeof(int) * PERMISSION_COUNT, SEEK_CUR) ;
	}
	
	fclose (fp) ;
	return 0 ;
}

int add_role (const char *role, const int *permission)
{
	int role_len = strlen (role) ;
	char _role[MAX_ROLENAME+1] ;
	int i ;
	
	if (role_len > MAX_ROLENAME)
	{
		printf ("Role name too long\n") ;
		return 0 ;
	}
	if (sizeof(permission) != PERMISSION_COUNT * sizeof(int))
	{
		printf ("invalid [permission] setting\n") ;
		return 0 ;
	}
	if (role_exists(role) == 1)
	{
		printf ("Role already exists\n") ;
		return 0 ;
	}
			
	for (i=0; i<MAX_ROLENAME; i++)
	{
		if (i < role_len)
				_role[i] = role[i] ;
		else
				_role[i] = '\0' ;
	}
	
	return write_role (_role, permission) ;
}

int del_user2role (int uid)
{
	FILE *fp = fopen (USER2ROLE_PATH, "rb") ;
	int uids[100] ;
	int count = 0 ;
	int i ;
	
	char roles[100][MAX_ROLENAME+1] ;
	
	if (!fp)
	{
		printf ("open file failed\n") ;
		return -1 ;
	}
	
	while (fread (&uids[count], sizeof(int), 1, fp))
	{
		fread (roles[count], sizeof(char), MAX_ROLENAME+1, fp) ;

		if (uids[count] != uid)
			count++ ;
	}
	
	fclose (fp) ;
	fp = fopen (USER2ROLE_PATH, "w") ;
	for (i=0; i<count; i++)
	{
		fwrite (&uids[i], sizeof(int), 1, fp) ;
		fwrite (roles[i], sizeof(char), MAX_ROLENAME+1, fp) ;
	}
	
	fclose (fp) ;
	return 1 ;
}

int del_role (const char *role)
{
	FILE *fp = fopen (ROLES_PATH, "rb") ;
	char roles[100][MAX_ROLENAME+1] ;
	int permissions[100][PERMISSION_COUNT] ;
	int count = 0 ;
	int i ;
	
	if (!fp)
	{
		printf ("open file failed\n") ;
		return -1 ;
	}
	
	while (fread (roles[count], sizeof(char), MAX_ROLENAME+1, fp))
	{
		fread (permissions[count], sizeof(int), PERMISSION_COUNT, fp) ;

		if (strcmp(roles[count], role))
			count++ ;
	}
	
	fclose (fp) ;
	fp = fopen (ROLES_PATH, "w") ;
	for (i=0; i<count; i++)
	{
		fwrite (roles[i], sizeof(char), MAX_ROLENAME+1, fp) ;
		fwrite (permissions[i], sizeof(int), PERMISSION_COUNT, fp) ;
	}
	
	fclose (fp) ;
	return 1 ;
}

int change_user2role (int uid, const char *role)
{
	int res1 = del_user2role (uid) ;
	int res2 = add_user2role (uid, role) ;
	
	return res1 * res2 ;
}

int change_role (const char *role, const int *permission)
{
	int res1 = del_role (role) ;
	int res2 = add_role (role, permission) ;
	
	return res1 * res2 ;
}

int show_user2role ()
{
	FILE *fp = fopen (USER2ROLE_PATH, "rb") ;
	int uid ;
	char role[MAX_ROLENAME+1] ;
	
	if (!fp)
	{
		printf ("No items now\n") ;
		return -1 ;
	}
	
	while (fread (&uid, sizeof(int), 1, fp))
	{
		fread (role, sizeof(char), MAX_ROLENAME+1, fp) ;
		printf ("uid %d : %s\n", uid, role) ;
	}
	
	fclose (fp) ;
	return 1 ;
}

int show_roles ()
{
	FILE *fp = fopen (ROLES_PATH, "rb") ;
	int permission[PERMISSION_COUNT] ;
	char role[MAX_ROLENAME+1] ;
	
	if (!fp)
	{
		printf ("No items now\n") ;
		return -1 ;
	}
	
	while (fread (role, sizeof(char), MAX_ROLENAME+1, fp))
	{
		fread (permission, sizeof(int), PERMISSION_COUNT, fp) ;
		printf ("%s permission: inode_create[%d], inode_rename[%d]\n", role, permission[0], permission[1]) ;
	}
	
	fclose (fp) ;
	return 1 ;
}

int _pow10 (int p)
{
	int res = 1 ;
	int i ;
	
	for (i=1; i<=p; i++)
		res *= 10 ;
		
	return res ;
}

int s2i (const char *s)
{
	int len = strlen (s) ;
	int i ;
	int res = 0 ;
	int temp ;
	
	for (i=0; i<len; i++)
	{
		temp = s[i] - '0' ;
		if (temp > 9 || temp < 0)
		{
			printf ("the string can not convert to number\n") ;
			return -1 ;
		}
		
		res += temp * _pow10(len - i - 1) ;
	}
	
	return res ;
}

int get_state ()
{
	FILE *fp = fopen (CONTROL_PATH, "rb") ;
	int state ;

	if (!fp)
	{
		printf ("file open failed\n") ;
		return -1 ;
	}
	
	fread(&state, sizeof(int), 1, fp) ;
	
	return state ;
}

int set_state (int state)
{
	FILE *fp = fopen (CONTROL_PATH, "wb") ;

	if (!fp)
	{
		printf ("file open failed\n") ;
		return 0 ;
	}

	fwrite (&state, sizeof(int), 1, fp) ;

	return 1 ;
}

int main(int argc, char *argv[])
{
	int i ;
	int permission_count ;
	int permission[PERMISSION_COUNT] ;
	int uid ;
	
	if (argc == 1)
	{
		printf ("Error: missing arguments\n") ;
		return 0 ;
	}
		
	if (!strcmp(argv[1], "-s"))
	{
		if (argc != 3)
		{
			printf ("Error: invalid arguments after -s\n") ;
			return 0 ;
		}
			
		if (!strcmp(argv[2], "user2role"))
		{
			show_user2role() ;
			return 0 ;
		}
			
		if (!strcmp(argv[2], "roles"))
		{
			show_roles() ;
			return 0 ;
		}
			
		printf ("Error: unkonw argument after -s\n") ;
		return 0 ;
	}
	
	if (!strcmp(argv[1], "-ra"))
	{
		if (argc != 4)
		{
			printf ("Error: invalid arguments after -ra\n") ;
			return 0 ;
		}
			
		permission_count = strlen (argv[3]) ;
		if (permission_count != PERMISSION_COUNT)
		{
			printf ("Error: invalid [permission] setting after -ra\n") ;
			return 0 ;
		}

		for (i=0; i<permission_count; i++)
		{
			permission[i] = argv[3][i] - '0' ;
			
			if (permission[i] != 0 && permission[i] != 1)
			{
				printf ("Error: invalid [permission] setting after -ra\n") ;
				return 0 ;
			}
		}
		
		if (add_role(argv[2], permission) == 1)
			printf ("Role added successfully\n") ;
		else
			printf ("Role add failed\n") ;
		
		return 0 ;
	}
	
	if (!strcmp(argv[1], "-ua"))
	{
		if (argc != 4)
		{
			printf ("Error: invalid arguments after -ua\n") ;
			return 0 ;
		}
		
		uid = s2i (argv[2]) ;
		if (uid == -1)
		{
			printf ("Error: invalid [uid] setting after -ua\n") ;
			return 0 ;
		}
		
		if (add_user2role(uid, argv[3]) == 1)
			printf ("The role of user added successfully\n") ;
		else
			printf ("The role of user add failed\n") ;
		
		return 0 ;
	}
	
	if (!strcmp(argv[1], "-rd"))
	{
		if (argc != 3)
		{
			printf ("Error: invalid arguments after -rd\n") ;
			return 0 ;
		}
		
		if (del_role(argv[2]) == 1)
			printf ("Role deleted successfully\n") ;
		else
			printf ("Role delete failed\n") ;
		
		return 0 ;
	}
	
	if (!strcmp(argv[1], "-ud"))
	{
		if (argc != 3)
		{
			printf ("Error: invalid arguments after -ud\n") ;
			return 0 ;
		}
		
		uid = s2i (argv[2]) ;
		if (uid == -1)
		{
			printf ("Error: invalid [uid] setting after -ud\n") ;
			return 0 ;
		}
		
		if (del_user2role(uid) == 1)
			printf ("The role of user deleted successfully\n") ;
		else
			printf ("The role of user delete failed\n") ;
		
		return 0 ;
	}
	
	if (!strcmp(argv[1], "-rc"))
	{
		if (argc != 4)
		{
			printf ("Error: invalid arguments after -rc\n") ;
			return 0 ;
		}
			
		permission_count = strlen (argv[3]) ;
		if (permission_count != PERMISSION_COUNT)
		{
			printf ("Error: invalid [permission] setting after -rc\n") ;
			return 0 ;
		}

		for (i=0; i<permission_count; i++)
		{
			permission[i] = argv[3][i] - '0' ;
			
			if (permission[i] != 0 && permission[i] != 1)
			{
				printf ("Error: invalid [permission] setting after -rc\n") ;
				return 0 ;
			}
		}
		
		if (change_role(argv[2], permission) == 1)
			printf ("Role changed successfully\n") ;
		else
			printf ("Role change failed\n") ;
		
		return 0 ;
	}
	
	if (!strcmp(argv[1], "-uc"))
	{
		if (argc != 4)
		{
			printf ("Error: invalid arguments after -uc\n") ;
			return 0 ;
		}
		
		uid = s2i (argv[2]) ;
		if (uid == -1)
		{
			printf ("Error: invalid [uid] setting after -uc\n") ;
			return 0 ;
		}

		if (change_user2role(uid, argv[3]) == 1)
			printf ("The role of user changed successfully\n") ;
		else
			printf ("The role of user change failed\n") ;
		
		return 0 ;
	}

	if (!strcmp(argv[1], "-state"))
	{
		if (argc > 2)
		{
			printf ("Error: invalid arguments after -state\n") ;
			return 0 ;
		}
		
		int state = get_state () ;
		if (state == 1)
			printf ("State: Enable\n") ;
		else if (state == 0)
			printf ("State: Disable\n") ;

		return 0 ;
	}

	if (!strcmp(argv[1], "-enable"))
	{
		if (argc > 2)
		{
			printf ("Error: invalid arguments after -enable\n") ;
			return 0 ;
		}

		if (set_state (1) == 1)
			printf ("Enable!\n") ;

		return 0 ;
	}
	
	if (!strcmp(argv[1], "-disable"))
	{
		if (argc > 2)
		{
			printf ("Error: invalid arguments after -disable\n") ;
			return 0 ;
		}

		if (set_state (0) == 1)
			printf ("Disable!\n") ;

		return 0 ;
	}

	printf ("Error: invalid argument option\n") ;

	return 0 ;
}
