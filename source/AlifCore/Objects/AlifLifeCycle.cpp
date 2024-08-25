#include "alif.h"

//#include "AlifCore_FileUtils.h"
//#include "AlifCore_FreeList.h"
//#include "AlifCore_Import.h"
#include "AlifCore_InitConfig.h"
#include "AlifCore_LifeCycle.h"
#include "AlifCore_Memory.h"
#include "AlifCore_State.h"
#include "AlifCore_DureRun.h"
#include "AlifCore_DureRunInit.h"



AlifDureRun _alifDureRun_ = ALIF_DURERUNSTATE_INIT(_alifDureRun_); // 103

static AlifIntT dureRunInitialized = 0; // 110

AlifIntT alifDureRun_initialize() { // 112

	if (dureRunInitialized) return 1;

	dureRunInitialized = 1;

	return alifDureRunState_init(&_alifDureRun_);
}


char* alif_setLocale(AlifIntT category) {  // 332

	char* res;
#ifdef __ANDROID__
	const char* locale;
	const char** pvar;
#ifdef ALIF_COERCE_C_LOCALE
	const char* coerce_c_locale;
#endif
	const char* utf8_locale = "C.UTF-8";
	const char* env_var_set[] = {
		"LC_ALL",
		"LC_CTYPE",
		"LANG",
		nullptr,
	};

	for (pvar = env_var_set; *pvar; pvar++) {
		locale = getenv(*pvar);
		if (locale != nullptr and *locale != '\0') {
			if (strcmp(locale, utf8_locale) == 0 or
				strcmp(locale, "en_US.UTF-8") == 0) {
				return setlocale(category, utf8_locale);
			}
			return setlocale(category, "C");
		}
	}

#ifdef ALIF_COERCE_C_LOCALE
	coerce_c_locale = getenv("ALIFCOERCECLOCALE");
	if (coerce_c_locale == nullptr or strcmp(coerce_c_locale, "0") != 0) {
		if (setenv("LC_CTYPE", utf8_locale, 1)) {
			fprintf(stderr, "Warning: failed setting the LC_CTYPE "
				"environment variable to %s\n", utf8_locale);
		}
	}
#endif
	res = setlocale(category, utf8_locale);
#else
	res = setlocale(category, "");
#endif

	return res;
}


static AlifIntT alifCore_initDureRun(AlifDureRun* _dureRun, const AlifConfig* _config) { // 507

	if (_dureRun->initialized) {
		return -1;
	}

	AlifIntT status = alifConfig_write(_config, _dureRun);
	if (status < 1) return status;

	//status = alifImport_init();

	status = alifInterpreter_enable(_dureRun);
	if (status < 1) return status;

	return 1;
}

static AlifIntT alifCore_createInterpreter(AlifDureRun* _dureRun,
	const AlifConfig* _config, AlifThread** _threadP) { // 637

	AlifIntT status{};
	AlifInterpreter* interpreter{};

	status = alifInterpreter_new(nullptr, &interpreter);
	if (status < 1) return status;

	interpreter->ready = 1;

	status = alifConfig_copy(&interpreter->config, _config);
	if (status < 1) return status;


	if (alifInterpreterMem_init(interpreter) < 1) {
		// memory error
		return -1;
	}

	AlifThread* thread = alifThreadState_new(interpreter);

	if (thread == nullptr) {
		// cant make thread
		printf("%s \n", "لا يمكن إنشاء ممر");
		return -1;
	}

	_dureRun->mainThread = thread;
	alifThread_attach(thread); // temp
	//alifThread_bind(thread);

	*_threadP = thread;
	return 1;
}

static AlifIntT alifCore_builtinsInit(AlifThread* _thread) { // 775

	AlifObject* modules{};
	AlifObject* builtinsDict{};

	AlifInterpreter* interp = _thread->interpreter;

	AlifObject* biMod = alifBuiltin_init(interp);
	//if (biMod == nullptr) goto error;

	////modules = interp->imports.modules_; // alifImport_getModule
	////if (alifImport_fixupBuiltin(_thread, biMod, "builtins", modules) < 0) {
	////	goto error;
	////}

	//builtinsDict = alifModule_getDict(biMod);
	//if (builtinsDict == nullptr) goto error;

	//interp->builtins = ALIF_NEWREF(builtinsDict);

	return 1;

//error:
	//ALIF_XDECREF(biMod);
	return -1;
}

static AlifIntT alifCore_interpreterInit(AlifThread* _thread) { // 843

	AlifInterpreter* interpreter = _thread->interpreter;
	AlifIntT status = 1;
	const AlifConfig* config{};
	AlifObject* sysMod = nullptr;

	//status = alifGC_init(interpreter);
	if (status < 0) return status;

	//status = alifCore_initTypes(interpreter);
	if (status < 0) goto done;

	//status = alifAtExit_init(interpreter);
	if (status < 0) return status;

	//status = alifSys_create(_thread, &sysMod);
	if (status < 0) goto done;

	status = alifCore_builtinsInit(_thread);
	if (status < 0) goto done;

	config = &interpreter->config;

	//status = alifImport_initCore(thread, sysMod, config_->installImportLib);
	if (status < 0) goto done;

done:
	//ALIF_XDECREF(sysmod);
	return status;

}

static AlifIntT alifInit_config(AlifDureRun* _dureRun,
	AlifThread** _threadP, const AlifConfig* _config) { // 917

	AlifIntT status{};

	status = alifCore_initDureRun(_dureRun, _config);
	if (status < 1) return status;

	AlifThread* thread_{};
	status = alifCore_createInterpreter(_dureRun, _config, &thread_);
	if (status < 1) return status;
	*_threadP = thread_;

	status = alifCore_interpreterInit(thread_);
	if (status < 1) return status;

	_dureRun->coreInitialized = 1;
	return 1;
}

static AlifIntT alifInit_core(AlifDureRun* _dureRun,
	const AlifConfig* _config, AlifThread** _threadPtr) { // 1069

	AlifIntT status{};

	AlifConfig config{};
	alifConfig_initAlifConfig(&config);

	status = alifConfig_copy(&config, _config);
	if (status < 1) goto done;

	status = alifConfig_read(&config);
	if (status < 1) goto done;

	if (!_dureRun->coreInitialized) {
		status = alifInit_config(_dureRun, _threadPtr, &config);
	}
	//else {
	//	//status = alifInit_coreReconfig(_dureRun, _tStateP, &config_);
	//}
	if (status < 1) goto done;

done:
	alifConfig_clear(&config);
	return status;
}

static AlifIntT initInterpreter_main(AlifThread* _thread) { // 1156

	AlifIntT status{};
	AlifIntT isMainInterpreter = alif_isMainInterpreter(_thread->interpreter);
	AlifInterpreter* interpreter = _thread->interpreter;
	const AlifConfig* config_ = alifInterpreter_getConfig(interpreter);

	//if (!config_->installImportLib) {
	//	if (isMainInterpreter) {
	//		interpreter->dureRun->initialized = 1;
	//	}
	//	return 1;
	//}

	//status = alifConfig_initImportConfig(&interpreter->config);
	//if (status < 1) return status;

	//if (interpreter_updateConfig(_thread, 1) < 0) {
	//	return -1;
	//}

	//status = alifImport_initExternal(_thread);
	//if (status < 1) return status;

	//status = alifUnicode_initEncoding(_thread);
	//if (status < 1) return status;

	//if (isMainInterpreter) {
	//	if (config_->tracemalloc) {
	//		if (alifTraceMalloc_start(config_->tracemalloc) < 0) {
	//			return -1;
	//		}
	//	}
	//}

	//status = init_sysStream(_thread);
	//if (status < 1) return status;

	//status = init_setBuiltinsOpen();
	//if (status < 1) return status;

	//status = add_mainmodule(interpreter);
	//if (status < 1) return status;

	// need work

	return 1;

}

static AlifIntT alifInit_main(AlifThread* _thread) { // 1363

	AlifInterpreter* interpreter = _thread->interpreter;
	if (!interpreter->dureRun->coreInitialized) {
		// error
		return -1;
	}

	if (interpreter->dureRun->initialized) {
		//return alifInit_mainReconfigure(_thread);
	}

	AlifIntT status = initInterpreter_main(_thread);
	if (status < 1) return status;

	return 1;
}

AlifIntT alif_initFromConfig(const AlifConfig* _config) { // 1383

	if (_config == nullptr) {
		// error
		return -1;
	}

	AlifIntT status{};

	status = alifDureRun_initialize();
	if (status < 1) return status;

	AlifDureRun* dureRun = &_alifDureRun_;
	AlifThread* thread_ = nullptr;

	status = alifInit_core(dureRun, _config, &thread_);
	if (status < 1) return status;

	_config = alifInterpreter_getConfig(thread_->interpreter);

	if (_config->initMain) {
		status = alifInit_main(thread_);
		if (status < 1) return status;
	}

	return 1;
}

