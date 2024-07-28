#include "alif.h"

#include "AlifCore_GetConsoleLine.h"
#include "AlifCore_Memory.h"
#include "AlifCore_InitConfig.h"
#include "AlifCore_Interpreter.h"
#include "AlifCore_AlifCycle.h"
#include "AlifCore_AlifState.h"
#include "AlifCore_DureRun.h"

#ifdef _WINDOWS
#include <windows.h>
#include <fcntl.h>
#else
#include <unistd.h>
typedef uint32_t DWORD;
#define MAX_PATH 260
#endif

#define MAXPATHLEN 256

#pragma warning(disable : 4996) // for disable unsafe functions error

#define ALIF_ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

static const wchar_t usageLine[] =
L"usage: %ls [option] ... [-c cmd | -m mod | file | - ] [arg] ...\n";

static const wchar_t usageHelp[] = L"\
-c cmd : program passed in as string (terminates option list)\n\
-m mod : run library module as a script (terminates option list)\n\
file   : program read from script file\n\
-      : program read from stdin (default; interactive mode if a tty)\n\
arg ...: arguments passed to program in sys.argv[1:]\n\
-h     : print this help message and exit (--help)\n\
-v     : print Alif version number and exit (also --version)\n\
";

static void config_usage(const wchar_t* program)
{
	FILE* f = stdout;

	fwprintf(f, usageLine, program);

	fputws(usageHelp, f);
}


AlifIntT alif_setLocaleAndWChar() {
#ifdef _WINDOWS
	bool modeIn = _setmode(_fileno(stdin), _O_WTEXT);
	bool modeOut = _setmode(_fileno(stdout), _O_WTEXT);
	if (!modeIn or !modeOut) {
		std::wcout << L"لم يستطع تهيئة الطرفية لقراءة الأحرف العربية" << std::endl;
		return -1;
	}
#endif // _WINDOWS

	const char* locale = setlocale(LC_ALL, "en_US.UTF-8");
	if (locale == nullptr) {
		std::wcout << L"لم يستطع تهيئة الموقع، تأكد من تثبيت ar.utf-8 على نظامك" << std::endl;
		return -1;
	}

	setlocale(LC_ALL, locale);
	return 1;
}

AlifIntT alifArgv_asWStrList(AlifConfig* _config, AlifArgv* _args) {
	if (_args->useBytesArgv)
	{
		AlifWStringList wArgv = { 0, nullptr };
		wArgv.items = (wchar_t**)alifMem_dataAlloc(_args->argc * sizeof(wchar_t*) + 2);

		for (int i = 0; i < _args->argc; i++) {
			size_t len = mbstowcs(nullptr, (const char*)_args->bytesArgv[i], 0);
			wchar_t* arg = (wchar_t*)alifMem_dataAlloc(len * sizeof(wchar_t) + 2);
			mbstowcs(arg, (const char*)_args->bytesArgv[i], len);
			wArgv.items[i] = arg;
			wArgv.length++;
		}
		_config->argv = wArgv;
		_config->origArgv = wArgv;

	}
	else {
		_config->argv.length = _args->argc;
		_config->argv.items = (wchar_t**)_args->wcharArgv;

		_config->origArgv.length = _args->argc;
		_config->origArgv.items = (wchar_t**)_args->wcharArgv;
	}

	return 1;
}


void alifConfig_initAlifConfig(AlifConfig* _config) {

	_config->configInit = ConfigInitEnum::AlifConfig_Init_Alif;
	_config->tracemalloc = -1;
	_config->parseArgv = 1;
	_config->interactive = 0;
	_config->optimizationLevel = 0;
	_config->quite = 0;
}


static AlifIntT parse_consoleLine(AlifConfig* _config, AlifSizeT* _index) {

	AlifIntT status{};

	AlifIntT printVer{};
	const AlifWStringList* argv = &_config->argv;
	const wchar_t* program = _config->programName;
	if (!program and argv->length >= 1) {
		program = argv->items[0];
	}

	alif_resetConsoleLine();
	do {
		AlifIntT c = alif_getConsoleLine(argv->length, argv->items);

		if (c == EOF) {
			break;
		}

		if (c == L'c') {
			if (_config->runCommand == nullptr) {
				AlifUSizeT len = wcslen(optArg) + 2 + 2;
				wchar_t* command = (wchar_t*)alifMem_dataAlloc(len * sizeof(wchar_t));
				if (command == nullptr) {
					return -1;
				}
				memcpy(command, optArg, (len - 2) * sizeof(wchar_t));
				command[len - 2] = L'\n';
				command[len - 1] = 0;
				_config->runCommand = command;
			}
			break;
		}

		if (c == L'm') {
			if (_config->runModule == nullptr) {
				AlifUSizeT len = wcslen(optArg) * sizeof(wchar_t) + 2 + 2;
				_config->runModule = (wchar_t*)alifMem_dataAlloc(len);
				memcpy(_config->runModule, optArg, len);
				if (_config->runModule == nullptr) {
					return -1;
				}
			}
			break;
		}

		if (c == 0) {
			//config_complete_usage(program);
			exit(1);
		}
		else if (c == 1) {
			//config_envvars_usage();
			exit(1);
		}
		else if (c == L'h') {
			config_usage(program);
			exit(1);
		}
		else if (c == L'v') {
			printVer++;
			break;
		}
		else {
			//config_usage(1, program);
			exit(1);
		}

	} while (true);



	if (printVer) {
		wprintf(L"alif %ls\n", alif_getVersion());
		exit(1);
	}

	if (_config->runModule == nullptr and _config->runCommand == nullptr
		and optIdx < argv->length and wcscmp(argv->items[optIdx], L"-") != 0
		and _config->runFilename == nullptr) {
		AlifSizeT size = wcslen(argv->items[optIdx]) * sizeof(wchar_t);
		_config->runFilename = (wchar_t*)alifMem_dataAlloc(size + 2);
		memcpy(_config->runFilename, argv->items[optIdx], size);
	}

	if (_config->runCommand != nullptr or
		_config->runModule != nullptr) {
		optIdx--;
	}

	_config->programName = (wchar_t*)program;

	*_index = optIdx;
}

static AlifIntT update_argv(AlifConfig* _config, AlifSizeT _index) {
	const AlifWStringList* cmdlineArgv = &_config->argv;
	AlifWStringList configArgv = { 0, nullptr };
	if (cmdlineArgv->length <= _index) {
		wchar_t* append = (wchar_t*)alifMem_dataAlloc(sizeof(L""));
		*append = L'\0';
		configArgv.items = &append;
		configArgv.length++;
	}
	else {
		AlifWStringList slice{};
		slice.length = cmdlineArgv->length - _index;
		slice.items = &cmdlineArgv->items[_index];
		configArgv = slice;
	}

	wchar_t* arg1{};
	if (_config->runCommand != nullptr) {
		arg1 = (wchar_t*)L"-c";
	}
	else if (_config->runModule != nullptr) {
		arg1 = (wchar_t*)L"-m";
	}

	if (arg1 != nullptr) {
		configArgv.items[0] = arg1;
	}

	_config->argv = configArgv;

	return 1;
}

static int alif_extension(wchar_t* _filename) {
	const wchar_t* dotPos = wcschr(_filename, L'.');
	if (!dotPos) { return 0; }

	const wchar_t* suffix = wcsstr(_filename, dotPos + 1);
	if (suffix) { return wcscmp(suffix, L"alif") == 0; }
	else { return 0; }
}

static AlifIntT run_absPathFilename(AlifConfig* _config) {

	wchar_t* filename = _config->runFilename;
	// في حال عدم وجود ملف يجب تشغيله، لا تقم بجلب المسار
	if (!filename) { return -1; }

	// يتم التحقق من لاحقة الملف والتي يجب ان تكون .alif
	if (!alif_extension(filename)) {
		wprintf(L"%ls", L"تأكد من لاحقة الملف \n يجب ان ينتهي اسم الملف بـ .alif");
		exit(-1);
	}

	wchar_t* absFilename{};

#ifdef _WINDOWS
	wchar_t wOutBuf[MAX_PATH]{}, * wOutBufP = wOutBuf;
	DWORD result{};
	result = GetFullPathNameW(filename, ALIF_ARRAY_LENGTH(wOutBuf), wOutBuf, nullptr);

	absFilename = (wchar_t*)alifMem_dataAlloc(result * sizeof(wchar_t) + 2);
	memcpy(absFilename, wOutBuf, result * sizeof(wchar_t));
#else
	char buf[MAXPATHLEN + 1]{};

	char* cwd = getcwd(buf, sizeof(buf));
	size_t len = mbstowcs(nullptr, cwd, 0);
	wchar_t* wCwd = (wchar_t*)alifMem_dataAlloc(len * sizeof(wchar_t) + 2);
	mbstowcs(wCwd, cwd, len);

	size_t cwdLen = wcslen(wCwd);
	size_t pathLen = wcslen(filename);
	size_t length = cwdLen + 1 + pathLen + 1;

	absFilename = (wchar_t*)alifMem_dataAlloc(length * sizeof(wchar_t));

	wchar_t* absPath = absFilename;
	memcpy(absPath, wCwd, cwdLen * sizeof(wchar_t));
	absPath += cwdLen;

	*absPath = (wchar_t)L'/';
	absPath++;

	memcpy(absPath, filename, pathLen * sizeof(wchar_t));
	absPath += pathLen;

	*absPath = 0;
#endif // _WINDOWS

	_config->runFilename = absFilename;

	return 1;
}

static AlifIntT config_readConsole(AlifConfig* _config) {

	AlifIntT status{};

	if (_config->parseArgv < 0) _config->parseArgv = 1;

	if (_config->parseArgv == 1) {

		AlifSizeT index{};

		// تقوم هذه الدالة بتحليل سطر الطرفية
		status = parse_consoleLine(_config, &index);
		if (status < 1) return status;

		// تقوم هذه الدالة بجلب المسار الخاص بتنفيذ الملف فقط
		status = run_absPathFilename(_config);
		if (status < 1) return status;

		// تقوم هذه الدالة بتحديث المعطيات الممررة عبر الطرفية
		status = update_argv(_config, index);
		if (status < 1) return status;

	}
	else {
		// تقوم هذه الدالة بجلب المسار الخاص بتنفيذ الملف فقط
		status = run_absPathFilename(_config);
		if (status < 1) return status;
	}

	return 1;
}






AlifIntT alifConfig_write(const AlifConfig* _config, AlifDureRun* _dureRun) {

	//config_initStdio(_config); // for set arabic char reading in console

	memcpy(&_alifDureRun_.origArgv, &_config->argv, sizeof(AlifWStringList)); // يجب مراجعتها

	return 1;
}



AlifIntT alifConfig_read(AlifConfig* _config) {

	AlifIntT status{};

	if (_config->origArgv.length == 0 and !(_config->argv.length == 1
		and wcscmp(_config->argv.items[0], L"") == 0))
	{
		//if (alifWideStringList_copy(&_config->origArgv, &_config->argv) < 0) {
			//return ALIFSTATUS_NO_MEMORY();
		//}
		return -1; // temp
	}

	status = config_readConsole(_config);
	if (status < 1) return status;

	if (_config->tracemalloc < 0) _config->tracemalloc = 0;

	if (_config->argv.length < 1) {
		// error
	}


	if (_config->parseArgv == 1) _config->parseArgv = 2;

	return 1;
}
