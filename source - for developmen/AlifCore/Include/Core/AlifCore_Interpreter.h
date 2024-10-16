#pragma once

#include "AlifCore_Memory.h"
#include "AlifCore_Function.h"
#include "AlifCore_GC.h"
#include "AlifCore_GlobalObjects.h"
#include "AlifCore_Import.h"
#include "AlifCore_ObjectState.h"
#include "AlifCore_TypeObject.h"




class AlifInterpreter {
public:

	//CEval cEval;

	AlifInterpreter* next{};

	AlifIntT id_{};
	//AlifIntT idRefCount{};
	//AlifIntT reqIDRef{};

	//AlifThreadTypeLock idMutex{};

//#define ALIF_INTERPRETERSTATE_WHENCE_NOTSET -1
//#define ALIF_INTERPRETERSTATE_WHENCE_UNKNOWN 0
//#define ALIF_INTERPRETERSTATE_WHENCE_RUNTIME 1
//#define ALIF_INTERPRETERSTATE_WHENCE_LEGACY_CAPI 2
//#define ALIF_INTERPRETERSTATE_WHENCE_CAPI 3
//#define ALIF_INTERPRETERSTATE_WHENCE_XI 4
//#define ALIF_INTERPRETERSTATE_WHENCE_STDLIB 5
//#define ALIF_INTERPRETERSTATE_WHENCE_MAX 5
//	long _whence;

	AlifIntT initialized{};
	AlifIntT ready{};
	AlifIntT finalizing{};

	class AlifThreads {
	public:
		AlifUSizeT nextUniquID{};
		AlifThread* head{};
		AlifThread* main{};
		AlifUSizeT count{};

		AlifSizeT stackSize{};
	} threads;

	class AlifDureRun* dureRun{};
	AlifConfig config{};

	AlifFrameEvalFunction evalFrame{};

	AlifGCDureRun gc{};

	AlifObject* builtins{};

	class ImportState imports;

	AlifMemory* memory_{};

	AlifObjectState objectState{};

	class TypesState types;

};






AlifIntT alifInterpreter_new(AlifThread*, AlifInterpreter**);
