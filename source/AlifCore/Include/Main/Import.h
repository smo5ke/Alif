#pragma once









//AlifObject* alifImport_addModuleRef(const wchar_t*); // 47



/* --------------------------------------------------------------------------------------- */




class InitTable { // 7
public:
	const char* name{};
	AlifObject* (*initFunc)(void);
};


//extern class InitTable* _alifImportInitTab_; // 12
