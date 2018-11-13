
#define MAX_MESG 128

char ** largv = NULL;
int     largc;
char ** lenvp = NULL;
char ** lenvp_start = NULL; 


// commands store
struct comm {
	int cmd_id;
	 union cmd_t {
		int  msgq_no;
		char cmd_args[MAX_MESG];
	 } m;
} commands;

// structure and indetifiers for message queue 
struct mesg_buffer {
	 long mesg_type;
	 char mesg_text[MAX_MESG];
} message;

int msgid;
key_t key;

static void sig_handler(int sig);
void die(const char* format, ...);

void setRandezvous(void);
void setProcessName(void);
int  processCommands(void);
int  backgroundDaemonLeader(char * cmd_bg);
void cleanEnv(char* ldvar, int ldlen);
