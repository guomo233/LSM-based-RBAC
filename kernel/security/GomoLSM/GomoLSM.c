#include <linux/lsm_hooks.h>
#include <linux/sysctl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/types.h>
#include <linux/dcache.h>
#include <linux/string.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define USER2ROLE_PATH "/etc/GomoLSM/user2role"
#define ROLES_PATH "/etc/GomoLSM/roles"
#define CONTROL_PATH "/etc/GomoLSM/control"
#define MAX_ROLENAME 20
#define PERMISSION_COUNT 2 //inode_create, inode_rename

int get_role (const int uid, char *role)
{
	struct file *fout = filp_open (USER2ROLE_PATH, O_RDONLY, 0) ;
	int _uid ;
	char uid_buf[sizeof(int)] ;
	mm_segment_t fs ;
	int res = 0 ;

	if (!fout || IS_ERR(fout))
	{
		printk ("GomoLSM: [get_role] load file error\n") ;
		return -1 ;
	}
	
	fs = get_fs () ;
	set_fs (KERNEL_DS) ;

	while ((vfs_read(fout, uid_buf, sizeof(int), &fout->f_pos)) > 0)
	{
		memcpy (&_uid, uid_buf, sizeof(int)) ;
		vfs_read (fout, role, sizeof(char) * (MAX_ROLENAME+1), &fout->f_pos) ;
		
		if (uid == _uid)
		{
			printk ("GomoLSM: [get_role] uid: %d, role: %s\n", uid, role) ;
			
			res = 1 ;
			break ;
		}
	}

	if (res == 0)
		printk ("GomoLSM: [get_role] uid: %d has no role\n", uid) ;

	set_fs (fs) ;
	filp_close (fout, NULL) ;
	
	return res ;
}

int role_permission (const char *role, const int op)
{
	struct file *fout = filp_open (ROLES_PATH, O_RDONLY, 0) ;
	mm_segment_t fs ;
	char _role[MAX_ROLENAME+1] ;
	char permission_buf[PERMISSION_COUNT * sizeof(int)] ;
	int permission[PERMISSION_COUNT] ;
	int _op ;
	int res = -1 ;

	if (!fout || IS_ERR(fout))
	{
		printk ("GomoLSM: [role_permission] load file error\n") ;
		return -1 ;
	}
	
	fs = get_fs () ;
	set_fs (KERNEL_DS) ;

	while ((vfs_read(fout, _role, sizeof(char) * (MAX_ROLENAME+1), &fout->f_pos)) > 0)
	{
		vfs_read (fout, permission_buf, sizeof(int) * PERMISSION_COUNT, &fout->f_pos) ;
		memcpy (permission, permission_buf, sizeof(int) * PERMISSION_COUNT) ;
		
		if (strcmp(role, _role))
			continue ;

		for (_op=0; _op<PERMISSION_COUNT; _op++)
		{
			if (_op == op && permission[_op] == 1)
			{
				printk ("GomoLSM: [role_permission] role: %s has permission\n", role) ;
					
				res = 1 ;
				break ;
			}
			else if (_op == op && permission[_op] == 0)
			{
				printk ("GomoLSM: [role_permission] role: %s has no permission\n", role) ;
				res = 0 ;
				break ;
			}	
		}
	}

	if (res == -1)
		printk ("GomoLSM: [role_permission] role: %s not exist", role) ;

	set_fs (fs) ;
	filp_close (fout, NULL) ;

	return res ;
}

int is_enable (void)
{
	struct file *fout = filp_open (CONTROL_PATH, O_RDONLY, 0) ;
	char state_buf[sizeof(int)] ;
	int state ;
	mm_segment_t fs ;

	if (!fout || IS_ERR(fout))
	{
		printk ("GomoLSM: [is_enable] load file error\n") ;
		return -1 ;
	}
	
	fs = get_fs () ;
	set_fs (KERNEL_DS) ;
	
	vfs_read(fout, state_buf, sizeof(int), &fout->f_pos) ;
	memcpy (&state, state_buf, sizeof(int)) ;
	
	set_fs (fs) ;
	filp_close (fout, NULL) ;

	return state ;
}

int user_permission (int uid, int op)
{
	char role[MAX_ROLENAME+1] ;
	
	if (uid <= 999)
		return 0 ;
	
	if (!is_enable())
		return 0 ;

	if (get_role (uid, role) != 1)
		return 0 ;

	if (role_permission (role, op) == 0)
		return -1 ;

	return 0 ;
}

int gmlsm_inode_create (struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int uid = current->real_cred->uid.val ;
	printk ("GomoLSM: call [inode_create] by uid: %d\n", uid) ;
	
	return user_permission (uid, 0) ;
}

int gmlsm_inode_rename (struct inode *old_inode, struct dentry *old_dentry, 
			struct inode *new_inode, struct dentry *new_dentry)
{
	int uid = current->real_cred->uid.val ;
	printk ("GomoLSM: call [inode_rename] by uid: %d\n", uid) ;

	return user_permission (uid, 1) ;
}

static struct security_hook_list gmlsm_hooks[] = {
    LSM_HOOK_INIT(inode_rename,gmlsm_inode_rename),
    LSM_HOOK_INIT(inode_create,gmlsm_inode_create),
};

void __init gmlsm_add_hooks(void)
{
    pr_info("GomoLSM: LSM LOADED.\n");
    security_add_hooks(gmlsm_hooks, ARRAY_SIZE(gmlsm_hooks));
}

static __init int gmlsm_init(void){
    gmlsm_add_hooks();
    return 0;
}

security_initcall(gmlsm_init);
