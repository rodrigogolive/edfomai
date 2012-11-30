#include <linux/module.h>
//#include <native/mutex.h>
#include <rtdm/rtdm_driver.h>
#include "edfomai-data.h"
#include "edfomai-drv-data.h"

//#define DEBUG

#define DEVICE_NAME "edfomai"
#define SOME_SUB_CLASS 4711
#define POLL_DELAY 1000

/**
* The context of a device instance
* A context is created each time a device is opened and passed to
* other device handlers when they are called.
*/
typedef struct context_s{
} context_t;

/* 
* Access to edf data structures and services must be coordinated through this Spin Lock
*/
static rtdm_lock_t edf_data_lock;
//static RT_MUTEX edf_data_mutex;

/*
* 
*/
static int _mtx_create(void){
	int status=0;
	rtdm_lock_init(&edf_data_lock);
	//status=rt_mutex_create( &edf_data_mutex, DMUTEX_NAME);
        if (status){
        rtdm_printk("Edfomai: [@mtx] mutex (%s) creation failed with status (%d)\n", DMUTEX_NAME, status);
		return status;
	}
	return 0;
}
/*
* 
*/
static int _mtx_delete(void){
	int status=0;
        //status=rt_mutex_delete(&edf_data_mutex);
        if (status){
        rtdm_printk("Edfomai: [@mtx] mutex (%s) deletion failed with status (%d)\n", DMUTEX_NAME, status);
                return status;
        }
        return 0;
}
/*
* 
*/
static int _mtx_acquire(void){
	int status=0;
	rtdm_lock_get(&edf_data_lock);
        //status=rt_mutex_acquire(&edf_data_mutex, TM_INFINITE);
        if (status){
	rtdm_printk("Edfomai: [@mtx] mutex (%s) acquire failed with status (%d)\n", DMUTEX_NAME, status);
		return status;
        }
	return 0;
}
/*
* 
*/
static int _mtx_release(void){
        int status=0;
	rtdm_lock_put(&edf_data_lock);
	//status=rt_mutex_release(&edf_data_mutex);
        if (status){
              rtdm_printk("Edfomai: [@mtx] mutex (%s) release failed with status (%d)\n", DMUTEX_NAME, status);
              return status;
         }
	return 0;
}
static RT_TASK * _get_handle( char * rname ){
	int status=0;
	xnthread_t * task;
	xnhandle_t handle; // pointer to a xnobject_t
	//RT_TASK * rtask=NULL;
	//status= rt_task_bind(rtask,rname,TM_NONBLOCK);
	status=xnregistry_bind(rname, XN_NONBLOCK, XN_RELATIVE , &handle );
        if (status){
              rtdm_printk("Edfomai: [@get_handle] cant obtain (%s) handle from registry (%d)\n", rname,status);
              return NULL;
         }
	task=xnregistry_fetch(handle);
	if (task==NULL){
		rtdm_printk("Edfomai: [@get_handle] cant fetch obtained handle (%s)\n",rname);
		return NULL;
	}
	return T_DESC(task);
}
/**
* Open the device
* This function is called when the device shall be opened.
*/
static int edf_rtdm_open_nrt(struct rtdm_dev_context *context, rtdm_user_info_t *user_info, int oflags)
{
        #ifdef DEBUG
        rtdm_printk("Edfomai: [@open] device has been opened\n");
        #endif
	return 0;
}

/**
* Close the device
* This function is called when the device shall be closed.
*/
static int edf_rtdm_close_nrt(struct rtdm_dev_context *context, rtdm_user_info_t * user_info)
{
        #ifdef DEBUG
        rtdm_printk("Edfomai: [@close] device has been closed\n");
        #endif
	return 0;
}

/**
* Read from the device
* This function is called when the device is read (non-realtime)
* context.
*/
static ssize_t edf_rtdm_read_nrt(struct rtdm_dev_context *context, rtdm_user_info_t * user_info, void *buf, size_t nbyte)
{
        #ifdef DEBUG
        rtdm_printk("Edfomai: [@read] device is read\n");
  	#endif
  	// TODO: copy to user some kind of useful information for monitoring purposes..
	return -1;
}

/**
* Write in the device
* This function is called when the device is written (non-realtime).
* NOTE: only EDFMessage are interpreted by this routine, a negative value wille be
* returned
*/
static ssize_t edf_rtdm_write_nrt(struct rtdm_dev_context *context, rtdm_user_info_t * user_info, const void *buf, size_t nbyte)
{
	EDFMessage * message;
	RT_TASK * task;
	rtdm_lockctx_t lock_ctx;
	int status;
	//RT_TASK * rt_curr;
	//xnthread_t * curr;

	//if (nbyte!=sizeof(EDFMessage))
	//      return -1; /* You should talk to EDF-SCHED with EDFMessages */
	
	message = (EDFMessage*) rtdm_malloc(sizeof(EDFMessage));
	if ( rtdm_copy_from_user( user_info, message, buf, sizeof(EDFMessage) ) == EFAULT ){
		rtdm_free(message);
		return -EFAULT;
	}
	#ifdef DEBUG
	rtdm_printk("Edfomai: [@write] message received (%s)\n",(char*) &(message->task));
	#endif	
	//curr = (xnthread_t*) rtdm_task_current();
	//rt_curr=T_DESC(curr);
	//rtdm_printk("Edfomai: [@write] current task (%s)(%s)\n",curr->name,rt_curr->rname);
	task = _get_handle(message->task);
	if (task==NULL){
		rtdm_free(message);
		return -1;
	}
	#ifdef DEBUG
	rtdm_printk("Edfomai: [@write] serving request for task (%s)\n",task->rname);
	#endif	
	
	switch (message->command)
	{
	      case CREATE_TASK:
			#ifdef DEBUG
	      		rtdm_printk("Edfomai: [@write] task creation (%s)\n",task->rname);
			#endif
	             	status=_mtx_acquire();
	             	if (status){
				rtdm_free(message);
				return status;
	              	}
		      	rtdm_lock_irqsave(lock_ctx);
	              	status=0;//rt_dtask_create ( task , message->deadline );
		      	rtdm_lock_irqrestore(lock_ctx);
	        	if (status){
				#ifdef DEBUG
				rtdm_printk("Edfomai: [@write] problem during task creation\n");
				#endif
			}      
		/*
	      	* Task should be started outside the module 
	      	* because function pointer to new task main procedure 
	      	* must be provided during the rt_task_start.
	      	* We haven't it here.
	      	*/
	              	status=_mtx_release();
	              	if (status){
				rtdm_free(message);
				return status;
	              	}
	
	      break; 
	      case START_TASK:
			#ifdef DEBUG
	      		rtdm_printk("Edfomai: [@write] task start (%s)\n",task->rname);
			#endif
	      	/* 
	      	* We only set the task deadline. 
	      	* If you want to use this functionality properly you should
	      	* first create the rt_task specifying the DEADLINENOTSET param, 
	      	* then set the actual deadline using this command. 
	      	* This way task deadline will not be considered (more or less:).
	      	*/
	      		status=_mtx_acquire();
	      		if (status){
	      			rtdm_free(message);
				return status;
	      		}
			rtdm_lock_irqsave(lock_ctx);
	      		status=rt_dtask_setdeadline ( task , message->deadline );
			rtdm_lock_irqrestore(lock_ctx);
                	if (status){
                        	#ifdef DEBUG
                       		rtdm_printk("Edfomai: [@write] problems during task start\n");
                        	#endif
                	}

			status=_mtx_release();
	      		if (status){
	              		rtdm_free(message);  
		    		return status;
              		}
	      break;		
	      case RESET_DEADLINE:
			#ifdef DEBUG
	      		rtdm_printk("Edfomai: [@write] task reset deadline (%s)\n",task->rname);
	      		#endif
			status=_mtx_acquire();
	      		if (status){
	      			rtdm_free(message);
				return status;
	      		} 
	      		rtdm_lock_irqsave(lock_ctx);
	       		status=rt_dtask_resetdeadline(task);
	       		rtdm_lock_irqrestore(lock_ctx);
               		if (status){
                        	#ifdef DEBUG
                        	rtdm_printk("Edfomai: [@write] problems during deadline reset\n");
                        	#endif
               		}
	       		status=_mtx_release();
			if (status){
	               		rtdm_free(message);
	         		return status;
			}
	      break;
              case SET_DEADLINE:
			#ifdef DEBUG
			rtdm_printk("Edfomai: [@write] task set deadline (%s)\n", task->rname);
			#endif
			status=_mtx_acquire();
			if (status){
                        	rtdm_free(message);
				return status;
                	}
			rtdm_lock_irqsave(lock_ctx);
			status=rt_dtask_setdeadline(task, message->deadline);
			rtdm_lock_irqrestore(lock_ctx);
			if (status){
               	        	#ifdef DEBUG
               	         	rtdm_printk("Edfomai: [@write] problems during deadline set\n");
               	         	#endif
			}
	
			status=_mtx_release();
			if (status){
                              	rtdm_free(message);
				return status;
			}
              break;
	    default:
	        #ifdef DEBUG
        	rtdm_printk("Edfomai: [@write] unknown message\n");
        	#endif
		break;
	}
	rtdm_free(message);
	return nbyte;
}



/*
* This structure describe the simple RTDM device
*/
static struct rtdm_device device = {
	.struct_version = RTDM_DEVICE_STRUCT_VER,
	.device_flags = RTDM_NAMED_DEVICE,
	.context_size = sizeof(context_t),
	.device_name = DEVICE_NAME,
	.open_nrt = edf_rtdm_open_nrt,
	.ops = {
		.close_nrt = edf_rtdm_close_nrt,
		.read_nrt = edf_rtdm_read_nrt,
		.write_nrt = edf_rtdm_write_nrt,
	},
	.device_class = RTDM_CLASS_EXPERIMENTAL,
	.device_sub_class = SOME_SUB_CLASS,
	.profile_version = 1,
	.driver_name = "EDFOMAI",
	.driver_version = RTDM_DRIVER_VER(0, 0, 1),
	.peripheral_name = "EDFOMAI-SCHED-SVC",
	.provider_name = "Luca Mella",
	.proc_name = device.device_name,
};
/*
* START/SWITCH Task callback 
*/
void edf_startswitch_hook( void * cookie )
{
        RT_TASK * task;
        task=T_DESC(cookie);
	#ifdef DEBUG
	rtdm_printk("Edfomai: [@start/switch] task started or switched (%s)\n",task->rname);
	#endif
	rt_dtask_recalculateprio();
}
/*
* DELETE Task callback 
* 
*/
void edf_delete_hook( void * cookie )
{
	RT_TASK * task;
	task=T_DESC(cookie);
	#ifdef DEBUG
	rtdm_printk("Edfomai: [@delete] task deleted (%s)\n",task->rname);
	#endif
	rt_dtask_delete(task);
	rt_dtask_recalculateprio();
}
/**
* This function is called when the module is loaded.
* It simply registers the RTDM device and initialize data structures.
*/
int __init edf_rtdm_init(void)
{
	int res;
	res = rtdm_dev_register(&device);
	if(res == 0) {
		rtdm_printk("Edfomai: [@init] driver registered correctly\n");
		res=rt_dtask_init();
		if (res!=0){
			rtdm_printk("Edfomai: [@init] data structure initialization failed.\n");
		}
		res=_mtx_create();
                if (res!=0){
                        rtdm_printk("Edfomai: [@init] mutex (%s) creation failed with status (%d).\n",DMUTEX_NAME,res);
                }
		res=rt_task_add_hook(T_HOOK_START|T_HOOK_SWITCH, &edf_startswitch_hook);
                if (res!=0){
                        rtdm_printk("Edfomai: [@init] start/switch hook failed registration with status (%d).\n",res);
                }		
		res=rt_task_add_hook(T_HOOK_DELETE, &edf_delete_hook);
                if (res!=0){
                        rtdm_printk("Edfomai: [@init] delete hook failed registration with status (%d).\n",res);
                }

	}
	else {
		rtdm_printk("Edfomai: [@init] driver registration failed\n");
	    	switch(res) {
	      case -EINVAL:
	      	rtdm_printk("Edfomai: [@init] the device structure contains invalid entries. Check kernel log for further details.\n");
	      break;
	      case -ENOMEM:
	      	rtdm_printk("Edfomai: [@init] the context for an exclusive device cannot be allocated.\n");
	      break;
	      case -EEXIST:
	      	rtdm_printk("Edfomai: [@init] the specified device name of protocol ID is already in use.\n");
	      break;
	      case -EAGAIN: 
	      	rtdm_printk("Edfomai: [@init] some /proc entry cannot be created.\n");
	      break;
	      default:
	      	rtdm_printk("Edfomai: [@init] Unknown error code returned\n");
	      break;
	      }
	  }
	return res;
}

/**
* This function is called when the module is unloaded.
* It unregister the RTDM device and free data structures.
*
*/
void __exit edf_rtdm_exit(void)
{
	rtdm_printk("Edfomai: [@exit] removing module\n");
	rt_task_remove_hook(T_HOOK_START|T_HOOK_SWITCH, &edf_startswitch_hook);
	rt_task_remove_hook(T_HOOK_DELETE, &edf_delete_hook);  
	//_mtx_acquire();
	rt_dtask_dispose();
	_mtx_delete();
	rtdm_dev_unregister(&device, 1000);
	rtdm_printk("Edfomai: [@exit] removal complete\n");
}

module_init(edf_rtdm_init);
module_exit(edf_rtdm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Mella <lucam.ko@gmail.com>");
MODULE_DESCRIPTION("EDF Scheduler support");
MODULE_VERSION("0.1a");

