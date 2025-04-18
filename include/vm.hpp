
#ifndef VM_H_
#define VM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define DEFAULT_STACK_SIZE 1000
#define DEFAULT_CALL_STACK_SIZE 100
#define DEFAULT_NUM_LOCALS 10

  typedef enum
  {
    NOOP = 0,     // no operation
    IADD = 1,     // int add
    ISUB = 2,     // int sub
    IMUL = 3,     // int multiply
    IDIV = 4,     // int division
    IOR = 5,      // int comparison OR
    IAND = 6,     // int comparison AND
    INOT = 7,     // int comparison NOT
    ILT = 8,      // int less than
    IGT = 9,      // int greater than
    IEQ = 10,     // int equal
    BR = 11,      // branch
    BRT = 12,     // branch if true
    BRF = 13,     // branch if true
    ICONST = 14,  // push constant integer
    LOAD = 15,    // load from local context
    GLOAD = 16,   // load from global memory
    STORE = 17,   // store in local context
    GSTORE = 18,  // store in global memory
    PRINT = 19,   // print stack top
    PRINTC = 20,  // print char from stack top
    READINT = 21, // get int from std input and put it into top of the stack
    POP = 22,     // throw away top of stack
    CALL = 23,    // call function at address with nargs,nlocals
    RET = 24,     // return value from function
    HALT = 25
  } VM_CODE;

  typedef struct
  {
    int returnip;
    int locals[DEFAULT_NUM_LOCALS];
  } Context;

  typedef struct
  {
    int *code;
    int code_size;

    // global variable space
    int *globals;
    int nglobals;

    // Operand stack, grows upwards
    int stack[DEFAULT_STACK_SIZE];
    Context call_stack[DEFAULT_CALL_STACK_SIZE];
  } VM;

  VM *vm_create(int *code, int code_size, int nglobals);
  void vm_free(VM *vm);
  void vm_init(VM *vm, int *code, int code_size, int nglobals);
  void vm_exec(VM *vm, int startip, bool trace);
  void vm_print_instr(int *code, int ip);
  void vm_print_stack(int *stack, int count);
  void vm_print_data(int *globals, int count);
  void vm_context_init(Context *ctx, int ip, int nlocals);

#ifdef __cplusplus
}
#endif

#endif