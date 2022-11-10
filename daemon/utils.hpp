#pragma once
#include <windows.h>
#include <cstdio>

void alloc_console() {
#ifdef _DEBUG
  AllocConsole();

  FILE* fDummy;
  freopen_s(&fDummy, "CONIN$", "r", stdin);
  freopen_s(&fDummy, "CONOUT$", "w", stderr);
  freopen_s(&fDummy, "CONOUT$", "w", stdout);
#endif
}
