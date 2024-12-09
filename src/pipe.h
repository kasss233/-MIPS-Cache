#ifndef _PIPE_H_
#define _PIPE_H_

#include "shell.h"

/* Pipeline 操作 (Pipe_Op) 是流水线中指令的高级表示形式。
 * 该结构体并不是流水线控制信号的直接表示，而是包含了指令的原始信息、
 * 操作数以及目标寄存器等信息，为流水线处理提供基础数据支持。 */
typedef struct Pipe_Op
{
    /* 指令地址 */
    uint32_t pc;

    /* 原始指令的二进制编码 */
    uint32_t instruction;

    /* 解码后的主操作码和子操作码 */
    int opcode, subop; // 主操作码区分指令类型（如加法、加载、分支等），子操作码区分特殊操作（如 ADD, SUB）。

    /* 立即数相关信息 */
    uint32_t imm16, se_imm16; // 16 位立即数和符号扩展后的立即数。

    /* 移位位数 */
    int shamt; // 指定移位操作的位数。

    /* 寄存器源操作数 */
    int reg_src1, reg_src2;                  // 源寄存器编号（范围 0 - 31，如果没有源寄存器则为 -1）。
    uint32_t reg_src1_value, reg_src2_value; // 源寄存器的值。

    /* 内存操作信息 */
    int is_mem;         // 是否是内存操作指令（加载/存储）。
    uint32_t mem_addr;  // 内存地址（如果适用）。
    int mem_write;      // 是否向内存写入数据。
    uint32_t mem_value; // 内存操作的数据值（加载或写入的值）。

    /* 目标寄存器信息 */
    int reg_dst;             // 目标寄存器编号（范围 0 - 31，如果没有目标寄存器则为 -1）。
    uint32_t reg_dst_value;  // 目标寄存器值。
    int reg_dst_value_ready; // 目标寄存器值是否已准备好，避免数据冒险。

    /* 分支指令相关信息 */
    int is_branch;        // 是否是分支指令。
    uint32_t branch_dest; // 分支目标地址（如果执行）。
    int branch_cond;      // 是否是条件分支。
    int branch_taken;     // 分支是否被执行。
    int is_link;          // 是否是需要保存返回地址的指令（如 JAL）。
    int link_reg;         // 保存返回地址的寄存器编号。

} Pipe_Op; // 流水线中每条指令的详细信息。

/* Pipe_State 结构体表示流水线的当前状态。它维护了每个阶段当前处理的指令
 * 以及相关的寄存器、程序计数器（PC）和分支恢复信息。 */
typedef struct Pipe_State
{
    /* 每个流水线阶段当前的指令（如果没有则为 NULL） */
    Pipe_Op *decode_op, *execute_op, *mem_op, *wb_op;

    /* 寄存器文件状态 */
    uint32_t REGS[32]; // 通用寄存器数组。
    uint32_t HI, LO;   // 特殊寄存器（乘法/除法结果存储）。

    /* 取指阶段的程序计数器 */
    uint32_t PC;

    /* 分支恢复信息 */
    int branch_recover;   // 如果为 1，则加载新的 PC。
    uint32_t branch_dest; // 分支恢复后的 PC 地址。
    int branch_flush;     // 恢复时需要清空的阶段数（1 = fetch, 2 = fetch/decode 等）。

    /* 乘法器的延迟信息 */
    int multiplier_stall; // 距离 HI/LO 寄存器准备好的剩余周期数。

    /* 可以根据需要添加其他状态信息 */

} Pipe_State;

/* 全局变量 -- 流水线状态 */
extern Pipe_State pipe;

/* 模拟器启动时调用，用于初始化流水线 */
void pipe_init();

/* 主函数，用于控制流水线的循环执行 */
void pipe_cycle();

/* 辅助函数：用于分支恢复 */
void pipe_recover(int flush, uint32_t dest);

/* 以下每个函数实现流水线的一个阶段 */
void pipe_stage_fetch();   // 取指阶段
void pipe_stage_decode();  // 解码阶段
void pipe_stage_execute(); // 执行阶段
void pipe_stage_mem();     // 访存阶段
void pipe_stage_wb();      // 写回阶段

#endif
