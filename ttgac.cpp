#include <windows.h>
#include <stdio.h>

BOOL execute_cmd(TCHAR *cmd) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    if( !CreateProcess( NULL,   // No module name (use command line)
        cmd,            // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        printf( "CreateProcess failed (%d).\n", GetLastError() );
        return FALSE;
    }

    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    
    return TRUE;
}

/*
BOOL TrueTimeCommit(const char *FileName) {
	HANDLE hFile = CreateFile(
		FileName , GENERIC_READ , 0 , NULL ,
		OPEN_EXISTING , FILE_ATTRIBUTE_NORMAL , NULL
	);
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	FILETIME LocalLastWriteTime;
	GetFileTime( hFile, &CreationTime, &LastAccessTime, &LastWriteTime );
	
	SYSTEMTIME LocalTime;
	FileTimeToLocalFileTime(&LastWriteTime,&LocalLastWriteTime);
	FileTimeToSystemTime(&LocalLastWriteTime,&LocalTime);
	printf("%s LastWriteTime= %d”N %dŒŽ %d“ú %dŽž %d•ª %d•b\n",
		FileName,
		LocalTime.wYear , LocalTime.wMonth ,
		LocalTime.wDay , LocalTime.wHour ,
		LocalTime.wMinute , LocalTime.wSecond);
	CloseHandle(hFile);

	return TRUE;
}
*/

void GitAdd(char *filepath) {
	char cmd[_MAX_PATH  + 256];
	wsprintf(cmd,"git add %s",filepath);
	execute_cmd(cmd);
}

void GitCommit(const char *comment) {
	char *cmd = new char[strlen(comment) + 256];
	wsprintf(cmd,"git commit -m \"%s\"",comment);
	execute_cmd(cmd);
}

BOOL SearchDirectoryAndGit(char *path, const char *comment) {
	char subpath[_MAX_PATH];
	char temp[_MAX_PATH];
	WIN32_FIND_DATA fd;
	
	strcpy(temp,path);

	// initialize FindFirstFile API
	HANDLE hFind = FindFirstFile(temp,&fd);
	
	// remove last '*'
	temp[strlen(temp) - 1] = '\0';
	
	if (hFind == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	do {
//		printf(" '%-64s' %d",fd.cFileName,fd.dwFileAttributes);
		if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		    && strcmp(fd.cFileName,"..")
		    && strcmp(fd.cFileName,".")) {
			// Found directory. search recursively

			// skip .git directory
			if(!strcmp(fd.cFileName,".git")) {
//				printf("........ skip .git\n");
				continue;
			}
//			printf("........ [dir]\n");

			wsprintf(subpath,"%s%s\\*",temp,fd.cFileName);
			printf("Enter sub directry %s\n", subpath);
			if(!SearchDirectoryAndGit(subpath,comment)) return FALSE;
		} else {
			// Found file.
			SYSTEMTIME LocalTime;
			FILETIME LocalLastWriteTime;
			// convert file time to local
			FileTimeToLocalFileTime(&fd.ftLastWriteTime,&LocalLastWriteTime);
			// convert local filetime to systemtime
			FileTimeToSystemTime(&LocalLastWriteTime,&LocalTime);

			if(!strcmp(fd.cFileName,".") || !strcmp(fd.cFileName,"..") || !strcmp(fd.cFileName,".git")) {
//				printf("........ (skip)\n");
				continue;
			}

			char targetpath[_MAX_PATH];
			wsprintf(targetpath,"%s%s",temp,fd.cFileName);
			printf("  %-64s  %04d/%02d/%02d %02d:%02d:%02d\n",
				targetpath,
				LocalTime.wYear , LocalTime.wMonth ,
				LocalTime.wDay , LocalTime.wHour ,
				LocalTime.wMinute , LocalTime.wSecond);

			// git add found file
			GitAdd(targetpath);

			// change system time to file timestamp
			SYSTEMTIME CurrentTime;
			GetLocalTime(&CurrentTime);
			if(!SetLocalTime(&LocalTime)) {
				printf("SetLocalTime to FileTime failed");
				return FALSE;
			}
		
			// commit!
			GitCommit(comment);

			// restore system time to current time
			if(!SetLocalTime(&CurrentTime)) {
				printf("** Warning ** Restore Current Time failed ** ");
				return FALSE;
			}
		}
	} while (FindNextFile(hFind, &fd));

	FindClose(hFind);
	return TRUE;
}

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("TrueTime Git add and commit. (C)2018 Tokushu Denshi Kairo Inc.\n");
		printf("Usage: ttgac targetpath [comment]\n");
		return 1;
	}
	char *comment;
	if(argc >= 3) {
		comment = strdup(argv[2]);
		printf("comment '%s'\n",comment);
	}
	else {
		comment = "truetime commit";
	}

	// set current directory to target path
	if( !SetCurrentDirectory(argv[1])) {
		printf("SetCurrentDirectory to \"%s\" failed\n",argv[1]);
		return 1;
	}
	execute_cmd("git init");

	// execute!!
	SearchDirectoryAndGit("*",comment);

	free(free);

	return 0;
}

