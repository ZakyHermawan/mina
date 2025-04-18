#include "vm.hpp"

#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <iostream>

typedef struct
{
  char name[8];
  int nargs;
} VM_INSTRUCTION;

static VM_INSTRUCTION vm_instructions[] = {
    {"noop", 0},    // 0
    {"iadd", 0},    // 1
    {"isub", 0},    // 2
    {"imul", 0},    // 3
    {"idiv", 0},  // 4
    {"ior", 0},  // 5
    {"iand", 0},  // 6
    {"inot", 0},   // 7
    {"ilt", 0},     // 4
    {"igt", 0},   // 4
    {"ieq", 0},     // 5
    {"br", 1},      // 7
    {"brt", 1},     // 8
    {"brf", 1},     // 9
    {"iconst", 1},  // 10
    {"load", 1},   {"gload", 1}, {"store", 1}, {"gstore", 1}, {"print", 0},
    {"princ", 0}, {"readint", 0},
    {"pop", 0},    {"call", 3},  {"ret", 0},   {"halt", 0}};

void vm_init(VM *vm, int *code, int code_size, int nglobals)
{
  vm->code = code;
  vm->code_size = code_size;
  vm->globals = (int*)calloc(nglobals, sizeof(int));
  vm->nglobals = nglobals;
}

void vm_free(VM *vm)
{
  free(vm->globals);
  free(vm);
}

VM *vm_create(int *code, int code_size, int nglobals)
{
  VM *vm = (VM*)calloc(1, sizeof(VM));
  vm_init(vm, code, code_size, nglobals);
  return vm;
}

void vm_exec(VM *vm, int startip, bool trace)
{
  // registers
  int ip;      // instruction pointer register
  int sp;      // stack pointer register
  int callsp;  // call stack pointer register

  int a = 0;
  int b = 0;
  int addr = 0;
  int offset = 0;

  ip = startip;
  sp = -1;
  callsp = -1;
  int opcode = vm->code[ip];
  int nlocals;
  int nargs;
  while (opcode != HALT && ip < vm->code_size)
  {
    if (trace) vm_print_instr(vm->code, ip);

    ip++;  // jump to next instruction or to operand
    switch (opcode)
    {
      case IADD:
        b = vm->stack[sp--];      // 2nd opnd at top of stack
        a = vm->stack[sp--];      // 1st opnd 1 below top
        vm->stack[++sp] = a + b;  // push result
        break;
      case ISUB:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = a - b;
        break;
      case IMUL:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = a * b;
        break;
      case IDIV:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = a / b;
        break;
      case IOR:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = a || b;
        break;
      case IAND:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = a && b;
        break;
      case INOT:
        a = vm->stack[sp--];
        vm->stack[++sp] = !a;
        break;
      case ILT:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = (a < b) ? true : false;
        break;
      case IGT:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = (a > b) ? true : false;
        break;
      case IEQ:
        b = vm->stack[sp--];
        a = vm->stack[sp--];
        vm->stack[++sp] = (a == b) ? true : false;
        break;
      case BR:
        ip = vm->code[ip];
        break;
      case BRT:
        addr = vm->code[ip++];
        if (vm->stack[sp--]) ip = addr;
        break;
      case BRF:
        addr = vm->code[ip++];
        if (!(vm->stack[sp--])) ip = addr;
        break;
      case ICONST:
        vm->stack[++sp] = vm->code[ip++];  // push operand
        break;
      case LOAD:  // load local or arg
        offset = vm->code[ip++];
        vm->stack[++sp] = vm->call_stack[callsp].locals[offset];
        break;
      case GLOAD:  // load from global memory
        addr = vm->code[ip++];
        vm->stack[++sp] = vm->globals[addr];
        break;
      case STORE:
        offset = vm->code[ip++];
        vm->call_stack[callsp].locals[offset] = vm->stack[sp--];
        break;
      case GSTORE:
        addr = vm->code[ip++];
        vm->globals[addr] = vm->stack[sp--];
        break;
      case PRINT:
        printf("%d\n", vm->stack[sp--]);
        break;
      case PRINTC:
        printf("%c", (char)vm->stack[sp--]);
        break;
      case READINT:
        int tmp;
        scanf("%d", &tmp);
        //std::cout << tmp << std
        vm->stack[++sp] = tmp;
        break;
      case POP:
        --sp;
        break;
      case CALL:
        // expects all args on stack
        addr = vm->code[ip++];         // index of target function
        nargs = vm->code[ip++];    // how many args got pushed
        nlocals = vm->code[ip++];  // how many locals to allocate
        ++callsp;  // bump stack pointer to reveal space for this call
        vm_context_init(&vm->call_stack[callsp], ip, nargs + nlocals);
        // copy args into new context
        for (int i = 0; i < nargs; i++)
        {
          vm->call_stack[callsp].locals[i] = vm->stack[sp - i];
        }
        sp -= nargs;
        ip = addr;  // jump to function
        break;
      case RET:
        ip = vm->call_stack[callsp].returnip;
        callsp--;  // pop context
        break;
      default:
        printf("invalid opcode: %d at ip=%d\n", opcode, (ip - 1));
        exit(1);
    }
    if (trace) vm_print_stack(vm->stack, sp);
    opcode = vm->code[ip];
  }
  if (trace) vm_print_data(vm->globals, vm->nglobals);
}

static void vm_context_init(Context *ctx, int ip, int nlocals)
{
  if (nlocals > DEFAULT_NUM_LOCALS)
  {
    fprintf(stderr, "too many locals requested: %d\n", nlocals);
  }
  ctx->returnip = ip;
}

void vm_print_instr(int *code, int ip)
{
  int opcode = code[ip];
  VM_INSTRUCTION *inst = &vm_instructions[opcode];
  switch (inst->nargs)
  {
    case 0:
      printf("%04d:  %-20s", ip, inst->name);
      break;
    case 1:
      printf("%04d:  %-10s%-10d", ip, inst->name, code[ip + 1]);
      break;
    case 2:
      printf("%04d:  %-10s%d,%10d", ip, inst->name, code[ip + 1], code[ip + 2]);
      break;
    case 3:
      printf("%04d:  %-10s%d,%d,%-6d", ip, inst->name, code[ip + 1],
             code[ip + 2], code[ip + 3]);
      break;
  }
}

void vm_print_stack(int *stack, int count)
{
  printf("stack=[");
  for (int i = 0; i <= count; i++)
  {
    printf(" %d", stack[i]);
  }
  printf(" ]\n");
}

void vm_print_data(int *globals, int count)
{
  printf("Data memory:\n");
  for (int i = 0; i < count; i++)
  {
    printf("%04d: %d\n", i, globals[i]);
  }
}
