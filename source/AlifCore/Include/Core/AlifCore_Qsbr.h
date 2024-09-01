#pragma once

#include "AlifCore_Lock.h"


// 28
#define QSBR_OFFLINE 0
#define QSBR_INITIAL 1
#define QSBR_INCR    2


#define QSBR_LT(a, b) ((int64_t)((a)-(b)) < 0) // 35
#define QSBR_LEQ(a, b) ((int64_t)((a)-(b)) <= 0) // 36


class QSBRShared; // 38
class AlifThreadImpl; // 39


class QSBRThreadState { // 42
public:
	uint64_t seq{};
	QSBRShared* shared{};
	QSBRThreadState* tstate{};
	AlifIntT deferrals{};
	bool allocated{};
	QSBRThreadState* freeListNext{};
};


class QSBRPad { // 61
public:
	QSBRThreadState qsbr{};
	char __padding[64 - sizeof(QSBRThreadState)];
};

class QSBRShared { // 67
public:
	uint64_t wrSeq{};
	uint64_t rdSeq{};
	QSBRPad* array{};
	AlifSizeT size{};
	AlifMutex mutex{};
	QSBRThreadState* freeList{};
};





static inline uint64_t alifQSBR_sharedCurrent(QSBRShared* shared) { // 83
	return alifAtomic_loadUint64Acquire(&shared->wrSeq);
}


uint64_t alifQSBR_deferredAdvance(class QSBRThreadState*); // 117

bool alifQSBR_poll(class QSBRThreadState*, uint64_t); // 122

extern void alifQSBR_attach(QSBRThreadState*); // 125
extern void alifQSBR_detach(QSBRThreadState*); // 130
