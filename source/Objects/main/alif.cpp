#include "Alif.h"
#include "alifcore_initConfig.h"    // AlifArgv
#include "alifCore_alifCycle.h"


static AlifStatus alifMain_init(const AlifArgv* _args) {

	AlifStatus status{};

	AlifPreConfig preConfig{};
	alifPreConfig_initConfig(&preConfig);

	status = alif_preInitializeFromAlifArgv(&preConfig, _args);

	AlifConfig config{};
	alifConfig_initAlifConfig(&config);

	/* pass nullptr as the config: config is read from command line arguments,
   environment variables, configuration files */
	if (_args->useCharArgv) {
		status = alifConfig_setCharArgv(&config, _args->argc, _args->charArgv);
	}
	else {
		status = alifConfig_setArgv(&config, _args->argc, _args->wcharArgv);
	}

	status = alifInit_fromConfig(&config);

	return status;
}


static void alifMain_run(int* _exitcode)
{

}


static int alif_runMain()
{
	int exitcode = 0;

	alifMain_run(&exitcode);

	return exitcode;
}


static int alifMain_main(AlifArgv* _args)
{
	// مرحلة تهيئة البرنامج قبل تشغيله
	AlifStatus status = alifMain_init(_args);


	// مرحلة تشغيل البرنامج
	return alif_runMain();
}


int alif_mainWchar(int _argc, wchar_t** _argv)
{
	AlifArgv args = {
		.argc = _argc,
		.useCharArgv = 0,
		.charArgv = nullptr,
		.wcharArgv = _argv
	};

	return alifMain_main(&args);
}


int alif_mainChar(int _argc, char** _argv)
{
	AlifArgv args = {
		.argc = _argc,
		.useCharArgv = 1,
		.charArgv = _argv,
		.wcharArgv = nullptr
	};

	return alifMain_main(&args);
}


#ifdef MS_WINDOWS
int wmain(int _argc, wchar_t** _argv)
{
	wchar_t* argsv[] = { (wchar_t*)L"alif", (wchar_t*)L"example.alif" };
	return alif_mainWchar(2, argsv);
}
#else
int main(int _argc, char** _argv)
{
	char* argsv[] = { (char*)"alif", (char*)"example.alif" };
	return Alif_MainChar(2, argsv);
}
#endif


