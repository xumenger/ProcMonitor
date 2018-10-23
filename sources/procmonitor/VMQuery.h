#ifndef VMQUERY_H
#define VMQUERY_H

#include<Windows.h>

typedef struct{
    //Region information
    PVOID  pvRgnBaseAddress;     //对应内存区域基地址
    DWORD  dwRgnProtection;      //对应内存区域保护属性，PAGE_*
    SIZE_T RgnSize;              //对应内存区域大小
    DWORD  dwRgnStorage;         //对应内存区域物理存储器类型，MEM_*: Free, Image, Mapped, Private
    DWORD  dwRgnBlocks;          //对应内存区域块的个数
    DWORD  dwRgnGuardBlks;       //对应内存区域中具有PAGE_GUARD保护属性标志的块的数量，若为1表示该区域是为了线程栈而预订的
    BOOL   bRgnIsAStack;         //对应内存区域是否包含线程栈。该值是通过近似猜测得到的

    //Block information
    PVOID  pvBlkBaseAddress;     //对应内存块基地址
    DWORD  dwBlkProtection;      //对应内存块保护属性，PAGE_*
    SIZE_T BlkSize;              //对应内存块大小
    DWORD  dwBlkStorage;         //对应内存块物理存储器类型，MEM_*: Free, Image, Mapped, Private
} VMQUERY, *PVMQUERY;

BOOL VMQuery(HANDLE hProcess, LPCVOID pvAddress, PVMQUERY pVMQ);

#endif