#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
/***defines***/
#define CTRL_KEY(k) ((k) & 0x1f)

/***data***/

struct editorConfig{
	int num_rows;
	int num_cols;
	struct termios orig_termios;
};
struct editorConfig E;//global variable containing editor state;

/****terminal****/
//handle low level input
char editorReadKey(){//wait for one keypress and return it.
	int val;
	char temp;
	while((val=read(STDIN_FILENO, &temp, 1))!=1){
		if(val==-1 && errno!=EAGAIN) die("read");
	}
	return temp;
}
void die(const char* s){
	//clear screen when exit;
	write(STDOUT_FILENO, "\x1b[2J",4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	//check the global errno (which is set by C library functions);
	perror(s);
	exit(1);
}
void disableRawMode(){
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios)==-1){
		die("tcsetattr");
	}
}
void enableRawMode(){
	if(tcgetattr(STDIN_FILENO, &E.orig_termios)==-1){
		die("tcgetattr");
	}
	atexit(disableRawMode);
	struct termios raw=E.orig_termios;
	//ICRNL is used for disabling contrl+M;
	raw.c_iflag&=~(IXON|ICRNL|BRKINT|INPCK|ISTRIP);
	raw.c_oflag&=~(OPOST);// turn off all output processing features
	raw.c_lflag&=~(ECHO|ICANON|ISIG|IEXTEN);
	raw.c_cflag|=~(CS8);
	//set the character size to 8-bit;
	raw.c_cc[VMIN]=0;//minimum number of bytes of input needed 
	//before read() returns;
	raw.c_cc[VTIME]=1;
	//maximum time to wait before returning.
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)==-1){
		die("tcsetattr");
	}
}
int getWindowSize(int* rows, int* cols){
	struct winsize ws;
	//get the size of the terminal by simply calling ioctl(), which will return -1 when fail;
	//with TIOCGWINSZ request -> Terminal Input/Output Control Get WINdow SiZe;
	//we also ignore the case of returning 0, because it is s useless value;
	if(1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)==-1 || ws.ws_col==0){
		if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12)!=12) return -1;
		editorReadKey();
		return -1;
	}else{
		*cols=ws.ws_col;
		*rows=ws.ws_row;
		return 0;
	}
}

/****output****/
void editorDrawRows(){
	for(int i=0;i<E.num_rows;i++){
		write(STDOUT_FILENO, "~\r\n",3);
	}
}
void editorRefreshScreen(){
	write(STDOUT_FILENO, "\x1b[2J",4);
	//explan: "4" means writing 4 bytes, with the first one as "\x1b" (=27 dec), which is the escape character;
	//the other three are "[", "2" and "J".
	//"J" command: clear screen;
	write(STDOUT_FILENO, "\x1b[H",3);
	//re-position it at the top-left corner; 
	//"H" command position the cursor;
	//if want the cursor at x row and y column, use "\x1b[x;yH"; row and col starts from 1.
	editorDrawRows();
	write(STDOUT_FILENO, "\x1b[H", 3);
}

/*map to editor operations*/
/***input***/
//mapping keys to editor functions;
void editorProcessKeypress(){
	char temp=editorReadKey();
	switch(temp){
		case CTRL_KEY('q'):
		write(STDOUT_FILENO,"\x1b[2J",4);
		write(STDOUT_FILENO,"\x1b[H", 3);
		exit(0);
		break;
	}
}

/****init****/
void initEditor(){
	if(getWindowSize(&E.num_rows, &E.num_cols)==-1){
		die("getWindowSize");
	}
}
int main(){
	enableRawMode();
	initEditor();
	while(1){
		editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}
