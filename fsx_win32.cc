#include <node.h>
#include <v8.h>
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>

using namespace v8;

Handle<Value> Method(const Arguments& args) {
	HandleScope scope;

	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	int index=0;
	SYSTEMTIME stUTC, stLocal;
	char modifiedTimeStr[32];
	char sizeStr[32];

	if (args.Length() < 1) {
		ThrowException(Exception::TypeError(String::New("Missing path")));
		return scope.Close(Undefined());
	}

	String::Utf8Value value(args[0]->ToString());
	TCHAR* path = *value;

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	/*StringCchLength(path, MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
	{
	_tprintf(TEXT("\nDirectory path is too long.\n"));
	return (-1);
	}*/

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, path);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
	{
		ThrowException(Exception::TypeError(String::New("FindFirstFile failed")));
		return scope.Close(Undefined());
	}

	Local<Array> results = Array::New();
	do
	{
		Local<Object> obj = Object::New();

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			obj->Set(String::New("isDirectory"), String::New("true"));
		}
		else
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;

			// Convert the last-write time to local time.
			FileTimeToSystemTime(&(ffd.ftLastWriteTime), &stUTC);
			SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

			sprintf(modifiedTimeStr, "%02d/%02d/%d %02d:%02d:%02d.%03d", stLocal.wMonth, stLocal.wDay, stLocal.wYear, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds);

			sprintf(sizeStr, "%ld", filesize.QuadPart);

			obj->Set(String::New("modifiedTime"), String::New(modifiedTimeStr));
			obj->Set(String::New("size"), String::New(sizeStr));
		}

		obj->Set(String::New("fileName"), String::New(ffd.cFileName));

		results->Set(index, obj);

		index++;
	}
	while (FindNextFile(hFind, &ffd) != 0);

	DWORD dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) 
	{
		ThrowException(Exception::TypeError(String::New("FindNextFile failed")));
		return scope.Close(Undefined());
	}

	FindClose(hFind);

	return scope.Close(results);
}

void init(Handle<Object> target) {
	target->Set(String::NewSymbol("listDir"),
		FunctionTemplate::New(Method)->GetFunction());
}
NODE_MODULE(fsx_win32, init)
