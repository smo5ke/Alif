#include "alif.h"
#include "AlifCore_AST.h"
#include "AlifCore_AlifState.h"
#include "AlifCore_Integer.h"



class AlifASTOptimize { 
public:
	AlifIntT optimize{};
	AlifIntT recursionDepth{};
	AlifIntT recursionLimit{};
};


static AlifIntT make_const(Expression* _node, AlifObject* _val, AlifASTMem* _astMem) { 
	if (_val == nullptr) {
		// error
		return 1;
	}
	if (alifASTMem_listAddAlifObj(_astMem, _val) < 0) {
		ALIF_DECREF(_val);
		return 0;
	}

	_node->type = ConstantK;
	_node->V.constant.type = nullptr;
	_node->V.constant.val = _val;
	return 1;
}


// Forward
//static AlifIntT astFold_stmt(Statement*, AlifASTMem*, AlifASTOptimize*);
static AlifIntT astFold_keyword(Keyword*, AlifASTMem*, AlifASTOptimize*);

#define CALL(_func, _arg) if (!_func((_arg), _ctx, _astState)) return 0 


#define CALL_SEQ(_func, _type, _fullType, _arg) {\
	AlifIntT i{};	\
	_type ## Seq *seq = (_arg);		\
	for (i = 0; i < SEQ_LEN(seq); i++) { \
		_fullType* elt = (_fullType*)SEQ_GET(seq, i);	\
		if (elt != nullptr and !_func(elt, _ctx, _astState)) return 0;	\
	}		\
}

static AlifIntT fold_binOp(Expression* _node, AlifASTMem* _astMem, AlifASTOptimize* _astState) { 

	Expression* left, *right;

	left = _node->V.binOp.left;
	right = _node->V.binOp.right;

	if (left->type != ConstantK) return 1;

	AlifObject* leftVal = left->V.constant.val;

	// code here

	AlifObject* rightVal = right->V.constant.val;
	AlifObject* newVal = nullptr;

	if (_node->V.binOp.op == Operator::Add) newVal = alifNumber_add(leftVal, rightVal);
	else if (_node->V.binOp.op == Operator::Sub) newVal = alifNumber_subtract(leftVal, rightVal);


	return make_const(_node, newVal, _astMem);

}

static AlifIntT astFold_expr(Expression* _node, AlifASTMem* _ctx, AlifASTOptimize* _astState) { 
	if (++_astState->recursionDepth > _astState->recursionLimit) {
		// error
		return 0;
	}

	if (_node->type == ExprType::BinOpK) {
		CALL(astFold_expr, _node->V.binOp.left);
		CALL(astFold_expr, _node->V.binOp.right);
		CALL(fold_binOp, _node);
	}
	else if (_node->type == ExprType::CallK) {
		CALL(astFold_expr, _node->V.call.func);
		CALL_SEQ(astFold_expr, Expr, Expression, _node->V.call.args);
		CALL_SEQ(astFold_keyword, Keyword, Keyword, _node->V.call.keywords);
	}
	else if (_node->type == ExprType::NameK) {
		// لا شيء
	}

	_astState->recursionDepth--;
	return 1;
}

static AlifIntT astFold_keyword(Keyword* _node, AlifASTMem* _ctx, AlifASTOptimize* _astState) { 
	CALL(astFold_expr, _node->val);
	return 1;
}

static AlifIntT astFold_stmt(Statement* _node, AlifASTMem* _ctx, AlifASTOptimize* _astState) { 
	if (++_astState->recursionDepth > _astState->recursionLimit) {
		// error
		return 0;
	}

	if (_node->type == StmtType::FunctionDefK) {

	}
	else if (_node->type == StmtType::AssignK) {
		CALL_SEQ(astFold_expr, Expr, Expression, _node->V.assign.targets);
		CALL(astFold_expr, _node->V.assign.val);
	}
	else if (_node->type == StmtType::ExprK) {
		CALL(astFold_expr, _node->V.expression.val);
	}

	_astState->recursionDepth--;
	return 1;
}

static AlifIntT astFold_body(StmtSeq* _stmts, AlifASTMem* _ctx, AlifASTOptimize* _astState) { 
	AlifIntT docString = alifAST_getDocString(_stmts) != nullptr;

	CALL_SEQ(astFold_stmt, Stmt, Statement, _stmts);
	if (!docString and alifAST_getDocString(_stmts) != nullptr) {
		Statement* stmt = (Statement*)SEQ_GET(_stmts, 0);
		ExprSeq* vals = alifNew_exprSeq(1, _ctx);
		if (!vals) return 0;

		SEQ_SET(vals, 0, stmt->V.expression.val);
		Expression* expr = alifAST_joinedStr(vals, stmt->lineNo, stmt->colOffset, stmt->endLineNo, stmt->endColOffset, _ctx);
		if (!expr) return 0;
		stmt->V.expression.val = expr;
	}

	return 1;
}

static AlifIntT astFold_module(Module* _node, AlifASTMem* _ctx, AlifASTOptimize* _astState) { 

	if (_node->type == ModType::ModuleK) {
		CALL(astFold_body, _node->V.module.body);
	}
	//else if (_node->type == ModType::InteractiveK) {
	//	CALL_SEQ(astFold_stmt, stmt, _node->V.interactive.body);
	//}
	//else if (_node->type == ModType::ExpressionK) {
	//	CALL(astFold_expr, Expression, _node->V.expression.body);
	//}

	return 1;
}


AlifIntT alifAST_optimize(Module* _module, AlifASTMem* _astMem, AlifIntT _optimize) { 
	AlifThread* thread_{};
	AlifIntT startRecursionDepth{};

	AlifASTOptimize astState{};
	astState.optimize = _optimize;

	thread_ = alifThread_get();
	if (!thread_) return 0;

	// this section is temp and need fix later
	thread_->recursionRemaining = ALIFCPP_RECURSION_LIMIT;
	//////////////////////////////////////////

	AlifIntT recursionDepth = ALIFCPP_RECURSION_LIMIT - thread_->recursionRemaining;
	startRecursionDepth = recursionDepth;
	astState.recursionDepth = startRecursionDepth;
	astState.recursionLimit = ALIFCPP_RECURSION_LIMIT;

	AlifIntT val = astFold_module(_module, _astMem, &astState);
	if (val and astState.recursionDepth != startRecursionDepth) {
		// error
		return 0;
	}

	return val;
}
