/* Group No: 4
Esha Manideep Dinne, 19CS10030
ASRP Vyahruth, 19CS10002
*/

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <signal.h>
#include <time.h>

using namespace std;

#define MAX_SIZE 128
#define HISTORY_SIZE 10000


static struct termios old, current;
int tempint;
int cmdsthisinstance = 0;

void initTermios(int echo) 
{
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    current = old; /* make new settings same as old settings */
    current.c_lflag &= ~ICANON; /* disable buffered i/o */
    if (echo) {
        current.c_lflag |= ECHO; /* set echo mode */
    } else {
        current.c_lflag &= ~ECHO; /* set no echo mode */
    }
    tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
    tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_() 
{
  char ch;
  initTermios(0);
  ch = getchar();
  resetTermios();
  return ch;
}

static volatile bool tobackground = false;

string readCommand();
int ExecuteShell(char** args, int argscount, int input_fd, int output_fd);
int ExecuteCd(char** args);
int ExecuteExit(char** args);
int ExecuteHelp(char** args);
int ExecuteHistory(char** args);
int autocomplete(string, vector <string>&);
string searchinhistory();
void sighandler(int);
void stphandler(int);

string readCommand()
{
    //required variables
    char c;
    int position = 0;
    string cmd;
    
    while(1)
    {
        //read the character
        c = getch_();
        //check for ctrl-r
        if((int)c==18) {
			string temps = searchinhistory();
			if(temps.empty()) {
				printf("No matching command could be found\n");
				continue;
			}
			printf(">>> ");  int k = temps.size();
			for(int i=0;i<k;i++) {
				if(i==position) {
					cmd.push_back(temps[i]);
					position++;
				}
				printf("%c",temps[i]);
			}
            continue;
        }
        //arrow keys
        if((int)c==27) {
            // printf("Inside escape\n");
            c = getch_();
            // printf("%d\n",(int)c);
            if((int)c == 91) {
                c = getch_(); 
                // if((int)c==67) printf("Right Arrow\n");
                // else if((int)c==65) printf("Up Arrow\n");
                // else if((int)c==66) printf("Down Arrow\n");
                // else printf("Left Arrow\n");
                // if()
                // c = getch_(); getch_();
                continue;
            }
        }
        //backspace handling
        if((int)c == 127) {
            if(position>0) {
                position--; printf("\b \b");
                cmd.pop_back();
            }
            continue;
        }
        //autocomplete handling
        else if(c == '\t') {
            vector <string> matches;
            int fromhere = autocomplete(cmd,matches), k = matches.size();
            if(k==0) continue;
            if(k>1) {
                printf("\n");
                for(int i =0;i<min(k,9);i++) 
                    cout << i+1 << "." << matches[i] << "\t";
                printf("\n>>> "); cout << cmd; 
                c = getch_(); tempint = c-'0';
                if((tempint<1) || (tempint>min(9,k))) continue;
                tempint--; 
            }
            if(k==1) tempint = 0;
            k = position - fromhere; 
            while(k<matches[tempint].size()) {
                cmd.push_back(matches[tempint][k]);
                printf("%c",matches[tempint][k]); position++;
                k++;
            }
            continue;
        }
        //print the character because echo is off
        printf("%c",c); 
        //exit upon EOF or newline
        if(c == EOF || c == '\n') {
            return cmd;
        }
        else {
            position++;
        }
        cmd.push_back(c);
    }
}

vector<string> PipesSplit(string cmd_line) {
	//splitting pipes
	vector <string> commands;
	string s;
	int len = cmd_line.size();
	int i=0;
	while(i < len){
		// checking for double invert commas
		if(cmd_line[i] == '\"'){
			s += cmd_line[i];
			i++;

			while((cmd_line[i] != '\"') && (i<len)){
				s += cmd_line[i];
				i++;
			}

			s+= cmd_line[i];
			i++;
		}
		// checking for single inverted commas
		else if(cmd_line[i] == '\''){
			s += cmd_line[i];
			i++;

			while((cmd_line[i] != '\'') && (i<len)){

				s += cmd_line[i];
				i++;
			}

			s+= cmd_line[i];
			i++;
		}
		
		//checking for pipe
		else if(cmd_line[i] == '|'){
			commands.push_back(s);
			s.clear(); i++;
		}
		else{
			s += cmd_line[i];
			i++;
		}
	}

	commands.push_back(s);
	return commands;
}

char** CommandSplit(string command, int* no_of_tokens){
	char** tokens = (char**)malloc(sizeof(char*) * MAX_SIZE);
	// allocating space for tokens
	if(tokens == NULL){
		cout << "Error, memory allocation failed"<< endl;
		exit(EXIT_FAILURE);
	}

	for(int i=0 ; i<MAX_SIZE; i++){
		tokens[i] = (char*)malloc(sizeof(char)*MAX_SIZE);

		if(tokens[i] == NULL){
			cout << "Error, memory allocation failed" << endl;
			exit(EXIT_FAILURE);
		}
	}

	int i=0,j, index =0, len = command.length();
	*no_of_tokens = 0;
	int curr_size = MAX_SIZE;
	bool isword = false;

	while(i < len){
		if(command[i] == ' ' || command[i] == '\n' || command[i] == '\t' || command[i] == '\r' || command[i] == '\a'){
			if(!isword) {
				while(command[i] == ' ' || command[i] == '\n' || command[i] == '\t' || command[i] == '\r' || command[i] == '\a') i++;
				continue;
			}
			if(strlen(tokens[(*no_of_tokens)]) > 0){
				tokens[*no_of_tokens][index] = '\0';
				(*no_of_tokens)++ ;
			}
			index = 0;
			isword = false;
			i++;
		}

		//checking for single inverted comma and reading till the other one
		else if(command[i] == '\''){
			isword = true;
			index = 0;
			i = i+1;
			while(i<len && command[i] != '\''){

				tokens[*no_of_tokens][index] = command[i];
				index++;
				i++;
			}
			if(strlen(tokens[*no_of_tokens]) >0){

				tokens[*no_of_tokens][index] = '\0';
				(*no_of_tokens)++;
			}
			index = 0;
			i++;
		}
	//checking for double inverted comma and reading till the other one
		else if(command[i] == '\"'){
			isword = true;
			if(tokens[(*no_of_tokens)-1][0] == 'a' && tokens[(*no_of_tokens)-1][1] == 'w' && tokens[(*no_of_tokens)-1][2] == 'k'){
				fprintf(stderr, "Error in Syntax\n");
				return NULL;
			}

			index = 0;
			i = i+1;
			while(i<len && command[i] != '\"'){
				tokens[*no_of_tokens][index] = command[i];
				index++;
				i++;
			}

			if(strlen(tokens[*no_of_tokens]) >0){
				tokens[*no_of_tokens][index] = '\0';
				(*no_of_tokens)++;
			}
			index = 0;
			i++;
		}
		else{

			isword = true;
			tokens[*no_of_tokens][index] = command[i];
			index++;

			if( i == len - 1){
				if(strlen(tokens[*no_of_tokens]) > 0){
					tokens[*no_of_tokens][index] = '\0';
					(*no_of_tokens)++;
				}
			}

			i++;
		}

		// reallocate the memory if current memory not sufficient
		if((*no_of_tokens) >= curr_size){
			curr_size += MAX_SIZE;
			tokens = (char**)realloc(tokens, sizeof(char*)*curr_size);
		}
	}
	return tokens;
}

char const* builtIns[] = {
    "cd",
    "exit",
    "help",
    "history"
};

int (*builtInFuncs[])(char**)={
    &ExecuteCd,
    &ExecuteExit,
    &ExecuteHelp,
    &ExecuteHistory
};

int Execute(char** args, int argscount){

	//checking for inbuilt functions first and then calling shell exevute which uses execvp()
	if(args[0] == NULL)
		return EXIT_SUCCESS;
	else{
		for(int i=0; i< 4; i++){
			if(strcmp(args[0], builtIns[i]) == 0){

				return (*builtInFuncs[i])(args);
			}

		}
		return ExecuteShell(args, argscount,0,1);
	}

}

int ExecuteCd(char** args) {

	// using the chdir in C to execute cd
    if(args[1]==NULL){
        cout << "Error, cd expects one argument, no argument given." << endl ;
    }
    else{
        if(chdir(args[1])!=0){
            cout << "Error, no such directory exits." << endl;
        }
    }
    return EXIT_SUCCESS;
}

int ExecuteExit(char** args) {
    return 1;
}

int ExecuteHelp(char** args) {

	//printing avaialble commands (basic help function)
    cout << "Welcome to SHELL HELP" << endl;
    cout << "The following are the built-in commands: " << endl;
    for(int i=0;i<3;i++){
         cout << builtIns[i] << endl;       
    }
    cout << "Type man <command> to know about a command" << endl;
    cout << "Type man to know about other commands" << endl;
    return EXIT_SUCCESS;
}


int ExecuteShell(char** args, int argscount, int input_fd, int output_fd) {

	pid_t pid, wait_pid;
	char *arg;
	int status;
	
	// redirecting stdin and stdout out our input_fd and output_fd files
	if((pid=fork()) == 0){

		if(input_fd != 0){
			dup2(input_fd, 0);
			close(input_fd);
		}
		if(output_fd != 1){
			dup2(output_fd, 1);
			close(output_fd);
		}

		int Cmd = 0, firstsymbol = 0;

		// checking for redirect symbols and redirecting the output accordingly using dup2()
		for(int i=0; i<argscount; i++){
			arg = args[i];

			if(firstsymbol == 0){
				if(strcmp(arg, ">")==0 || strcmp(arg, "<")==0 || strcmp(arg, "&")==0  ){
					firstsymbol = 1;
					Cmd = i;
				}
			}
			if(strcmp(arg, ">")==0){
				int redirect_output_fd = open(args[i+1], O_CREAT | O_TRUNC | O_WRONLY, 0666);
				dup2(redirect_output_fd, STDOUT_FILENO);
			}
			if(strcmp(arg, "<")==0){
				int redirect_input_fd = open(args[i+1], O_RDONLY);
				dup2(redirect_input_fd, STDIN_FILENO);
			}
		}

		if(firstsymbol == 0) Cmd = argscount;

		// creating args_copy to copy the args to be sent to execvp() and terminating it with a null character
		char** args_copy = (char**)malloc(sizeof(char*)*(Cmd+1));

		if(args == NULL){

			cout << "Error, memory allocation failed"<< endl;
			exit(EXIT_FAILURE);
		}
		int i=0;

		for(i=0; i<Cmd;i++){
			args_copy[i] = args[i];
		}
		args_copy[i] = NULL;

		// call execvp() and error check if wrong
		if(execvp(args_copy[0], args_copy) == -1){
			fprintf(stderr, "Error, failed to excute the given command\n");
			exit(EXIT_FAILURE);
		}
	}

	else if(pid < 0){
		fprintf(stderr, "Error, forking failed\n");
		exit(EXIT_FAILURE);
	}

	else{
		if(strcmp(args[argscount-1], "&") != 0 && !tobackground){
			signal(SIGTSTP, stphandler);
			do{

				if(tobackground == true)break;
                wait_pid = waitpid(pid,&status,WUNTRACED);
            } while(!WIFEXITED(status) && !WIFSIGNALED(status));

		}
		if(WIFSTOPPED(status)) kill(pid,SIGCONT);
		tobackground = false;
	}
	return EXIT_SUCCESS;
} 

int CompleteExecute(string main_cmd) {
    if(main_cmd.empty()) return EXIT_SUCCESS;
	// taking the user input and pipe splitting it
	vector<string> piped_commands = PipesSplit(main_cmd);
	int no_of_pipe_process = piped_commands.size();

	int status;
	char** args;
	int no_of_tokens;
	int err_pipe=0;

	// taking the pipe splitted commands and sending them to command split
	if(no_of_pipe_process == 1){
		args = CommandSplit(main_cmd, &no_of_tokens);
		if((no_of_tokens >0) && (args !=NULL)) {
			status = Execute(args, no_of_tokens);
		}
	}
	else if(no_of_pipe_process > 1){
		int file_desc[2], i ,input_fd = 0;

		for(i=0; i< no_of_pipe_process-1 ; i++){
			if(pipe(file_desc) == -1){
				err_pipe = 1;
				cout << "Error in piping" << endl;
				break;
			}
			else{
				args = CommandSplit(piped_commands[i], &no_of_tokens);
				if(no_of_tokens <=0){
					err_pipe = 1;
					cout << "Error, incorrect syntax!" << endl;
					break;
				}
				else{
					
					// calling execue shell
					status = ExecuteShell(args, no_of_tokens, input_fd,file_desc[1]);
				}
				close(file_desc[1]);
				input_fd = file_desc[0];				
			}
		}
		if(err_pipe == 0){
			args = CommandSplit(piped_commands[i], &no_of_tokens);
			if(no_of_tokens <= 0){
				err_pipe = 1;
				cout << "Error, incorrect syntax!" << endl;
				exit(EXIT_FAILURE);
			}
			else{

				status = ExecuteShell(args, no_of_tokens, input_fd, 1);
			}
		}
	}
	free(args);
	fflush(stdin);
	fflush(stdout);
	return status;
}

int autocomplete(string cmd, vector <string>& toret) {
    //autocomplete function
    //required variables
    string lastword; int n = cmd.size();
    int i = n-1; 
    
    //isolate the last word
    while(i>=0 && cmd[i]!=' ' && cmd[i]!='/') {
        i--;
    }
    i++;

    //retrieve the files in the directory
    vector <string> filesindir;
    DIR *d; struct dirent *dir;
    d = opendir("."); 
    int cur = 0;
    if(d) {
         while ((dir = readdir(d)) != NULL) {
            string temps(dir->d_name);
            filesindir.push_back(temps);
        }
        closedir(d);
    }

    //check for match with files in directory
    for(string s: filesindir) {
        int len = s.size(); bool doesmatch = true;
        for(int j=i;j<n;j++) {
            if(s[j-i]!=cmd[j]) {
                doesmatch = false;
                break;
            }
        }
        if(doesmatch) toret.push_back(s);
    }
    return i;
}

int LCSubStr(string X, string Y)
{
    int m = X.size(); int n = Y.size();
 
    int LCSuff[m + 1][n + 1];
    int result = 0; // To store length of the
                    // longest common substring
    for (int i = 0; i <= m; i++)
    {
        for (int j = 0; j <= n; j++)
        {
            if (i == 0 || j == 0)
                LCSuff[i][j] = 0;
 
            else if (X[i - 1] == Y[j - 1]) {
                LCSuff[i][j] = LCSuff[i - 1][j - 1] + 1;
                result = max(result, LCSuff[i][j]);
            }
            else
                LCSuff[i][j] = 0;
        }
    }
    return result;
}

string searchinhistory() {
    //search in history
    printf("\nEnter a search term: "); //take input

    //get term to search for
    string tosearch; getline(cin,tosearch);

    //required variables
    int maxmatch = -1; string bestmatch;
    //history file name
    string filename = ".mybash_history.txt";
    ifstream fin;
    fin.open(filename); //open history file
    string lastLine; int count = 0;
    if(fin.is_open()) {
        fin.seekg(-2,ios_base::end);                // go to one spot before the EOF

        bool keepLooping = true;
        while(keepLooping) {
            char ch;
            fin.get(ch);   
            // printf("%c",ch);
            lastLine.push_back(ch);                 // Get current byte's data

            if((int)fin.tellg() <= 1) {             // If the data was at or before the 0th byte
                fin.seekg(0);                       // The first line is the last line
                keepLooping = false;                // So stop there
            }
            else if(ch == '\n') {  
                lastLine.pop_back(); reverse(lastLine.begin(),lastLine.end());
                tempint = LCSubStr(lastLine,tosearch);
                if(tempint==tosearch.size() && tempint == lastLine.size()) {
					return lastLine;
                }
                else if(tempint > 2 && tempint > maxmatch) {
                    bestmatch = lastLine; maxmatch = tempint;
                }
                lastLine.clear();
                count++; 
                if(count==(HISTORY_SIZE+cmdsthisinstance)) {
                    keepLooping = false; 
                    continue;
                }               
                fin.seekg(-2,ios_base::cur);   // Stop at the current position.
            }
            else {                                  // If the data was neither a newline nor at the 0 byte
                fin.seekg(-2,ios_base::cur);        // Move to the front of that data, then to the front of the data before it
            }
        }
        fin.close();
    }
	return bestmatch;
}

int ExecuteHistory(char** args) {
    //print last 1000 commands from history
    printf("The previous commands: \n");
    string filename = ".mybash_history.txt";
    ifstream fin;
    fin.open(filename); //open history file
    string lastLine; int count = 0;
    vector <string> cmds;

    //similar method to searchinhistory
    if(fin.is_open()) {
        fin.seekg(-2,ios_base::end);                // go to one spot before the EOF

        bool keepLooping = true;
        while(keepLooping) {
            char ch;
            fin.get(ch);   
            lastLine.push_back(ch);                 // Get current byte's data

            if((int)fin.tellg() <= 1) {             // If the data was at or before the 0th byte
                fin.seekg(0);                       // The first line is the last line
                keepLooping = false;                // So stop there
            }
            else if(ch == '\n') {  
                lastLine.pop_back(); reverse(lastLine.begin(),lastLine.end());
                cmds.push_back(lastLine); lastLine.clear();
                count++; 
                if(count==1000) {
                    keepLooping = false; 
                    continue;
                }               
                fin.seekg(-2,ios_base::cur);   // Stop at the current position.
            }
            else {                                  // If the data was neither a newline nor at the 0 byte
                fin.seekg(-2,ios_base::cur);        // Move to the front of that data, then to the front of the data before it
            }
        }
        reverse(cmds.begin(),cmds.end());
        for(auto s: cmds) cout << s << endl;
        fin.close();
    }
    return EXIT_SUCCESS;
}

//create name of temporary file from PID
string namefrompid(pid_t pid) {
	string tooutput = ".temp."; 
	char stringpid[6]; sprintf(stringpid, "%d", pid);
	tooutput += stringpid; tooutput.append(".txt");
	return tooutput;
}

//execute multiwatch function
int executeMultiWatch(vector <string> cmds, int output_fd) {
    int n = cmds.size();  int status = 0;
    vector <pid_t> childpids;

	int saved_stdout; 

    //output redirection
    if(output_fd != 1){
		saved_stdout = dup(1);
    	dup2(output_fd, 1);
    	close(output_fd);	
    }

	for(auto x: cmds) cout << x << endl;

    //fork a child for each command
    for(int i=0;i<n;i++) {
    	pid_t childpid; 
		childpid = fork();

        //forking error
		if(childpid == -1){
			fprintf(stderr, "Error, child process could not be created\n");
			return EXIT_FAILURE;
		}
    	else if(childpid==0) {
    		pid_t mypid = getpid();
    		string suff = " > "; suff.append(namefrompid(mypid));
    		cmds[i].append(suff); 
    		CompleteExecute(cmds[i]); 
    		exit(0);
    	}
    	else childpids.push_back(childpid);
    }
    fd_set mwatch; FD_ZERO(&mwatch); 
    vector <int> fds; int maxfds = -1; //vector of inotify file descriptors
    //we will use inotify to check for modification of a file
    for(int i=0;i<n;i++) {
    	char filename[17]; strcpy(filename,namefrompid(childpids[i]).c_str());
    	filename[namefrompid(childpids[i]).size()] = '\0';
    	FILE *fp; fp = fopen(filename,"w"); fclose(fp); //create the file if it doesnt exist
    	int inofd = inotify_init(); int watch = inotify_add_watch(inofd,filename,IN_MODIFY);
        //add the filedescriptor
    	fds.push_back(inofd); maxfds = max(maxfds,inofd);
    }
    //select timeout at select
    struct timeval tv;
    tv.tv_sec = 1; tv.tv_usec = 0;
    vector <int> openfds; //open to read from the files
    for(int i=0;i<n;i++) {
    	char filename[17]; strcpy(filename,namefrompid(childpids[i]).c_str());
		int curfile = open(filename,O_RDONLY); openfds.push_back(curfile);
    }
    //while loop to check for temporary file modifications
    do {
    	char pbuf[1024]; 
        //add all the inotify descriptors to watch
    	for(auto x: fds) {
    		FD_SET(x,&mwatch);
    	}
        //select
    	int selout = select(maxfds+1,&mwatch,NULL,NULL,&tv);
        //check if a file is modified, read the data and print
    	for(int i=0;i<n;i++) {
    		if(FD_ISSET(fds[i],&mwatch)) {
    			read(fds[i],pbuf,1024);
				char toread; 
				if(read(openfds[i],&toread,1)==0) continue;
				cout << cmds[i] << " ";
    			time_t rawtime;
				struct tm * timeinfo;

				time ( &rawtime );
				timeinfo = localtime ( &rawtime );
				printf ( "Current local time and date: %s", asctime (timeinfo) );
				printf("<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-<-\n");
				do {
					printf("%c",toread);
				} while(read(openfds[i],&toread,1)!=0);
				printf("->->->->->->->->->->->->->->->->->->->\n");

    		}
    	}
        //set mwatch to zero again
    	FD_ZERO(&mwatch);
    } while(waitpid(-1,NULL,WNOHANG)!=-1);
	dup2(saved_stdout,1); close(saved_stdout); //get the stdout back again
    //close the filedescriptors again
    for(int i=0;i<n;i++) close(openfds[i]);
    //remove the temporary files
    for(auto x: childpids) {
    	char filename[20];  strcpy(filename,namefrompid(x).c_str());
    	filename[namefrompid(x).size()] = '\0';
    	remove(filename); 
		// cout << "Removed " << filename << endl;
    }
    return EXIT_SUCCESS;
}

void sighandler(int signum) {
	printf("\n");
}

void stphandler(int signum) {
    tobackground = true;
}

int ismultiwatchcommand(string main_cmd) {
	vector<string> cmds;
	int redirect_output_fd;
	int status;

	int stdoutcopy = dup(1);

	//find index of square brackets
	int sqrbracopen_index = main_cmd.find('[');
	int sqrbracclose_index = main_cmd.find(']');

	// error check
    if(sqrbracopen_index == string::npos) return EXIT_FAILURE;
    if(sqrbracclose_index == string::npos) return EXIT_FAILURE;
	
	//Slicing the inner commands of multiwatch
	string processed_cmd = main_cmd.substr(sqrbracopen_index+1, sqrbracclose_index- sqrbracopen_index-1);

	size_t pos = 0;

	// tokenize using comma
	if((pos = processed_cmd.find(',')) == string::npos) {

		cmds.push_back(processed_cmd);
	}
	else{

		while((pos = processed_cmd.find(',')) != string::npos){
			string token = processed_cmd.substr(0,pos);

			cmds.push_back(token);
			processed_cmd.erase(0, pos+1);
		}
		cmds.push_back(processed_cmd);
	}
	if(main_cmd.find('>') != string::npos){

		string filename = main_cmd.substr(0, main_cmd.length());

		int index = filename.find('>');
		filename.erase(0, index+1);

		while(filename.find(' ') != string::npos){
			int index = filename.find(' ');
			filename.erase(0, index+1);
		}
		char const* file = filename.c_str();
		redirect_output_fd = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0666);
		//call multiwatch function
		status = executeMultiWatch(cmds, redirect_output_fd);

	}
	else{
		status = executeMultiWatch(cmds, 1);
	}

	//change the stdout printing back to terminal
	dup2(stdoutcopy,1);
	close(stdoutcopy);

	return status;
}



int main(){
	
	int status;

	signal(SIGINT,sighandler);
	ofstream fout;

	do {
		cin.clear(); cout.clear();
		fflush(stdin);
		fflush(stdout);
		cout << ">>> ";
		fout.open(".mybash_history.txt",std::ios_base::app);
		if(!fout.is_open()) {
			fprintf(stderr, "Error, could not open the history file\n");
			exit(-1);
		}
		string main_cmd = readCommand(); 
        if(!main_cmd.empty()) {
            cmdsthisinstance++;
            fout << main_cmd << endl;
        }
		fout.close();

		if(main_cmd.find("multiWatch") != std::string::npos) {

			status = ismultiwatchcommand(main_cmd);

			if(status == EXIT_SUCCESS) continue;
			else {
                cout << "Error in multiWatch format. Please try again" << endl;
                status = EXIT_SUCCESS;
                continue;
			}
		}
		status = CompleteExecute(main_cmd);

	} while(status == EXIT_SUCCESS);
	
	// checking for 10k limit on lines in historyfile before exiting the code
	ifstream fin; fin.open(".mybash_history.txt"); int nooflines = 0; string temps;
	if(!fin.is_open()) {
		fprintf(stderr, "Error, could not open the history file\n");
		exit(-1);
	}
	while(getline(fin,temps)) nooflines++;
	// cout << nooflines << endl; 
	if(nooflines > HISTORY_SIZE) {
		fin.clear(); fin.seekg(0); int count = 0;
		ofstream fout; fout.open(".mybash_historytemp.txt");
		if(!fout.is_open()) {
			fprintf(stderr, "Error, could not open the temporary history file\n");
			exit(-1);
		}
		while(count < (nooflines - HISTORY_SIZE)) {
			getline(fin,temps); count++;
		}
		while(getline(fin,temps)) fout << temps << endl;
		fin.close(); fout.close();
		fout.open(".mybash_history.txt",ofstream::out|ofstream::trunc); 
		fin.open(".mybash_historytemp.txt"); 
		while(getline(fin,temps)) fout << temps << endl;
		fout.close(); fin.close(); remove(".mybash_historytemp.txt");
	}
	else fin.close();
	return 0;
}