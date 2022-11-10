#pragma once

char *DumpMem(unsigned char *pAddr, int len);

extern unsigned char encrBuffer[4092];

void kyretardizeDatagram(unsigned char *buf, int len, int port, int unk);
