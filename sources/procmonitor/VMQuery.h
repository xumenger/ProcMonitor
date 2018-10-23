#ifndef VMQUERY_H
#define VMQUERY_H

#include<Windows.h>

typedef struct{
    //Region information
    PVOID  pvRgnBaseAddress;     //��Ӧ�ڴ��������ַ
    DWORD  dwRgnProtection;      //��Ӧ�ڴ����򱣻����ԣ�PAGE_*
    SIZE_T RgnSize;              //��Ӧ�ڴ������С
    DWORD  dwRgnStorage;         //��Ӧ�ڴ���������洢�����ͣ�MEM_*: Free, Image, Mapped, Private
    DWORD  dwRgnBlocks;          //��Ӧ�ڴ������ĸ���
    DWORD  dwRgnGuardBlks;       //��Ӧ�ڴ������о���PAGE_GUARD�������Ա�־�Ŀ����������Ϊ1��ʾ��������Ϊ���߳�ջ��Ԥ����
    BOOL   bRgnIsAStack;         //��Ӧ�ڴ������Ƿ�����߳�ջ����ֵ��ͨ�����Ʋ²�õ���

    //Block information
    PVOID  pvBlkBaseAddress;     //��Ӧ�ڴ�����ַ
    DWORD  dwBlkProtection;      //��Ӧ�ڴ�鱣�����ԣ�PAGE_*
    SIZE_T BlkSize;              //��Ӧ�ڴ���С
    DWORD  dwBlkStorage;         //��Ӧ�ڴ������洢�����ͣ�MEM_*: Free, Image, Mapped, Private
} VMQUERY, *PVMQUERY;

BOOL VMQuery(HANDLE hProcess, LPCVOID pvAddress, PVMQUERY pVMQ);

#endif