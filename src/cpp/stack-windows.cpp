#pragma comment(lib, "version.lib") // for "VerQueryValue"

#include <tchar.h>
#include <iostream>
#include <windows.h>
#include <dbghelp.h>

#include "stack-windows.hpp"


constexpr int TTBUFLEN = 8096;
constexpr int STACKWALK_MAX_NAMELEN = 1024;
constexpr int MAX_MODULE_NAME32 = 255;
constexpr int TH32CS_SNAPMODULE = 0x00000008;
constexpr size_t nSymPathLen = 4096;

// Normally it should be enough to use 'CONTEXT_FULL' (better would be 'CONTEXT_ALL')
constexpr int USED_CONTEXT_FLAGS = CONTEXT_FULL;


struct IMAGEHLP_MODULE64_V2 {
	DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_MODULE64)
	DWORD64  BaseOfImage;            // base load address of module
	DWORD    ImageSize;              // virtual size of the loaded module
	DWORD    TimeDateStamp;          // date/time stamp from pe header
	DWORD    CheckSum;               // checksum from the pe header
	DWORD    NumSyms;                // number of symbols in the symbol table
	SYM_TYPE SymType;                // type of symbols loaded
	CHAR     ModuleName[32];         // module name
	CHAR     ImageName[256];         // image name
	CHAR     LoadedImageName[256];   // symbol file name
};

// SymCleanup()
typedef BOOL (__stdcall *tSC)(IN HANDLE hProcess);

// SymFunctionTableAccess64()
typedef PVOID (__stdcall *tSFTA)(HANDLE hProcess, DWORD64 AddrBase);

// SymGetLineFromAddr64()
typedef BOOL (__stdcall *tSGLFA)(
	IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line
);

// SymGetModuleBase64()
typedef DWORD64 (__stdcall *tSGMB)(IN HANDLE hProcess, IN DWORD64 dwAddr);

// SymGetModuleInfo64()
typedef BOOL (__stdcall *tSGMI)(
	IN HANDLE hProcess, IN DWORD64 dwAddr, OUT IMAGEHLP_MODULE64_V2 *ModuleInfo
);

// SymGetOptions()
typedef DWORD (__stdcall *tSGO)(VOID);

// SymGetSymFromAddr64()
typedef BOOL (__stdcall *tSGSFA)(
	IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD64 pdwDisplacement, OUT PIMAGEHLP_SYMBOL64 Symbol
);

// SymInitialize()
typedef BOOL (__stdcall *tSI)(
	IN HANDLE hProcess, IN PSTR UserSearchPath, IN BOOL fInvadeProcess
);

// SymLoadModule64()
typedef DWORD64 (__stdcall *tSLM)(
	IN HANDLE hProcess, IN HANDLE hFile,
	IN PSTR ImageName, IN PSTR ModuleName, IN DWORD64 BaseOfDll, IN DWORD SizeOfDll
);

// SymSetOptions()
typedef DWORD (__stdcall *tSSO)(IN DWORD SymOptions);

// StackWalk64()
typedef BOOL (__stdcall *tSW)(
	DWORD MachineType,
	HANDLE hProcess,
	HANDLE hThread,
	LPSTACKFRAME64 StackFrame,
	PVOID ContextRecord,
	PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
	PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
	PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
	PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
);

// UnDecorateSymbolName()
typedef DWORD (__stdcall WINAPI *tUDSN)(
	PCSTR DecoratedName, PSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags
);

typedef BOOL (__stdcall WINAPI *tSGSP)(
	HANDLE hProcess, PSTR SearchPath, DWORD SearchPathLength
);

#pragma pack(push, 8)
typedef struct tagMODULEENTRY32 {
		DWORD   dwSize;
		DWORD   th32ModuleID;       // This module
		DWORD   th32ProcessID;      // owning process
		DWORD   GlblcntUsage;       // Global usage count on the module
		DWORD   ProccntUsage;       // Module usage count in th32ProcessID's context
		BYTE  * modBaseAddr;        // Base address of module in th32ProcessID's context
		DWORD   modBaseSize;        // Size in bytes of module starting at modBaseAddr
		HMODULE hModule;            // The hModule of this module in th32ProcessID's context
		char    szModule[MAX_MODULE_NAME32 + 1];
		char    szExePath[MAX_PATH];
} MODULEENTRY32;
typedef MODULEENTRY32* PMODULEENTRY32;
typedef MODULEENTRY32* LPMODULEENTRY32;
#pragma pack(pop)

typedef struct _MODULEINFO {
	LPVOID lpBaseOfDll;
	DWORD SizeOfImage;
	LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

// EnumProcessModules()
typedef BOOL (__stdcall *tEPM)(
	HANDLE hProcess, HMODULE *lphModule, DWORD cb, LPDWORD lpcbNeeded
);

// GetModuleFileNameEx()
typedef DWORD (__stdcall *tGMFNE)(
	HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize
);

// GetModuleBaseName()
typedef DWORD (__stdcall *tGMBN)(
	HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize
);

// GetModuleInformation()
typedef BOOL (__stdcall *tGMI)(
	HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize
);

typedef BOOL (__stdcall *PReadProcessMemoryRoutine) (
	HANDLE hProcess,
	DWORD64 qwBaseAddress,
	PVOID lpBuffer,
	DWORD nSize,
	LPDWORD lpNumberOfBytesRead,
	LPVOID pUserData  // optional data, which was passed in "ShowCallstack"
);

// Entry for each Callstack-Entry
struct CallstackEntry {
	DWORD64 offset; // if 0, we have no valid entry
	CHAR name[STACKWALK_MAX_NAMELEN];
	CHAR undName[STACKWALK_MAX_NAMELEN];
	CHAR undFullName[STACKWALK_MAX_NAMELEN];
	DWORD64 offsetFromSmybol;
	DWORD offsetFromLine;
	DWORD lineNumber;
	CHAR lineFileName[STACKWALK_MAX_NAMELEN];
	DWORD symType;
	LPCSTR symTypeString;
	CHAR moduleName[STACKWALK_MAX_NAMELEN];
	DWORD64 baseOfImage;
	CHAR loadedImageName[STACKWALK_MAX_NAMELEN];
};

enum CallstackEntryType {
	firstEntry,
	nextEntry,
	lastEntry
};


// The following is used to pass the "userData"-Pointer to the user-provided readMemoryFunction
// This has to be done due to a problem with the "hProcess"-parameter in x64...
// Because this class is in no case multi-threading-enabled (because of the limitations
// of dbghelp.dll) it is "safe" to use a static-variable
PReadProcessMemoryRoutine s_readMemoryFunction = nullptr;
LPVOID s_readMemoryFunction_UserData = nullptr;

tSC pSC = nullptr;
tSFTA pSFTA = nullptr;
tSGLFA pSGLFA = nullptr;
tSGMB pSGMB = nullptr;
tSGMI pSGMI = nullptr;
tSGO pSGO = nullptr;
tSGSFA pSGSFA = nullptr;
tSI pSI = nullptr;
tSLM pSLM = nullptr;
tSSO pSSO = nullptr;
tSW pSW = nullptr;
tUDSN pUDSN = nullptr;
tSGSP pSGSP = nullptr;

HMODULE m_hDbhHelp = nullptr;


bool GetModuleInfo(HANDLE hProcess, DWORD64 baseAddr, IMAGEHLP_MODULE64_V2 *pModuleInfo) {
	if(!pSGMI) {
		SetLastError(ERROR_DLL_INIT_FAILED);
		return false;
	}
	
	pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE64_V2);
	
	// reserve enough memory, so the bug in v6.3.5.1 does not lead to memory-overwrites...
	void *pData = malloc(4096);
	if (!pData) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return false;
	}
	
	memcpy(pData, pModuleInfo, sizeof(IMAGEHLP_MODULE64_V2));
	
	if (pSGMI(hProcess, baseAddr, reinterpret_cast<IMAGEHLP_MODULE64_V2*>(pData))) {
		// only copy as much memory as is reserved...
		memcpy(pModuleInfo, pData, sizeof(IMAGEHLP_MODULE64_V2));
		pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE64_V2);
		free(pData);
		return true;
	}
	
	free(pData);
	SetLastError(ERROR_DLL_INIT_FAILED);
	return false;
}

void OnLoadModule(
	LPCSTR img,
	LPCSTR mod,
	DWORD64 baseAddr,
	DWORD size,
	DWORD result,
	LPCSTR symType,
	LPCSTR pdbName,
	uint64_t fileVersion
) {
	char buffer[STACKWALK_MAX_NAMELEN];
	if (!fileVersion) {
		_snprintf_s(
			buffer,
			STACKWALK_MAX_NAMELEN,
			"%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s'\n",
			img, mod, (LPVOID) baseAddr, size, result, symType, pdbName
		);
		return;
	}
	
	DWORD v4 = (DWORD) fileVersion & 0xFFFF;
	DWORD v3 = (DWORD) (fileVersion>>16) & 0xFFFF;
	DWORD v2 = (DWORD) (fileVersion>>32) & 0xFFFF;
	DWORD v1 = (DWORD) (fileVersion>>48) & 0xFFFF;
	_snprintf_s(
		buffer,
		STACKWALK_MAX_NAMELEN,
		"%s:%s (%p), size: %d (result: %d), SymType: '%s', PDB: '%s', fileVersion: %d.%d.%d.%d\n",
		img, mod, (LPVOID) baseAddr, size, result, symType, pdbName, v1, v2, v3, v4
	);
}


uint64_t getFileVersion(char *szImg) {
	uint64_t fileVersion = 0;
	
	DWORD dwHandle;
	DWORD dwSize = GetFileVersionInfoSizeA(szImg, &dwHandle);
	if (dwSize <= 0) {
		return fileVersion;
	}
	
	LPVOID vData = malloc(dwSize);
	if (!vData) {
		return fileVersion;
	}
	
	if (GetFileVersionInfoA(szImg, dwHandle, dwSize, vData)) {
		VS_FIXEDFILEINFO *fInfo = nullptr;
		UINT len;
		TCHAR szSubBlock[] = _T("\\");
		if (!VerQueryValue(
			vData,
			szSubBlock,
			reinterpret_cast<LPVOID*>(&fInfo),
			&len
		)) {
			fInfo = nullptr;
		} else {
			fileVersion = (
				static_cast<uint64_t>(fInfo->dwFileVersionLS) +
				(static_cast<uint64_t>(fInfo->dwFileVersionMS) << 32)
			);
		}
	}
	free(vData);
	
	return fileVersion;
}


inline void setSymType(SYM_TYPE SymType, const char **szSymType) {
	switch(SymType) {
		case SymNone:
			*szSymType = "-nosymbols-";
			break;
		case SymCoff:
			*szSymType = "COFF";
			break;
		case SymCv:
			*szSymType = "CV";
			break;
		case SymPdb:
			*szSymType = "PDB";
			break;
		case SymExport:
			*szSymType = "-exported-";
			break;
		case SymDeferred:
			*szSymType = "-deferred-";
			break;
		case SymSym:
			*szSymType = "SYM";
			break;
		case 8: //SymVirtual:
			*szSymType = "Virtual";
			break;
		case 9: // SymDia:
			*szSymType = "DIA";
			break;
	}
}


DWORD LoadModule(
	HANDLE hProcess,
	LPCSTR img,
	LPCSTR mod,
	DWORD64 baseAddr,
	DWORD size
) {
	CHAR *szImg = _strdup(img);
	CHAR *szMod = _strdup(mod);
	DWORD result = ERROR_SUCCESS;
	
	if (!szImg || !szMod) {
		result = ERROR_NOT_ENOUGH_MEMORY;
	} else if (pSLM(hProcess, 0, szImg, szMod, baseAddr, size) == 0) {
		result = GetLastError();
	}
	
	if (szImg) {
		// try to retrive the file-version:
		uint64_t fileVersion = getFileVersion(szImg);
		
		// Retrive some additional-infos about the module
		IMAGEHLP_MODULE64_V2 Module;
		const char *szSymType = "-unknown-";
		if (GetModuleInfo(hProcess, baseAddr, &Module)) {
			setSymType(Module.SymType, &szSymType);
		}
		OnLoadModule(
			img, mod, baseAddr, size,
			result, szSymType, Module.LoadedImageName, fileVersion
		);
	}
	if (szImg) {
		free(szImg);
	}
	if (szMod) {
		free(szMod);
	}
	return result;
}


bool loadPsapi(
	HINSTANCE *hPsapi,
	tEPM *pEPM,
	tGMFNE *pGMFNE,
	tGMBN *pGMBN,
	tGMI *pGMI
) {
	*hPsapi = LoadLibrary(_T("psapi.dll"));
	if (!*hPsapi) {
		return false;
	}
	
	*pEPM = reinterpret_cast<tEPM>(GetProcAddress(*hPsapi, "EnumProcessModules"));
	*pGMFNE = reinterpret_cast<tGMFNE>(GetProcAddress(*hPsapi, "GetModuleFileNameExA"));
	*pGMBN = reinterpret_cast<tGMFNE>(GetProcAddress(*hPsapi, "GetModuleBaseNameA"));
	*pGMI = reinterpret_cast<tGMI>(GetProcAddress(*hPsapi, "GetModuleInformation"));
	
	if (!*pEPM || !*pGMFNE || !*pGMBN || !*pGMI) {
		// we couldn't find all functions
		FreeLibrary(*hPsapi);
		return false;
	}
	
	return true;
}


bool allocMods(HMODULE **hMods, char **tt, char **tt2) {
	*hMods = reinterpret_cast<HMODULE*>(
		malloc(sizeof(HMODULE) * (TTBUFLEN / sizeof HMODULE))
	);
	*tt = reinterpret_cast<char*>(
		malloc(sizeof(char) * TTBUFLEN)
	);
	*tt2 = reinterpret_cast<char*>(
		malloc(sizeof(char) * TTBUFLEN)
	);
	return *hMods && *tt && *tt2;
}


void freeMods(HMODULE *hMods, char *tt, char *tt2) {
	if (tt2) {
		free(tt2);
	}
	if (tt) {
		free(tt);
	}
	if (hMods) {
		free(hMods);
	}
}


bool GetModuleListPSAPI(HANDLE hProcess) {
	DWORD i;
	DWORD cbNeeded;
	MODULEINFO mi;
	
	HINSTANCE hPsapi;
	tEPM pEPM;
	tGMFNE pGMFNE;
	tGMBN pGMBN;
	tGMI pGMI;
	if (!loadPsapi(&hPsapi, &pEPM, &pGMFNE, &pGMBN, &pGMI)) {
		return false;
	}
	
	int cnt = 0;
	
	HMODULE *hMods;
	char *tt;
	char *tt2;
	if (!allocMods(&hMods, &tt, &tt2)) {
		goto cleanup;
	}
	
	if (!pEPM(hProcess, hMods, TTBUFLEN, &cbNeeded)) {
		//_ftprintf(fLogFile, _T("%lu: EPM failed, GetLastError = %lu\n"), g_dwShowCount, gle);
		goto cleanup;
	}
	
	if (cbNeeded > TTBUFLEN) {
		//_ftprintf(fLogFile, _T("%lu: More than %lu module handles. Huh?\n"), g_dwShowCount, lenof(hMods));
		goto cleanup;
	}
	
	for (i = 0; i < cbNeeded / sizeof hMods[0]; i++) {
		// base address, size
		pGMI(hProcess, hMods[i], &mi, sizeof mi);
		// image file name
		tt[0] = 0;
		pGMFNE(hProcess, hMods[i], tt, TTBUFLEN);
		// module name
		tt2[0] = 0;
		pGMBN(hProcess, hMods[i], tt2, TTBUFLEN);
		
		DWORD dwRes = LoadModule(
			hProcess, tt, tt2, (DWORD64) mi.lpBaseOfDll, mi.SizeOfImage
		);
		if (dwRes != ERROR_SUCCESS) {
			GetLastError();
		}
		cnt++;
	}
	
cleanup:
	if (hPsapi) {
		FreeLibrary(hPsapi);
	}
	freeMods(hMods, tt, tt2);
	return cnt != 0;
}  // GetModuleListPSAPI


bool GetModuleListTH32(HANDLE hProcess, DWORD pid) {
	// CreateToolhelp32Snapshot()
	typedef HANDLE (__stdcall *tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
	// Module32First()
	typedef BOOL (__stdcall *tM32F)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	// Module32Next()
	typedef BOOL (__stdcall *tM32N)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
	
	// try both dlls...
	const TCHAR *dllname[] = { _T("kernel32.dll"), _T("tlhelp32.dll") };
	HINSTANCE hToolhelp = nullptr;
	tCT32S pCT32S = nullptr;
	tM32F pM32F = nullptr;
	tM32N pM32N = nullptr;
	
	HANDLE hSnap;
	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	BOOL keepGoing;
	size_t i;
	
	for (i = 0; i < (sizeof(dllname) / sizeof(dllname[0])); i++) {
		hToolhelp = LoadLibrary(dllname[i]);
		if (!hToolhelp) {
			continue;
		}
		
		pCT32S = (tCT32S) GetProcAddress(hToolhelp, "CreateToolhelp32Snapshot");
		pM32F = (tM32F) GetProcAddress(hToolhelp, "Module32First");
		pM32N = (tM32N) GetProcAddress(hToolhelp, "Module32Next");
		if (pCT32S && pM32F && pM32N) {
			break; // found the functions!
		}
		
		FreeLibrary(hToolhelp);
		hToolhelp = nullptr;
	}
	
	if (!hToolhelp) {
		return false;
	}
	
	hSnap = pCT32S(TH32CS_SNAPMODULE, pid);
	if (hSnap == reinterpret_cast<HANDLE>(-1)) {
		return false;
	}
	
	keepGoing = !!pM32F(hSnap, &me);
	int cnt = 0;
	while (keepGoing) {
		LoadModule(hProcess, me.szExePath, me.szModule, (DWORD64) me.modBaseAddr, me.modBaseSize);
		cnt++;
		keepGoing = !!pM32N(hSnap, &me);
	}
	
	CloseHandle(hSnap);
	FreeLibrary(hToolhelp);
	
	return cnt > 0;
} // GetModuleListTH32


bool LoadModules(HANDLE hProcess, DWORD dwProcessId) {
	// first try toolhelp32
	if (GetModuleListTH32(hProcess, dwProcessId)) {
		return true;
	}
	
	// then try psapi
	return GetModuleListPSAPI(hProcess);
}


void cleanInternal(HANDLE hProcess) {
	if (pSC) {
		pSC(hProcess); // SymCleanup
	}
	if (m_hDbhHelp) {
		FreeLibrary(m_hDbhHelp);
		m_hDbhHelp = nullptr;
	}
}


bool loadDbgHelpDll() {
	// First try to load the newsest one from
	TCHAR szTemp[4096];
	// But before we do this, we first check if the ".local" file exists
	if (GetModuleFileName(nullptr, szTemp, 4096) > 0) {
		_tcscat_s(szTemp, _T(".local"));
		if (GetFileAttributes(szTemp) == INVALID_FILE_ATTRIBUTES) {
			// ".local" file does not exist, so we can try to load the dbghelp.dll
			// from the "Debugging Tools for Windows"
			if (GetEnvironmentVariable(_T("ProgramFiles"), szTemp, 4096) > 0) {
				_tcscat_s(szTemp, _T("\\Debugging Tools for Windows\\dbghelp.dll"));
				// now check if the file exists:
				if (GetFileAttributes(szTemp) != INVALID_FILE_ATTRIBUTES) {
					m_hDbhHelp = LoadLibrary(szTemp);
				}
			}
				// Still not found? Then try to load the 64-Bit version:
			if (
				!m_hDbhHelp &&
				(GetEnvironmentVariable(_T("ProgramFiles"), szTemp, 4096) > 0)
			) {
				_tcscat_s(szTemp, _T("\\Debugging Tools for Windows 64-Bit\\dbghelp.dll"));
				if (GetFileAttributes(szTemp) != INVALID_FILE_ATTRIBUTES) {
					m_hDbhHelp = LoadLibrary(szTemp);
				}
			}
		}
	}
	
	// if not already loaded, try to load a default-one
	if (!m_hDbhHelp) {
		m_hDbhHelp = LoadLibrary(_T("dbghelp.dll"));
	}
	
	return m_hDbhHelp != nullptr;
}


bool loadDbgFuncs() {
	pSI = reinterpret_cast<tSI>(GetProcAddress(m_hDbhHelp, "SymInitialize"));
	pSC = reinterpret_cast<tSC>(GetProcAddress(m_hDbhHelp, "SymCleanup"));
	pSW = reinterpret_cast<tSW>(GetProcAddress(m_hDbhHelp, "StackWalk64"));
	pSGO = reinterpret_cast<tSGO>(GetProcAddress(m_hDbhHelp, "SymGetOptions"));
	pSSO = reinterpret_cast<tSSO>(GetProcAddress(m_hDbhHelp, "SymSetOptions"));
	pSFTA = reinterpret_cast<tSFTA>(GetProcAddress(m_hDbhHelp, "SymFunctionTableAccess64"));
	pSGLFA = reinterpret_cast<tSGLFA>(GetProcAddress(m_hDbhHelp, "SymGetLineFromAddr64"));
	pSGMB = reinterpret_cast<tSGMB>(GetProcAddress(m_hDbhHelp, "SymGetModuleBase64"));
	pSGMI = reinterpret_cast<tSGMI>(GetProcAddress(m_hDbhHelp, "SymGetModuleInfo64"));
	pSGSFA = reinterpret_cast<tSGSFA>(GetProcAddress(m_hDbhHelp, "SymGetSymFromAddr64"));
	pUDSN = reinterpret_cast<tUDSN>(GetProcAddress(m_hDbhHelp, "UnDecorateSymbolName"));
	pSLM = reinterpret_cast<tSLM>(GetProcAddress(m_hDbhHelp, "SymLoadModule64"));
	pSGSP = reinterpret_cast<tSGSP>(GetProcAddress(m_hDbhHelp, "SymGetSearchPath"));
	
	return (
		pSC && pSFTA && pSGMB && pSGMI && pSGO && pSGSFA && pSI && pSSO && pSW && pUDSN && pSLM
	);
}


bool Init(HANDLE hProcess, LPCSTR szSymPath) {
	// Dynamically load the Entry-Points for dbghelp.dll:
	if (!loadDbgHelpDll()) {
		return false;
	}
	
	if (!loadDbgFuncs()) {
		FreeLibrary(m_hDbhHelp);
		m_hDbhHelp = nullptr;
		pSC = nullptr;
		return false;
	}
	
	if (!pSI(hProcess, nullptr, FALSE)) {
		GetLastError();
	}
	
	DWORD symOptions = pSGO(); // SymGetOptions
	symOptions |= SYMOPT_LOAD_LINES;
	symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
	symOptions = pSSO(symOptions);
	
	char buf[STACKWALK_MAX_NAMELEN] = {0};
	if (pSGSP && !pSGSP(hProcess, buf, STACKWALK_MAX_NAMELEN)) {
		GetLastError();
	}
	
	return true;
}


inline void initEntityStrings(CallstackEntry *csEntry) {
	csEntry->name[0] = 0;
	csEntry->undName[0] = 0;
	csEntry->undFullName[0] = 0;
	csEntry->offsetFromSmybol = 0;
	csEntry->offsetFromLine = 0;
	csEntry->lineFileName[0] = 0;
	csEntry->lineNumber = 0;
	csEntry->loadedImageName[0] = 0;
	csEntry->moduleName[0] = 0;
}


inline void setSymType(SYM_TYPE SymType, CallstackEntry *csEntry) {
	switch (SymType) {
	case SymNone:
		csEntry->symTypeString = "-nosymbols-";
		break;
	case SymCoff:
		csEntry->symTypeString = "COFF";
		break;
	case SymCv:
		csEntry->symTypeString = "CV";
		break;
	case SymPdb:
		csEntry->symTypeString = "PDB";
		break;
	case SymExport:
		csEntry->symTypeString = "-exported-";
		break;
	case SymDeferred:
		csEntry->symTypeString = "-deferred-";
		break;
	case SymSym:
		csEntry->symTypeString = "SYM";
		break;
#if API_VERSION_NUMBER >= 9
	case SymDia:
		csEntry->symTypeString = "DIA";
		break;
#endif
	case 8: //SymVirtual:
		csEntry->symTypeString = "Virtual";
		break;
	default:
		csEntry->symTypeString = nullptr;
		break;
	}
}


void handleOutput(std::ofstream &outfile, LPCSTR buffer) {
	std::cerr << buffer << std::endl;
	
	if (outfile.is_open()) {
		outfile << buffer << std::endl;
		
		if (outfile.bad()) {
			std::cerr << "SegfaultHandler: Error writing to file." << std::endl;
		}
	}
}


void handleStackEntry(std::ofstream &outfile, CallstackEntryType eType, CallstackEntry *entry) {
	CHAR buffer[STACKWALK_MAX_NAMELEN];
	
	if ((eType != lastEntry) && (entry->offset != 0)) {
		if (entry->undFullName[0]) {
			strcpy_s(entry->name, entry->undFullName);
		} else if (entry->undName[0]) {
			strcpy_s(entry->name, entry->undName);
		}
		
		if (!entry->name[0] && !entry->lineFileName[0] && !entry->moduleName[0]) {
			return;
		}
		
		if (!entry->name[0]) {
			strcpy_s(entry->name, "(#unknown_function)");
		}
		
		if (!entry->lineFileName[0]) {
			strcpy_s(entry->lineFileName, "(#unknown_file)");
			if (!entry->moduleName[0]) {
				strcpy_s(entry->moduleName, "#unknown_moudle");
			}
			_snprintf_s(
				buffer, STACKWALK_MAX_NAMELEN, "%p (%s): %s: %s",
				(LPVOID) entry->offset, entry->moduleName,
				entry->lineFileName, entry->name
			);
		} else {
			_snprintf_s(
				buffer,
				STACKWALK_MAX_NAMELEN,
				"%s (%d): %s",
				entry->lineFileName, entry->lineNumber, entry->name
			);
		}
		handleOutput(outfile, buffer);
	}
}


BOOL __stdcall myReadProcMem(
	HANDLE hProcess,
	DWORD64 qwBaseAddress,
	PVOID lpBuffer,
	DWORD nSize,
	LPDWORD lpNumberOfBytesRead
) {
	if (!s_readMemoryFunction) {
		SIZE_T st;
		BOOL bRet = ReadProcessMemory(hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, &st);
		*lpNumberOfBytesRead = (DWORD) st;
		return bRet;
	}
	
	return s_readMemoryFunction(
		hProcess,
		qwBaseAddress,
		lpBuffer,
		nSize,
		lpNumberOfBytesRead,
		s_readMemoryFunction_UserData
	);
}


inline void iterateFrames(std::ofstream &outfile, HANDLE hProcess, HANDLE hThread, CONTEXT *c, void *_pSym) {
	IMAGEHLP_SYMBOL64 *pSym = reinterpret_cast<IMAGEHLP_SYMBOL64*>(_pSym);
	
	// init STACKFRAME for first call
	STACKFRAME64 s; // in/out stackframe
	memset(&s, 0, sizeof(s));
	DWORD imageType;
#ifdef _M_X64
	imageType = IMAGE_FILE_MACHINE_AMD64;
	s.AddrPC.Offset = c->Rip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c->Rsp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrStack.Offset = c->Rsp;
	s.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	imageType = IMAGE_FILE_MACHINE_IA64;
	s.AddrPC.Offset = c->StIIP;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c->IntSp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrBStore.Offset = c->RsBSP;
	s.AddrBStore.Mode = AddrModeFlat;
	s.AddrStack.Offset = c->IntSp;
	s.AddrStack.Mode = AddrModeFlat;
#else
	#error "Platform not supported!"
#endif
	
	CallstackEntry csEntry;
	
	IMAGEHLP_MODULE64_V2 Module;
	memset(&Module, 0, sizeof(Module));
	Module.SizeOfStruct = sizeof(Module);
	
	IMAGEHLP_LINE64 Line;
	memset(&Line, 0, sizeof(Line));
	Line.SizeOfStruct = sizeof(Line);
	
	for (int frameNum = 0; ; ++frameNum) {
		// get next stack frame (StackWalk64(), SymFunctionTableAccess64(), SymGetModuleBase64())
		// if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
		// assume that either you are done, or that the stack is so hosed that the next
		// deeper frame could not be found.
		// CONTEXT need not to be suplied if imageTyp is IMAGE_FILE_MACHINE_I386!
		if (!pSW(imageType, hProcess, hThread, &s, c, myReadProcMem, pSFTA, pSGMB, nullptr)) {
			GetLastError();
			break;
		}
		
		csEntry.offset = s.AddrPC.Offset;
		initEntityStrings(&csEntry);
		
		if (s.AddrPC.Offset == s.AddrReturn.Offset) {
			GetLastError();
			break;
		}
		
		if (s.AddrPC.Offset) {
			// we seem to have a valid PC
			// show procedure info (SymGetSymFromAddr64())
			if (pSGSFA(hProcess, s.AddrPC.Offset, &(csEntry.offsetFromSmybol), pSym)) {
				// TODO: Mache dies sicher...!
				strcpy_s(csEntry.name, pSym->Name);
				// UnDecorateSymbolName()
				pUDSN(
					pSym->Name, csEntry.undName, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY
				);
				pUDSN(
					pSym->Name, csEntry.undFullName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE
				);
			} else {
				GetLastError();
			}
			
			// show line number info, NT5.0-method (SymGetLineFromAddr64())
			if (pSGLFA) { // yes, we have SymGetLineFromAddr64()
				if (pSGLFA(
					hProcess,
					s.AddrPC.Offset,
					&(csEntry.offsetFromLine),
					&Line
				)) {
					csEntry.lineNumber = Line.LineNumber;
					// TODO: Mache dies sicher...!
					strcpy_s(csEntry.lineFileName, Line.FileName);
				} else {
					GetLastError();
				}
			} // yes, we have SymGetLineFromAddr64()
			
			// show module info (SymGetModuleInfo64())
			if (GetModuleInfo(hProcess, s.AddrPC.Offset, &Module)) {
				setSymType(Module.SymType, &csEntry);
				// TODO: Mache dies sicher...!
				strcpy_s(csEntry.moduleName, Module.ModuleName);
				csEntry.baseOfImage = Module.BaseOfImage;
				strcpy_s(csEntry.loadedImageName, Module.LoadedImageName);
			} else {
				GetLastError();
			}
		} // we seem to have a valid PC
		
		CallstackEntryType et = nextEntry;
		if (!frameNum) {
			et = firstEntry;
		}
		handleStackEntry(outfile, et, &csEntry);
		
		if (!s.AddrReturn.Offset) {
			handleStackEntry(outfile, lastEntry, &csEntry);
			SetLastError(ERROR_SUCCESS);
			break;
		}
	} // for (frameNum)
}


inline void buildSymPath(char *szSymPath) {
	szSymPath[0] = 0;
	
	strcat_s(szSymPath, nSymPathLen, ".;");
	
	const size_t kTempLen = 1024;
	char szTemp[kTempLen];
	// Now add the current directory:
	if (GetCurrentDirectoryA(kTempLen, szTemp) > 0) {
		szTemp[kTempLen-1] = 0;
		strcat_s(szSymPath, nSymPathLen, szTemp);
		strcat_s(szSymPath, nSymPathLen, ";");
	}
	
	// Now add the path for the main-module:
	if (GetModuleFileNameA(nullptr, szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		for (char *p = (szTemp+strlen(szTemp)-1); p >= szTemp; --p) {
			// locate the rightmost path separator
			if ((*p == '\\') || (*p == '/') || (*p == ':')) {
				*p = 0;
				break;
			}
		}  // for (search for path separator...)
		if (strlen(szTemp) > 0) {
			strcat_s(szSymPath, nSymPathLen, szTemp);
			strcat_s(szSymPath, nSymPathLen, ";");
		}
	}
	
	if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		strcat_s(szSymPath, nSymPathLen, szTemp);
		strcat_s(szSymPath, nSymPathLen, ";");
	}
	
	if (GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		strcat_s(szSymPath, nSymPathLen, szTemp);
		strcat_s(szSymPath, nSymPathLen, ";");
	}
	
	if (GetEnvironmentVariableA("SYSTEMROOT", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		strcat_s(szSymPath, nSymPathLen, szTemp);
		strcat_s(szSymPath, nSymPathLen, ";");
		// also add the "system32"-directory:
		strcat_s(szTemp, kTempLen, "\\system32");
		strcat_s(szSymPath, nSymPathLen, szTemp);
		strcat_s(szSymPath, nSymPathLen, ";");
	}
	
	if (GetEnvironmentVariableA("SYSTEMDRIVE", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		strcat_s(szSymPath, nSymPathLen, "SRV*");
		strcat_s(szSymPath, nSymPathLen, szTemp);
		strcat_s(szSymPath, nSymPathLen, "\\websymbols");
		strcat_s(szSymPath, nSymPathLen, "*http://msdl.microsoft.com/download/symbols;");
	} else {
		strcat_s(
			szSymPath,
			nSymPathLen,
			"SRV*c:\\websymbols*http://msdl.microsoft.com/download/symbols;"
		);
	}
}


bool loadModules(HANDLE hProcess) {
	DWORD dwProcessId = GetCurrentProcessId();
	// Build the sym-path:
	char *szSymPath = nullptr;
	
	szSymPath = reinterpret_cast<char*>(malloc(nSymPathLen));
	if (!szSymPath) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return false;
	}
	buildSymPath(szSymPath);
	
	// First Init the whole stuff...
	bool bRet = Init(hProcess, szSymPath);
	if (szSymPath) {
		free(szSymPath);
		szSymPath = nullptr;
	}
	
	if (!bRet) {
		GetLastError();
		SetLastError(ERROR_DLL_INIT_FAILED);
		return false;
	}
	
	bRet = LoadModules(hProcess, dwProcessId);
	
	return bRet;
}

inline void getValidContext(CONTEXT *dest) {
	memset(dest, 0, sizeof(CONTEXT));
	dest->ContextFlags = USED_CONTEXT_FLAGS;
	RtlCaptureContext(dest);
}


void showCallstack(std::ofstream &outfile) {
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	
	loadModules(hProcess);
	
	if (!m_hDbhHelp) {
		SetLastError(ERROR_DLL_INIT_FAILED);
		return;
	}
	
	s_readMemoryFunction = nullptr;
	s_readMemoryFunction_UserData = nullptr;
	
	CONTEXT c;
	getValidContext(&c);
	
	IMAGEHLP_SYMBOL64 *pSym = reinterpret_cast<IMAGEHLP_SYMBOL64*>(
		malloc(sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN)
	);
	
	if (pSym) {
		memset(pSym, 0, sizeof(IMAGEHLP_SYMBOL64) + STACKWALK_MAX_NAMELEN);
		pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
		pSym->MaxNameLength = STACKWALK_MAX_NAMELEN;
		
		iterateFrames(outfile, hProcess, hThread, &c, pSym);
		free(pSym);
	}
	
	ResumeThread(hThread);
}