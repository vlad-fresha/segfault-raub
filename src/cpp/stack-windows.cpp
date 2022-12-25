#include <tchar.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <windows.h>
#include <dbghelp.h>

#include "stack-windows.hpp"

#pragma comment(lib, "version.lib") // for "VerQueryValue"


constexpr int NAME_BUF_SIZE = 8096;
constexpr int DEFAULT_MODULE_COUNT = 1024;
constexpr int DEFAULT_MODULE_BUF_SIZE = DEFAULT_MODULE_COUNT * sizeof(HMODULE);
constexpr int STACKWALK_MAX_NAMELEN = 1024;
constexpr int MAX_MODULE_NAME32 = 255;
constexpr int TH32CS_SNAPMODULE = 0x00000008;
constexpr size_t nSymPathLen = 4096;

// Normally it should be enough to use 'CONTEXT_FULL' (better would be 'CONTEXT_ALL')
constexpr int USED_CONTEXT_FLAGS = CONTEXT_FULL;


typedef std::unique_ptr<std::remove_pointer_t<HMODULE>, decltype(&::FreeLibrary)> PtrModule;

struct IMAGEHLP_SYMBOL64_NAMED : IMAGEHLP_SYMBOL64 {
	char name[NAME_BUF_SIZE];
};
typedef std::unique_ptr<IMAGEHLP_SYMBOL64_NAMED> PtrSymbol;
typedef std::unique_ptr<char[]> PtrChars;

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
	IN HANDLE hProcess, IN PCSTR UserSearchPath, IN BOOL fInvadeProcess
);

// SymLoadModule64()
typedef DWORD64 (__stdcall *tSLM)(
	IN HANDLE hProcess, IN HANDLE hFile,
	IN PCSTR ImageName, IN PCSTR ModuleName, IN DWORD64 BaseOfDll, IN DWORD SizeOfDll
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
	PCSTR name, PSTR outputString, DWORD maxStringLength, DWORD flags
);

// SymGetSearchPath
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

// CreateToolhelp32Snapshot()
typedef HANDLE (__stdcall *tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
// Module32First()
typedef BOOL (__stdcall *tM32F)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
// Module32Next()
typedef BOOL (__stdcall *tM32N)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

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

PtrModule makePtrModule(const wchar_t *name) {
	PtrModule ptrModule(LoadLibrary(name), FreeLibrary);
	return ptrModule;
}


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


static inline void setSymType(SYM_TYPE SymType, const char **szSymType) {
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
	}
	if (szImg) {
		free(szImg);
	}
	if (szMod) {
		free(szMod);
	}
	return result;
}


static inline bool loadPsapi(
	PtrModule &ptrPsapi,
	tEPM *pEnumProcessModules,
	tGMFNE *pGetModuleFileNameExA,
	tGMBN *pGetModuleBaseNameA,
	tGMI *pGetModuleInformation
) {
	*pEnumProcessModules = reinterpret_cast<tEPM>(
		GetProcAddress(ptrPsapi.get(), "pEnumProcessModules")
	);
	*pGetModuleFileNameExA = reinterpret_cast<tGMFNE>(
		GetProcAddress(ptrPsapi.get(), "GetModuleFileNameExA")
	);
	*pGetModuleBaseNameA = reinterpret_cast<tGMFNE>(
		GetProcAddress(ptrPsapi.get(), "GetModuleBaseNameA")
	);
	*pGetModuleInformation = reinterpret_cast<tGMI>(
		GetProcAddress(ptrPsapi.get(), "GetModuleInformation")
	);
	
	return (
		*pEnumProcessModules && *pGetModuleFileNameExA && *pGetModuleBaseNameA && *pGetModuleInformation
	);
}


bool GetModuleListPSAPI(HANDLE hProcess) {
	tEPM pEnumProcessModules;
	tGMFNE pGetModuleFileNameExA;
	tGMBN pGetModuleBaseNameA;
	tGMI pGetModuleInformation;
	
	PtrModule ptrPsapi = makePtrModule(L"psapi.dll");
	if (!ptrPsapi.get() || !loadPsapi(ptrPsapi, &pEnumProcessModules, &pGetModuleFileNameExA, &pGetModuleBaseNameA, &pGetModuleInformation)) {
		return false;
	}
	
	int cnt = 0;
	
	std::unique_ptr<HMODULE[]> ptrModules(new (std::nothrow) HMODULE[DEFAULT_MODULE_COUNT]);
	if (!ptrModules.get()) {
		return false;
	}
	
	DWORD cbNeeded;
	if (!pEnumProcessModules(hProcess, ptrModules.get(), DEFAULT_MODULE_BUF_SIZE, &cbNeeded)) {
		return false;
	}
	
	if (cbNeeded > DEFAULT_MODULE_BUF_SIZE) {
		ptrModules.reset(new (std::nothrow) HMODULE[cbNeeded / sizeof(HMODULE)]);
		if (!ptrModules.get()) {
			return false;
		}
		if (!pEnumProcessModules(hProcess, ptrModules.get(), cbNeeded, &cbNeeded)) {
			return false;
		}
	}
	
	if (!cbNeeded) {
		return false;
	}
	
	int countModulesFinal = cbNeeded / sizeof(HMODULE);
	
	PtrChars ptrFileName(new (std::nothrow) char[NAME_BUF_SIZE]);
	PtrChars ptrBaseName(new (std::nothrow) char[NAME_BUF_SIZE]);
	
	if (!ptrFileName.get() || !ptrBaseName.get()) {
		return false;
	}
	
	MODULEINFO mi;
	for (int i = 0; i < countModulesFinal; i++) {
		// base address, size
		pGetModuleInformation(hProcess, ptrModules[i], &mi, sizeof mi);
		// image file name
		ptrModules[0] = 0;
		pGetModuleFileNameExA(hProcess, ptrModules[i], ptrFileName.get(), NAME_BUF_SIZE);
		// module name
		ptrModules[0] = 0;
		pGetModuleBaseNameA(hProcess, ptrModules[i], ptrBaseName.get(), NAME_BUF_SIZE);
		
		DWORD dwRes = LoadModule(
			hProcess,
			ptrFileName.get(),
			ptrBaseName.get(),
			reinterpret_cast<DWORD64>(mi.lpBaseOfDll),
			mi.SizeOfImage
		);
		if (dwRes != ERROR_SUCCESS) {
			GetLastError();
		}
		cnt++;
	}
	
	return cnt != 0;
}  // GetModuleListPSAPI


const std::vector<std::wstring> dllNames = { L"kernel32.dll", L"tlhelp32.dll" };

bool GetModuleListTH32(HANDLE hProcess, DWORD pid) {
	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	
	for (auto name: dllNames) {
		PtrModule ptrToolHelp = makePtrModule(name.c_str());
		if (!ptrToolHelp.get()) {
			continue;
		}
		
		tCT32S pCreateToolhelp32Snapshot = reinterpret_cast<tCT32S>(
			GetProcAddress(ptrToolHelp.get(), "CreateToolhelp32Snapshot")
		);
		tM32F pModule32First = reinterpret_cast<tM32F>(
			GetProcAddress(ptrToolHelp.get(), "Module32First")
		);
		tM32N pModule32Next = reinterpret_cast<tM32N>(
			GetProcAddress(ptrToolHelp.get(), "Module32Next")
		);
		
		if (!pCreateToolhelp32Snapshot || !pModule32First || !pModule32Next) {
			continue;
		}
		
		HANDLE hSnap = pCreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
		if (hSnap == reinterpret_cast<HANDLE>(-1)) {
			return false;
		}
		
		bool keepGoing = !!pModule32First(hSnap, &me);
		int cnt = 0;
		while (keepGoing) {
			LoadModule(
				hProcess, me.szExePath, me.szModule,
				reinterpret_cast<DWORD64>(me.modBaseAddr), me.modBaseSize
			);
			cnt++;
			keepGoing = !!pModule32Next(hSnap, &me);
		}
		
		CloseHandle(hSnap);
		return cnt > 0;
	}
	
	return false;
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
					m_hDbhHelp = LoadLibrary(szTemp); // NOLINT
				}
			}
				// Still not found? Then try to load the 64-Bit version:
			if (
				!m_hDbhHelp &&
				(GetEnvironmentVariable(_T("ProgramFiles"), szTemp, 4096) > 0)
			) {
				_tcscat_s(szTemp, _T("\\Debugging Tools for Windows 64-Bit\\dbghelp.dll"));
				if (GetFileAttributes(szTemp) != INVALID_FILE_ATTRIBUTES) {
					m_hDbhHelp = LoadLibrary(szTemp); // NOLINT
				}
			}
		}
	}
	
	// if not already loaded, try to load a default-one
	if (!m_hDbhHelp) {
		m_hDbhHelp = LoadLibrary(_T("dbghelp.dll")); // NOLINT
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


const size_t kTempLen = 1024;
char szTemp[kTempLen];

static inline std::string _buildSymbolsPath() {
	std::stringstream ss;
	ss << ".;";
	
	// Now add the current directory:
	if (GetCurrentDirectoryA(kTempLen, szTemp) > 0) {
		szTemp[kTempLen-1] = 0;
		ss << szTemp << ";";
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
			ss << szTemp << ";";
		}
	}
	
	if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		ss << szTemp << ";";
	}
	
	if (GetEnvironmentVariableA("_NT_ALTERNATE_SYMBOL_PATH", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		ss << szTemp << ";";
	}
	
	if (GetEnvironmentVariableA("SYSTEMROOT", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		ss << szTemp << ";";
		ss << szTemp << "\\system32;";
	}
	
	ss << "SRV*";
	if (GetEnvironmentVariableA("SYSTEMDRIVE", szTemp, kTempLen) > 0) {
		szTemp[kTempLen-1] = 0;
		ss << szTemp;
	} else {
		ss << "c:";
	}
	ss << "\\websymbols*http://msdl.microsoft.com/download/symbols;";
	
	return ss.str();
}


static inline bool _init(HANDLE hProcess) {
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
	
	std::string symbolsPath = _buildSymbolsPath();
	if (!pSI(hProcess, symbolsPath.c_str(), FALSE)) {
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


static inline void initEntityStrings(CallstackEntry *csEntry) {
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


static inline void setSymType(SYM_TYPE SymType, CallstackEntry *csEntry) {
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


void handleOutput(std::ofstream &outfile, const std::string& text) {
	std::cerr << text << std::endl;
	
	if (outfile.is_open()) {
		outfile << text << std::endl;
		
		if (outfile.bad()) {
			std::cerr << "SegfaultHandler: Error writing to file." << std::endl;
		}
	}
}


void handleStackEntry(std::ofstream &outfile, CallstackEntryType eType, CallstackEntry *entry) {
	if (eType == lastEntry || !entry->offset) {
		return;
	}
	
	CHAR *pName = entry->name;
	if (entry->undFullName[0]) {
		CHAR *pName = entry->undFullName;
		// strcpy_s(entry->name, entry->undFullName);
	} else if (entry->undName[0]) {
		CHAR *pName = entry->undName;
		// strcpy_s(entry->name, entry->undName);
	}
	
	if (!pName[0] && !entry->lineFileName[0] && !entry->moduleName[0]) {
		return;
	}
	
	std::stringstream ss;
	if (!entry->lineFileName[0]) {
		ss << "0x" << std::hex << static_cast<size_t>(entry->offset);
		
		if (entry->moduleName[0]) {
			ss << " [" << entry->moduleName << "]";
		}
		
		if (pName[0]) {
			ss << ": " << pName;
		}
	} else {
		ss << entry->lineFileName[0] << " (" << entry->lineNumber << "): " << pName;
	}
	
	handleOutput(outfile, ss.str());
}


BOOL __stdcall myReadProcMem(
	HANDLE hProcess,
	DWORD64 qwBaseAddress,
	PVOID lpBuffer,
	DWORD nSize,
	LPDWORD lpNumberOfBytesRead
) {
	SIZE_T st;
	BOOL bRet = ReadProcessMemory(hProcess, (LPVOID) qwBaseAddress, lpBuffer, nSize, &st);
	*lpNumberOfBytesRead = static_cast<DWORD>(st);
	return bRet;
}


static inline CONTEXT getValidContext() {
	CONTEXT ctx;
	memset(&ctx, 0, sizeof(CONTEXT));
	ctx.ContextFlags = USED_CONTEXT_FLAGS;
	RtlCaptureContext(&ctx);
	return ctx;
}


static inline void iterateFrames(std::ofstream &outfile, PtrSymbol &ptrSymbol) {
	CONTEXT ctx = getValidContext();
	
	// init STACKFRAME for first call
	STACKFRAME64 s; // in/out stackframe
	memset(&s, 0, sizeof(s));
	DWORD imageType;
#ifdef _M_X64
	imageType = IMAGE_FILE_MACHINE_AMD64;
	s.AddrPC.Offset = ctx.Rip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = ctx.Rsp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrStack.Offset = ctx.Rsp;
	s.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	imageType = IMAGE_FILE_MACHINE_IA64;
	s.AddrPC.Offset = ctx.StIIP;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = ctx.IntSp;
	s.AddrFrame.Mode = AddrModeFlat;
	s.AddrBStore.Offset = ctx.RsBSP;
	s.AddrBStore.Mode = AddrModeFlat;
	s.AddrStack.Offset = ctx.IntSp;
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
	
	HANDLE hThread = GetCurrentThread();
	HANDLE hProcess = GetCurrentProcess();
	
	for (int frameNum = 0; ; ++frameNum) {
		// get next stack frame (StackWalk64(), SymFunctionTableAccess64(), SymGetModuleBase64())
		// if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
		// assume that either you are done, or that the stack is so hosed that the next
		// deeper frame could not be found.
		// CONTEXT need not to be suplied if imageTyp is IMAGE_FILE_MACHINE_I386!
		if (!pSW(imageType, hProcess, hThread, &s, &ctx, myReadProcMem, pSFTA, pSGMB, nullptr)) {
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
			if (pSGSFA(hProcess, s.AddrPC.Offset, &(csEntry.offsetFromSmybol), ptrSymbol.get())) {
				// TODO: Mache dies sicher...!
				strcpy_s(csEntry.name, ptrSymbol->Name);
				// UnDecorateSymbolName()
				pUDSN(
					ptrSymbol->Name, csEntry.undName, STACKWALK_MAX_NAMELEN, UNDNAME_NAME_ONLY
				);
				pUDSN(
					ptrSymbol->Name, csEntry.undFullName, STACKWALK_MAX_NAMELEN, UNDNAME_COMPLETE
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


static inline void _preloadInvolvedModules() {
	HANDLE hProcess = GetCurrentProcess();
	bool bRet = _init(hProcess);
	
	if (!bRet) {
		GetLastError();
		SetLastError(ERROR_DLL_INIT_FAILED);
		return;
	}
	
	DWORD dwProcessId = GetCurrentProcessId();
	LoadModules(hProcess, dwProcessId);
}


void showCallstack(std::ofstream &outfile) {
	_preloadInvolvedModules();
	
	if (!m_hDbhHelp) {
		SetLastError(ERROR_DLL_INIT_FAILED);
		return;
	}
	
	PtrSymbol ptrSymbol = std::make_unique<IMAGEHLP_SYMBOL64_NAMED>();
	if (!ptrSymbol.get()) {
		return;
	}
	
	memset(ptrSymbol.get(), 0, sizeof(IMAGEHLP_SYMBOL64_NAMED));
	ptrSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	ptrSymbol->MaxNameLength = STACKWALK_MAX_NAMELEN;
	
	iterateFrames(outfile, ptrSymbol);
}
