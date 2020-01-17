#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include "../../util/log.h"
#include "../../util/mem.h"
#include "../../util/tree.h"
#include "llvm-code-gen.h"

struct TypeLLVM {
	LLVMTypeRef type;
	LLVMValueRef init;
	struct Tree **builds;
	struct Tree **methods;
};

struct TreeList {
	unsigned int treeCount;
	struct Tree strong *strong *trees;
};

void treeListCreate(struct TreeList weak *ret) {
	ret->treeCount = 0;
	ret->trees = NULL;
}

// list takes ownership of tree
void treeListPush(struct TreeList *this, struct Tree *tree) {
	this->treeCount++;
	this->trees = memRealloc(this->trees, this->treeCount * sizeof(struct Tree *));
	this->trees[this->treeCount - 1] = tree;
}

// kills the tree
void treeListPop(struct TreeList weak *this) {
	this->treeCount--;
	treeDel(this->trees[this->treeCount]);
	this->trees = memRealloc(this->trees, this->treeCount * sizeof(struct Tree *));
}

struct Tree weak *treeListPeak(struct TreeList const weak *self) {
	return self->trees[self->treeCount - 1];	
}

// does not free list
void treeListDel(struct TreeList strong *this) {
	for (int i = 0; i < (int) this->treeCount; i++) {
		treeDel((this->trees[i]));
	}
}


LLVMTypeRef codeGenTypeLLVM(LLVMModuleRef module, struct Tree **localTypes, struct Type *type) {
	logMsg(LOG_INFO, 2, "Started Type Code Gen");
	LLVMTypeRef ret;
	switch (type->type) {
		case BUILDIN_TYPE:;
			struct BuildinType *buildinType = (struct BuildinType *) type;
			switch(buildinType->type) {
				case INT_BUILDIN_TYPE:
					ret = LLVMPointerType(((struct TypeLLVM *) treeLookUp(*localTypes, "Int"))->type, false);
					break;
				case STRING_BUILDIN_TYPE:
					//ret = LLVMPointerType(LLVMIntType(8), 0);
					ret = LLVMPointerType(((struct TypeLLVM *) treeLookUp(*localTypes, "String"))->type, false);
					//ret = LLVMIntType(32);
					break;
				case VOID_BUILDIN_TYPE:
					ret = LLVMVoidType();
					break;
				default:
					logMsg(LOG_ERROR, 4, "Unimplemented Buildin Type");
					exit(-1);
			}
			break;
		case SIMPLE_TYPE: {
				struct SimpleType *simpleType = (struct SimpleType *) type;
				if (!strcmp(simpleType->name, "Void")) {
					ret = LLVMVoidType();
				} else {
					ret = LLVMPointerType(((struct TypeLLVM *) treeLookUp(*localTypes, simpleType->name))->type, false);
				}
				break;
			}
		default:
			logMsg(LOG_ERROR, 4, "Invalid Type!");
			exit(-1);
	}
	logMsg(LOG_INFO, 2, "Finished Type Code Gen");
	return ret;
}

LLVMValueRef codeGenIntLiteralLLVM(LLVMBuilderRef builder, struct Tree **localTypes, struct IntLiteral *lit) {
	logMsg(LOG_INFO, 1, "Generating Int Literal with value %d", lit->val);
	LLVMValueRef cInt = LLVMConstInt(LLVMIntType(32), lit->val, false);
	struct TypeLLVM *intType = treeLookUp(*localTypes, "Int");
	assert(intType != NULL);
	LLVMValueRef intObj = LLVMBuildCall(builder, intType->init, NULL, 0, "");

	LLVMValueRef method = treeLookUp(*intType->methods, "_literal");

	assert(method != NULL);
	
	LLVMValueRef literalArgs[2] = {intObj, cInt};
	LLVMBuildCall(builder, method, literalArgs, 2, "");

	//LLVMValueRef litFunc = LLVMBuildLoad(builder, LLVMBuildStructGEP(builder, intObj, 2, ""), "");


	/*LLVMTypeRef literalArgTypes[2] = {LLVMPointerType(intType->type, false), LLVMIntType(32)};

	litFunc = LLVMBuildBitCast(builder, litFunc, LLVMPointerType(LLVMFunctionType(LLVMVoidType(), literalArgTypes, 2, false), false), "");*/

	logMsg(LOG_INFO, 1, "Generated Int Literal", lit->val);
	return intObj;
}


// Spaghetti code fix later
LLVMValueRef codeGenStringLiteralLLVM(LLVMBuilderRef builder, struct Tree **localTypes, struct StringLiteral *lit) {
	logMsg(LOG_INFO, 1, "Generating String Literal with value %s", lit->val);
	LLVMValueRef cString = LLVMBuildMalloc(builder, LLVMArrayType(LLVMIntType(8), strlen(lit->val) + 1), "");
	LLVMBuildStore(builder, LLVMConstString(lit->val, strlen(lit->val), false), cString);
	cString = LLVMBuildBitCast(builder, cString, LLVMPointerType(LLVMIntType(8), false), "");

	struct TypeLLVM *stringType = treeLookUp(*localTypes, "String");
	LLVMValueRef stringObj = LLVMBuildCall(builder, stringType->init, NULL, 0, "");

	LLVMValueRef literalArgs[2] = {stringObj, cString};
	LLVMBuildCall(builder, treeLookUp(*stringType->methods, "_literal"), literalArgs, 2, "");

	
	logMsg(LOG_INFO, 1, "Generated String Literal");
	return stringObj;
}

LLVMValueRef codeGenCharLiteralLLVM(LLVMBuilderRef builder, struct Tree **localTypes, struct CharLiteral *lit) {
	logMsg(LOG_INFO, 1, "Generating Int Literal with value %d", lit->val);
	LLVMValueRef cInt = LLVMConstInt(LLVMIntType(8), lit->val, false);
	struct TypeLLVM *intType = treeLookUp(*localTypes, "Char");
	LLVMValueRef intObj = LLVMBuildCall(builder, intType->init, NULL, 0, "");

	LLVMValueRef literalArgs[2] = {intObj, cInt};
	LLVMBuildCall(builder, treeLookUp(*intType->methods, "_literal"), literalArgs, 2, "");

	//LLVMValueRef litFunc = LLVMBuildLoad(builder, LLVMBuildStructGEP(builder, intObj, 2, ""), "");


	/*LLVMTypeRef literalArgTypes[2] = {LLVMPointerType(intType->type, false), LLVMIntType(32)};

	litFunc = LLVMBuildBitCast(builder, litFunc, LLVMPointerType(LLVMFunctionType(LLVMVoidType(), literalArgTypes, 2, false), false), "");*/
	logMsg(LOG_INFO, 1, "Generated Int Literal", lit->val);
	return intObj;
}

LLVMValueRef codeGenByteLiteralLLVM(LLVMBuilderRef builder, struct Tree **localTypes, struct ByteLiteral *lit) {
	logMsg(LOG_INFO, 1, "Generating Byte Literal with value %d", lit->val);
	LLVMValueRef cInt = LLVMConstInt(LLVMIntType(8), lit->val, false);
	struct TypeLLVM *intType = treeLookUp(*localTypes, "Byte");
	LLVMValueRef intObj = LLVMBuildCall(builder, intType->init, NULL, 0, "");

	LLVMValueRef literalArgs[2] = {intObj, cInt};
	LLVMBuildCall(builder, treeLookUp(*intType->methods, "_literal"), literalArgs, 2, "");

	//LLVMValueRef litFunc = LLVMBuildLoad(builder, LLVMBuildStructGEP(builder, intObj, 2, ""), "");


	/*LLVMTypeRef literalArgTypes[2] = {LLVMPointerType(intType->type, false), LLVMIntType(32)};

	litFunc = LLVMBuildBitCast(builder, litFunc, LLVMPointerType(LLVMFunctionType(LLVMVoidType(), literalArgTypes, 2, false), false), "");*/
	logMsg(LOG_INFO, 1, "Generated Byte Literal", lit->val);
	return intObj;
}

LLVMValueRef codeGenBoolLiteralLLVM(LLVMBuilderRef builder, struct Tree **localTypes, struct BoolLiteral *lit) {
	logMsg(LOG_INFO, 1, "Generating Bool Literal with value %d", lit->val);
	LLVMValueRef cInt = LLVMConstInt(LLVMIntType(8), lit->val, false);
	struct TypeLLVM *intType = treeLookUp(*localTypes, "Bool");
	LLVMValueRef intObj = LLVMBuildCall(builder, intType->init, NULL, 0, "");

	LLVMValueRef literalArgs[2] = {intObj, cInt};
	LLVMBuildCall(builder, treeLookUp(*intType->methods, "_literal"), literalArgs, 2, "");

	//LLVMValueRef litFunc = LLVMBuildLoad(builder, LLVMBuildStructGEP(builder, intObj, 2, ""), "");


	/*LLVMTypeRef literalArgTypes[2] = {LLVMPointerType(intType->type, false), LLVMIntType(32)};

	litFunc = LLVMBuildBitCast(builder, litFunc, LLVMPointerType(LLVMFunctionType(LLVMVoidType(), literalArgTypes, 2, false), false), "");*/
	logMsg(LOG_INFO, 1, "Generated Bool Literal", lit->val);
	return intObj;
}



LLVMValueRef codeGenLiteralExprLLVM(LLVMBuilderRef builder, struct Tree **localTypes, struct LiteralExpr *expr) {
	switch (expr->type) {
		case INT_LITERAL:
			return codeGenIntLiteralLLVM(builder, localTypes, (struct IntLiteral *) expr);
		case STRING_LITERAL:
			return codeGenStringLiteralLLVM(builder, localTypes, (struct StringLiteral *) expr);
		case CHAR_LITERAL:
			return codeGenCharLiteralLLVM(builder, localTypes, (struct CharLiteral *) expr);
		case BYTE_LITERAL:
			return codeGenByteLiteralLLVM(builder, localTypes, (struct ByteLiteral weak *) expr);
		case BOOL_LITERAL:
			return codeGenBoolLiteralLLVM(builder, localTypes, (struct BoolLiteral weak *) expr);
		default:
			logMsg(LOG_ERROR, 4, "Invalid Literal Type!");
			exit(-1);
	}
}

LLVMValueRef codeGenExprLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct Expr *expr);

LLVMValueRef codeGenBinaryExprLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct BinaryExpr *expr) {
	LLVMValueRef ret;
	switch (expr->op) {
		case ADD_OP:
			ret = LLVMBuildAdd(builder, codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->lhs), codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->rhs), "");
			break;
		case SUB_OP:
			ret = LLVMBuildSub(builder, codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->lhs), codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->rhs), "");
			break;
		case MUL_OP:
			ret = LLVMBuildMul(builder, codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->lhs), codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->rhs), "");
			break;
		case DIV_OP:
			ret = LLVMBuildSDiv(builder, codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->lhs), codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->rhs), "");
			break;
		default:
			logMsg(LOG_ERROR, 4, "Unimplemented Op");
			exit(EXIT_FAILURE);
	}
	return ret;
}

LLVMValueRef codeGenVarExprLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct VarExpr *expr) {
	LLVMValueRef ret = NULL;
	for (int i = localVarSymbols->treeCount; ret == NULL && i > 0; i--) {

		ret = treeLookUp(localVarSymbols->trees[i - 1], expr->name);
	}

	if (ret == NULL) {
		logMsg(LOG_ERROR, 4, "Variable '%s' not defined in this scope!", expr->name);
		exit(-1);
	}
	return ret;
}

LLVMValueRef codeGenCallExprLLVM(LLVMBuilderRef builder, struct TreeList *varList, struct Tree **funcs, struct Tree **types, struct CallExpr *expr) {	
	LLVMValueRef ret = treeLookUp(*funcs, expr->name);
	if (ret == NULL) {
		logMsg(LOG_ERROR, 1, "Function '%s' not defined in this scope!", expr->name);
		logMsg(LOG_INFO, 1, "Assuming constructor");
		struct TypeLLVM *classType = treeLookUp(*types, expr->name);
		LLVMValueRef buildFunc = treeLookUp(*classType->builds, "_build");
		ret = LLVMBuildCall(builder, classType->init, NULL, 0, "build");
		LLVMValueRef args[expr->argCount + 1];
		args[0] = ret;
		for (int i = 0; i < (int) expr->argCount; i++) {
			args[i + 1] = codeGenExprLLVM(builder, varList, funcs, types,  expr->args[i]);
		}
	
		LLVMBuildCall(builder, buildFunc, args, expr->argCount+1, "");
	} else {
		LLVMValueRef args[expr->argCount];
		for (int i = 0; i < (int) expr->argCount; i++) {
			args[i] = codeGenExprLLVM(builder, varList, funcs, types,  expr->args[i]);
		}
		ret = LLVMBuildCall(builder, ret, args, expr->argCount, "");
	}
	return ret;
}

LLVMValueRef codeGenMethodCallLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct MethodCallExpr *expr) {
	logMsg(LOG_INFO, 2, "Building Method Call");
	LLVMValueRef ret;
	LLVMValueRef lhs = codeGenExprLLVM(builder, localVarSymbols, funcs, types, expr->lhs);
	LLVMTypeRef lhsType = LLVMGetElementType(LLVMTypeOf(lhs));
	LLVMDumpType(lhsType);
	char const *lhsTypeName = LLVMGetStructName(lhsType);
	logMsg(LOG_INFO, 1, "Name: %s", lhsTypeName);
	struct TypeLLVM *type = (struct TypeLLVM *) treeLookUp(*types, lhsTypeName);
	LLVMValueRef func = (LLVMValueRef) treeLookUp(*type->methods, expr->name);
	if (func != NULL) {
		LLVMValueRef args[expr->argCount+1];
		args[0] = lhs;
		for (int i = 0; i < (int) expr->argCount; i++) {
			args[i + 1] = codeGenExprLLVM(builder, localVarSymbols, funcs, types,  expr->args[i]);
		}

		ret = LLVMBuildCall(builder, func, args, expr->argCount + 1, "");
	} else {
		ret = NULL;
		logMsg(LOG_ERROR, 4, "Method '%s' doesn't exist for RHS", expr->name);	
		exit(EXIT_FAILURE);
	}
	logMsg(LOG_INFO, 2, "Build Method Call");
	return ret;
}

LLVMValueRef codeGenExprLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct Expr *expr) {
	LLVMValueRef ret;
	switch (expr->type) {
		case LITERAL_EXPR:
			logMsg(LOG_INFO, 2, "Building Literal Expression!");
			ret = codeGenLiteralExprLLVM(builder, types, (struct LiteralExpr *) expr);
			break;
		case BINARY_EXPR:
			logMsg(LOG_INFO, 2, "Building Binary Expression!");
			ret = codeGenBinaryExprLLVM(builder, localVarSymbols, funcs, types, (struct BinaryExpr *) expr);
			break;
		case VAR_EXPR:
			logMsg(LOG_INFO, 2, "Building Var Expression!");
			ret = codeGenVarExprLLVM(builder, localVarSymbols, (struct VarExpr *) expr);
			logMsg(LOG_INFO, 2, "Finished Building Var Expression!");
			break;
		case CALL_EXPR:
			ret = codeGenCallExprLLVM(builder, localVarSymbols, funcs, types, (struct CallExpr *) expr);
			break;
		case METHOD_CALL_EXPR:
			ret = codeGenMethodCallLLVM(builder, localVarSymbols, funcs, types, (struct MethodCallExpr *) expr);
			break;
		default:
			logMsg(LOG_ERROR, 4, "Invalid Expression Type!");
			exit(-1);
	}
	return ret;
}

LLVMValueRef codeGenExprStmtLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct ExprStmt *stmt) {
	return codeGenExprLLVM(builder, localVarSymbols, funcs, types, stmt->expr);
}

LLVMValueRef codeGenVarStmtLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct VarStmt *stmt) {
	logMsg(LOG_INFO, 1, "Creating Var with name %s", stmt->var->name);
	LLVMValueRef rhs = codeGenExprLLVM(builder, localVarSymbols, funcs, types, stmt->init);
logMsg(LOG_WARNING, 4, "== %s", stmt->var->name);
	localVarSymbols->trees[localVarSymbols->treeCount - 1] = treeAdd(treeListPeak(localVarSymbols), heapString(stmt->var->name), rhs);
	return rhs; 
}

LLVMValueRef codeGenAssignStmtLLVM(LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct AssignStmt *stmt) {
	LLVMValueRef rhs = codeGenExprLLVM(builder, localVarSymbols, funcs, types, stmt->init);
	localVarSymbols->trees[localVarSymbols->treeCount - 1] = treeAdd(localVarSymbols->trees[localVarSymbols->treeCount - 1], heapString(stmt->var->name), rhs);
	return rhs;
}

LLVMValueRef codeGenNullStmtLLVM(LLVMBuilderRef builder, struct NullStmt *stmt) {
	return LLVMBuildRet(builder, LLVMBuildAdd(builder, LLVMConstInt(LLVMIntType(32), 0, false), LLVMConstInt(LLVMIntType(32), 0, false), "no-op"));
}

LLVMValueRef codeGenStmtLLVM(LLVMValueRef functionRef, LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct Stmt *stmt);

LLVMValueRef codeGenBlockStmtLLVM(LLVMValueRef functionRef, LLVMBuilderRef builder, struct TreeList *list, struct Tree **funcs, struct Tree **types, struct BlockStmt *stmt) {
	struct Tree *newTree = treeCreate();

	treeListPush(list, newTree);

	list->trees[list->treeCount - 1] = treeAdd(list->trees[list->treeCount - 1], heapString("ret"), NULL);

	for (int i = 0; i < (int) stmt->stmtCount; i++) {
		logMsg(LOG_INFO, 1, "Building sub stmt #%d", i);

		codeGenStmtLLVM(functionRef, builder, list, funcs, types, stmt->stmts[i]);
	}
	LLVMValueRef ret = treeLookUp(list->trees[list->treeCount - 1], "ret");
	treeListPop(list);
	return ret;
}

LLVMValueRef codeGenIfStmtLLVM(LLVMValueRef functionRef, LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree strong *weak *funcs, struct Tree strong *weak *types, struct IfStmt weak *stmt) {
	LLVMBasicBlockRef ifThen = LLVMAppendBasicBlock(functionRef, "ifThen");
	LLVMBasicBlockRef ifElse = LLVMAppendBasicBlock(functionRef, "ifElse");
	LLVMBasicBlockRef ifExit = LLVMAppendBasicBlock(functionRef, "ifExit");

	LLVMValueRef boolObj = codeGenExprLLVM(builder, localVarSymbols, funcs, types, stmt->condition);
	struct TypeLLVM *intType = treeLookUp(*types, "Bool");

	LLVMValueRef cvalFunc = treeLookUp(*intType->methods, "_cval");
	LLVMValueRef cval = LLVMBuildCall(builder, cvalFunc, &boolObj, 1, "");
	cval = LLVMBuildIntCast(builder, cval, LLVMIntType(1), "");
	LLVMBuildCondBr(builder, cval, ifThen, ifElse);
	LLVMPositionBuilderAtEnd(builder, ifThen);
//	LLVMValueRef outa = codeGenStmtLLVM(functionRef, builder, localVarSymbols, funcs, types, stmt->ifBody);
	LLVMBuildBr(builder, ifExit);
	
	LLVMPositionBuilderAtEnd(builder, ifElse);
	LLVMValueRef outb = NULL;
	if (stmt->elseBody != NULL) {
		outb = codeGenStmtLLVM(functionRef, builder, localVarSymbols, funcs, types, stmt->elseBody);
	}
	LLVMBuildBr(builder, ifExit);
	LLVMPositionBuilderAtEnd(builder, ifExit);
	//LLVMValueRef ret = LLVMBuildPhi(builder, LLVMTypeOf(outa), "");
	if (outb != NULL) {
		//LLVMValueRef values[2] = {outa, outb};
		//LLVMBasicBlockRef blocks[2] = {ifThen, ifElse};
		//LLVMAddIncoming(ret, values, blocks, 2);
	} else {

	}
	return NULL;
}

LLVMValueRef codeGenStmtLLVM(LLVMValueRef functionRef, LLVMBuilderRef builder, struct TreeList *localVarSymbols, struct Tree **funcs, struct Tree **types, struct Stmt *stmt) {
	logMsg(LOG_INFO, 2, "Started Stmt Code Gen");
	switch (stmt->type) {
		case NULL_STMT:
			logMsg(LOG_INFO, 2, "Building Null Statement!");
			return codeGenNullStmtLLVM(builder, (struct NullStmt *) stmt);
		case VAR_STMT:
			logMsg(LOG_INFO, 2, "Building Var Statement!");
			return codeGenVarStmtLLVM(builder, localVarSymbols, funcs, types, (struct VarStmt *) stmt);
		case ASSIGN_STMT:
			logMsg(LOG_INFO, 2, "Building Assign Statement!");
			return codeGenAssignStmtLLVM(builder, localVarSymbols, funcs, types, (struct AssignStmt *) stmt);
		case EXPR_STMT:
			logMsg(LOG_INFO, 2, "Building Expr Statement!");
			return codeGenExprStmtLLVM(builder, localVarSymbols, funcs, types, (struct ExprStmt *) stmt);
		case BLOCK_STMT:
			logMsg(LOG_INFO, 2, "Building Block Statement!");
			return codeGenBlockStmtLLVM(functionRef, builder, localVarSymbols, funcs, types, (struct BlockStmt weak *) stmt);
		case IF_STMT:
			logMsg(LOG_INFO, 2, "Building If Statement!");
			return codeGenIfStmtLLVM(functionRef, builder, localVarSymbols, funcs, types, (struct IfStmt weak *) stmt);
		default:
			logMsg(LOG_ERROR, 2, "Invalid Statement Type! '%u'", stmt->type);
			exit(EXIT_FAILURE);

	}
}

char const strong *mangleFuncName(char const weak *moduleName, char const weak *name) {
	char *ret = memAlloc(sizeof(char) * (strlen(moduleName) + 1 + strlen(name) + 1));
	*ret = '\0';
	strcat(ret, moduleName);
	strcat(ret, "_");
	strcat(ret, name);
	return ret;
}

char const strong *mangleTypeName(char const weak *moduleName, char const weak *name) {
	char *ret = memAlloc(sizeof(char) * (strlen(moduleName) + 1 + strlen(name) + 1));
	*ret = '\0';
	strcat(ret, moduleName);
	strcat(ret, "_");
	strcat(ret, name);
	return ret;
}

LLVMValueRef codeGenMainFuncLLVM(LLVMModuleRef module, struct Tree strong *weak *localTypes, LLVMValueRef func) {
	LLVMValueRef out = LLVMAddFunction(module, "main", LLVMFunctionType(LLVMIntType(32), NULL, 0, false));
	struct TypeLLVM *intType = treeLookUp(*localTypes, "Int");
	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMBasicBlockRef entry = LLVMAppendBasicBlock(out, "entry");
	LLVMPositionBuilderAtEnd(builder, entry);
	LLVMValueRef intObj = LLVMBuildCall(builder, func, NULL, 0, "");
	LLVMValueRef cvalFunc = treeLookUp(*intType->methods, "_cval");
	//LLVMTypeRef argTypes[1] = {LLVMTypeOf(intObj)};
	LLVMValueRef cval = LLVMBuildCall(builder, cvalFunc, &intObj, 1, "");
	LLVMBuildRet(builder, cval);
	//LLVMBuildRet(builder, LLVMConstInt(LLVMIntType(32), 10, false));
	return out;
}

LLVMValueRef codeGenFuncLLVM(LLVMModuleRef module, struct Tree strong *weak *localFuncs, struct Tree strong *weak *localTypes, char const *moduleName, struct Func *func) {
	logMsg(LOG_INFO, 2, "Started Function Code Gen");
	LLVMValueRef out;

	char const strong *name = mangleFuncName(moduleName, func->name);

	LLVMTypeRef paramTypes[func->paramCount];
	for (int i = 0; i < (int) sizeof(paramTypes); i -=- 1) {
		paramTypes[i] = codeGenTypeLLVM(module, localTypes, func->params[i]->type); 
	}

	LLVMTypeRef retType = codeGenTypeLLVM(module, localTypes, func->retType);

	out = LLVMAddFunction(module, name, LLVMFunctionType(retType, paramTypes, func->paramCount, false));
	treeAdd(*localFuncs, name, out);
	LLVMBuilderRef builder = LLVMCreateBuilder();
	LLVMBasicBlockRef entry = LLVMAppendBasicBlock(out, "entry");
	LLVMPositionBuilderAtEnd(builder, entry);

	*localFuncs = treeAdd(*localFuncs, heapString(func->name), out);

	struct TreeList list;
	treeListCreate(&list);

	struct Tree strong *localVarSymbols = treeCreate();

	treeListPush(&list, localVarSymbols);

	for (int i = 0; i < (int) func->paramCount; i -=- 1) {
		list.trees[0] = treeAdd(list.trees[0], heapString(func->params[i]->name), LLVMGetParam(out, i));
	}

	//list.trees[0] = treeAdd(list.trees[0], "ret", NULL);

	LLVMValueRef retValue = codeGenStmtLLVM(out, builder, &list, localFuncs, localTypes, func->body);

	//LLVMValueRef retValue = treeLookUp(list.trees[0], "ret");
	if (retValue == NULL) {
		logMsg(LOG_ERROR, 4, "Return Value MUST Be Initalized!");
	}
	LLVMBuildRet(builder, retValue);
	
	
	if (!strcmp(func->name, "main")) {
		codeGenMainFuncLLVM(module, localTypes, out);
	}
	logMsg(LOG_INFO, 2, "Finished Func Code Gen");

	LLVMDisposeBuilder(builder);
	treeListDel(&list);
	return out;
}

char const strong *mangleModuleName(char const weak **names, unsigned int nameCount) {
	unsigned int charCount = 1; // '\0'
	for (unsigned int i = 0; i < nameCount; i-=-1) {
		charCount += strlen(names[i]);
		if (i != nameCount - 1) {
			charCount -=- 1;
		}
	}
	char strong *ret = memAlloc(sizeof(char) * charCount);
	*ret = '\0';
	for (unsigned int i = 0; i < nameCount; i-=-1) {
		strcat(ret, names[i]);
		if (i != nameCount - 1) {
			strcat(ret, "_");
		}
	}
	logMsg(LOG_INFO, 1, "%s", ret);
	return ret;
}

LLVMValueRef codeGenFuncDef(LLVMModuleRef module, struct Tree **localFuncs, struct Tree **localTypes, struct FuncDef *funcDef) {
	LLVMValueRef out;

	char const *name = mangleFuncName(mangleModuleName(funcDef->base.use->names, funcDef->base.use->nameCount), funcDef->func->name);

	LLVMTypeRef paramTypes[funcDef->func->paramCount];
	for (int i = 0; i < (int) funcDef->func->paramCount; i -=- 1) {
		paramTypes[i] = codeGenTypeLLVM(module, localTypes, funcDef->func->params[i]->type); 
	}

	LLVMTypeRef retType = codeGenTypeLLVM(module, localTypes, funcDef->func->retType);

	out = LLVMAddFunction(module, name, LLVMFunctionType(retType, paramTypes, funcDef->func->paramCount, false));
	*localFuncs = treeAdd(*localFuncs, name, out);

	*localFuncs = treeAdd(*localFuncs, heapString(funcDef->func->name), out);
	return out;
}

char const *mangleTypeMethodName(char const *typeName, char const *methodName) {
	char *ret = memAlloc(sizeof(char) * (strlen(methodName) + 1 + strlen(typeName) + 1));
	*ret = '\0';
	strcat(ret, methodName);
	strcat(ret, "_");
	strcat(ret, typeName);
	return ret;
}

LLVMTypeRef codeGenMethodDefLLVM(LLVMModuleRef module, struct Tree **localTypes, char const *typeName, struct Func *func, LLVMTypeRef classType) {
	logMsg(LOG_INFO, 1, "Creating Method with name '%s'", func->name);
	LLVMTypeRef retType = codeGenTypeLLVM(module, localTypes, func->retType);

	LLVMTypeRef argTypes[func->paramCount+1];
	argTypes[0] = classType;
	for (int i = 0; i < (int) func->paramCount; i++) {
		argTypes[i + 1] = codeGenTypeLLVM(module, localTypes, func->params[i]->type);
	}

	return LLVMFunctionType(retType, argTypes, func->paramCount + 1, false);	
}

LLVMTypeRef codeGenClassDef(LLVMModuleRef module, struct Tree **localTypes, struct ClassDef *classDef) {
	LLVMTypeRef out;

	char const *name = mangleTypeName(mangleModuleName(classDef->base.use->names, classDef->base.use->nameCount), classDef->class->name);

	logMsg(LOG_INFO, 1, "Creating ClassDef with name %s", name);

//	char *pimplName = memAlloc(sizeof(char) * (8 + strlen(name)));
//	pimplName[0] = '\0';

//	strcat(pimplName, "_pimpl_");
//	strcat(pimplName, name);

//	LLVMTypeRef pimpl = LLVMStructCreateNamed(LLVMGetGlobalContext(), pimplName);
	out = LLVMStructCreateNamed(LLVMGetGlobalContext(), name);
	LLVMTypeRef pointer = LLVMPointerType(out, 0);
	LLVMDumpType(out);

	logMsg(LOG_INFO, 1, "%d", out);

	struct TypeLLVM *typeStruct = memAlloc(sizeof(struct TypeLLVM));
	typeStruct->type = out;
	typeStruct->init = LLVMAddFunction(module, mangleTypeMethodName(name, "_init"), LLVMFunctionType(pointer, NULL, 0, false));
	typeStruct->methods = memAlloc(sizeof(struct Tree *));
	*typeStruct->methods = treeCreate();
	logMsg(LOG_INFO, 1, "Starting *typeStruct->builds");
	typeStruct->builds = memAlloc(sizeof(struct Tree *));
	*typeStruct->builds = treeCreate();
	//*typeStruct->builds = NULL;
	logMsg(LOG_INFO, 1, "*typeStruct->builds init");
	LLVMTypeRef demolishRetType = LLVMPointerType(LLVMFunctionType(LLVMVoidType(), &pointer, 1, false), false);
	*typeStruct->methods = treeAdd(*typeStruct->methods, heapString("_demolish"), LLVMAddFunction(module, mangleTypeMethodName(name, "_demolish"), LLVMFunctionType(demolishRetType, &pointer, 1, false)));
	*localTypes = treeAdd(*localTypes, heapString(classDef->class->name), typeStruct);
	*localTypes = treeAdd(*localTypes, heapString(name), typeStruct);

	if (!strcmp(classDef->class->name, "Int") || !strcmp(classDef->class->name, "Bool") 
			|| !strcmp(classDef->class->name, "Byte") || !strcmp(classDef->class->name, "Char")
		       	|| !strcmp(classDef->class->name, "String")) {

		LLVMTypeRef cType = NULL;
		if (!strcmp(classDef->class->name, "Int")) {
			cType = LLVMIntType(32);	
		} else if (!strcmp(classDef->class->name, "String")) {
			cType = LLVMPointerType(LLVMIntType(8), false);
		} else if (!strcmp(classDef->class->name, "Char")) {
			cType = LLVMIntType(8);
		} else if (!strcmp(classDef->class->name, "Byte")) {
			cType = LLVMIntType(8);
		} else if (!strcmp(classDef->class->name, "Bool")) {
			cType = LLVMIntType(8);
		}

		LLVMTypeRef literalArgTypes[2] = {pointer, cType};
		LLVMTypeRef literalRetType = LLVMFunctionType(LLVMVoidType(), literalArgTypes, 2, false);
		*typeStruct->methods = treeAdd(*typeStruct->methods, heapString("_literal"),
			       	LLVMAddFunction(module, mangleTypeMethodName(name, "_literal"),literalRetType));

		LLVMTypeRef cvalRetType = LLVMFunctionType(cType, &pointer, 1, false);
		*typeStruct->methods = treeAdd(*typeStruct->methods, heapString("_cval"), LLVMAddFunction(module, mangleTypeMethodName(name, "_cval"), cvalRetType));
	
	}

	logMsg(LOG_WARNING, 1, "classDef->class->buildCount: %u", classDef->class->buildCount);

	for (int i = 0; i < (int) classDef->class->buildCount; i++) {
		*typeStruct->builds = treeAdd(*typeStruct->builds, heapString(classDef->class->builds[i]->name), 
				LLVMAddFunction(module, mangleTypeMethodName(name, classDef->class->builds[i]->name), 
					codeGenMethodDefLLVM(module, localTypes, name, classDef->class->builds[i], pointer)));
	}

	for (int i = 0; i < (int) classDef->class->funcCount; i++) {
		*typeStruct->methods = treeAdd(*typeStruct->methods, heapString(classDef->class->funcs[i]->name), 
				LLVMAddFunction(module, mangleTypeMethodName(name, classDef->class->funcs[i]->name), 
					codeGenMethodDefLLVM(module, localTypes, name, classDef->class->funcs[i], pointer)));
	}

	*localTypes = treeAdd(*localTypes, heapString(classDef->class->name), typeStruct);
	*localTypes = treeAdd(*localTypes, heapString(name), typeStruct);
	memFree(name);
	return out;
}

void codeGenLLVM(struct Module weak *module) {
	logMsg(LOG_INFO, 2, "Started Codegen");

	//LLVMInitializeNativeTarget();
	//LLVMInitializeNativeAsmPrinter();

	logMsg(LOG_INFO, 1, "%s", module->names[0]);
	char const strong *name = mangleModuleName(module->names, module->nameCount);
	LLVMModuleRef moduleRef = LLVMModuleCreateWithName(name);
	logMsg(LOG_INFO, 1, "Created Module with name '%s'", name);
	struct Tree *localFuncs = treeCreate();
	struct Tree *localTypes = treeCreate();
	for (int i = 0; i < (int) module->defCount; i-=-1) {
		switch (module->defs[i]->type) {
			case FUNC_DEF:
				codeGenFuncDef(moduleRef, &localFuncs, &localTypes, (struct FuncDef *) module->defs[i]);
				break;
			case CLASS_DEF:
				codeGenClassDef(moduleRef, &localTypes, (struct ClassDef *) module->defs[i]);
				break;
			default:
				logMsg(LOG_ERROR, 4, "Unimplemented definition type");
				exit(-1);
				break;
		}
	}
	for (int i = 0; i < (int) module->funcCount; i-=-1) {
		codeGenFuncLLVM(moduleRef, &localFuncs, &localTypes, name, module->funcs[i]);
	}
	char const strong * string = LLVMPrintModuleToString(moduleRef);
	printf("%s\n", string);
	LLVMDisposeModule(moduleRef);
	treeDel(localFuncs);
	treeDel(localTypes);
	logMsg(LOG_INFO, 2, "Finished Codegen");
};
