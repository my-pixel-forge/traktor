#include <CoreFoundation/CFBundle.h>
#include <Foundation/Foundation.h>
#if !defined(__IOS__)
#	include <crt_externs.h>
#	include <mach-o/dyld.h>
#else
#	import <UIKit/UIKit.h>
#endif
#include <glob.h>
#include <pwd.h>
#include <spawn.h>
#include <stdlib.h>
#include <unistd.h>
#include <mach/mach_host.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include "Core/Io/FileSystem.h"
#include "Core/Io/Path.h"
#include "Core/Io/Utf8Encoding.h"
#include "Core/Log/Log.h"
#include "Core/Misc/String.h"
#include "Core/Misc/StringSplit.h"
#include "Core/Misc/TString.h"
#include "Core/Singleton/SingletonManager.h"
#include "Core/System/Environment.h"
#include "Core/System/OS.h"
#include "Core/System/ResolveEnv.h"
#include "Core/System/OsX/ProcessOsX.h"
#include "Core/System/OsX/SharedMemoryOsX.h"

namespace traktor
{
	namespace
	{

#if defined(__IOS__)
NSString* makeNSString(const std::wstring& str)
{
	std::string mbs = wstombs(Utf8Encoding(), str);
	return [[NSString alloc] initWithCString: mbs.c_str() encoding: NSUTF8StringEncoding];
}
#endif

	}

T_IMPLEMENT_RTTI_CLASS(L"traktor.OS", OS, Object)

OS& OS::getInstance()
{
	static OS* s_instance = nullptr;
	if (!s_instance)
	{
		s_instance = new OS();
		s_instance->addRef(nullptr);
		SingletonManager::getInstance().add(s_instance);
	}
	return *s_instance;
}

std::wstring OS::getName() const
{
	return L"macOS";	
}

std::wstring OS::getIdentifier() const
{
	return L"macos";
}

uint32_t OS::getCPUCoreCount() const
{
	host_basic_info_data_t hostInfo;
	mach_msg_type_number_t infoCount;
	
	infoCount = HOST_BASIC_INFO_COUNT;
	host_info(mach_host_self(), HOST_BASIC_INFO, (host_info_t)&hostInfo, &infoCount);
	
	return uint32_t(hostInfo.max_cpus);
}

Path OS::getExecutable() const
{
#if !defined(__IOS__)
	char path[1024];
	uint32_t size = sizeof(path);
	if (_NSGetExecutablePath(path, &size) == 0)
		return Path(mbstows(path));
	else
		return Path(L"");
#else
	return Path(L"");
#endif
}

std::wstring OS::getCommandLine() const
{
	return L"";
}

std::wstring OS::getComputerName() const
{
	char name[MAXHOSTNAMELEN];
	if (gethostname(name, sizeof_array(name)) != -1)
		return mbstows(name);
	return L"";
}

std::wstring OS::getCurrentUser() const
{
	passwd* pwd = getpwuid(geteuid());
	if (!pwd)
		return L"";

	const char* who = pwd->pw_name;
	if (!who)
		return L"";

	return mbstows(who);
}

std::wstring OS::getUserHomePath() const
{
    glob_t* globbuf = (glob_t*)alloca(sizeof(glob_t));
	char path[] = { "~" };
	
	if (glob(path, GLOB_TILDE, NULL, globbuf) == 0)
	{
		char* ep = globbuf->gl_pathv[0];
		return mbstows(ep);
	}
	
	return L"";
}

std::wstring OS::getUserApplicationDataPath() const
{
	return getUserHomePath();
}

std::wstring OS::getWritableFolderPath() const
{
#if defined(__IOS__)
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString* documentsDirectory = [paths objectAtIndex: 0];
	return mbstows([documentsDirectory UTF8String]);
#else
	return getUserHomePath() + L"/Library";
#endif
}

bool OS::openFile(const std::wstring& file) const
{
#if !defined(__IOS__)
	system(("open " + wstombs(file)).c_str());
	return true;
#else
	NSString* fs = makeNSString(file);
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString: fs]];
	[fs release];	
    return true;
#endif
}

bool OS::editFile(const std::wstring& file) const
{
	return false;
}

bool OS::exploreFile(const std::wstring& file) const
{
	return false;
}

bool OS::setEnvironment(const std::wstring& name, const std::wstring& value) const
{
	return false;
}

Ref< Environment > OS::getEnvironment() const
{
	Ref< Environment > env = new Environment();
	
#if !defined(__IOS__)
	char** environ = *_NSGetEnviron();
	for (char** e = environ; *e; ++e)
	{
		std::wstring pair(mbstows(*e));
		size_t p = pair.find('=');
		if (p != pair.npos)
		{
			std::wstring key = pair.substr(0, p);
			std::wstring value = pair.substr(p + 1);
			env->set(key, value);
		}
	}
#endif

	return env;
}

bool OS::getEnvironment(const std::wstring& name, std::wstring& outValue) const
{
	outValue = L"";

	// Get values from Info.plist.
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle)
	{
		bool gotValue = false;
		
		if (name == L"BUNDLE_PATH")	// /path-to-bundle/MyApplication.app
		{
			if (mainBundle)
			{
				CFURLRef appUrl = CFBundleCopyBundleURL(mainBundle);
				if (appUrl)
				{
					char bundlePath[PATH_MAX];
					CFURLGetFileSystemRepresentation(appUrl, TRUE, (uint8_t*)bundlePath, PATH_MAX);
					CFRelease(appUrl);
				
					outValue = mbstows(bundlePath);
					gotValue = true;
				}
			}
		}
		
		if (gotValue)
			return true;

		// Try to get value from bundle.
		CFStringRef keyStr = CFStringCreateWithCString(kCFAllocatorDefault, wstombs(name).c_str(), kCFStringEncodingMacRoman);
		if (keyStr)
		{
			CFStringRef valueRef = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(mainBundle, keyStr);
			if (valueRef)
			{
				const char* valueStr = CFStringGetCStringPtr(valueRef, kCFStringEncodingMacRoman);
				if (valueStr)
				{
					outValue = mbstows(valueStr);
					
					// \hack Replace "\$" sequences with plain "$" as Info.plist preprocessor
					// seems to intercept and remove those.
					outValue = replaceAll(outValue, L"\\$", L"$");
					
					gotValue = true;
				}
			}
			CFRelease(keyStr);
		}
		
		if (gotValue)
			return true;
	}
	
	// User home path.
	if (name == L"HOME_PATH")
	{
		outValue = getUserHomePath();
		return true;
	}

	// Ordinary variables; read from process environment.
	const char* env = getenv(wstombs(name).c_str());
	if (!env)
		return false;
		
	outValue = mbstows(env);
	return true;
}

Ref< IProcess > OS::execute(
	const std::wstring& commandLine,
	const Path& workingDirectory,
	const Environment* env,
	uint32_t flags
) const
{
#if !defined(__IOS__)

	posix_spawn_file_actions_t* fileActions = 0;
	char cwd[512];
	char* envv[256];
	char* argv[64];
	int envc = 0;
	int argc = 0;
	int err;
	pid_t pid;
	int childStdOut[2] = { 0 };
	int childStdErr[2] = { 0 };
	std::wstring executable;
	std::wstring arguments;

	// Resolve entire command line.
	std::wstring resolvedCommandLine = resolveEnv(commandLine, env);

	// Extract executable file from command line.
	if (resolvedCommandLine.empty())
		return nullptr;
	if (resolvedCommandLine[0] == L'\"')
	{
		size_t i = resolvedCommandLine.find(L'\"', 1);
		if (i == resolvedCommandLine.npos)
			return nullptr;
		executable = resolvedCommandLine.substr(1, i - 1);
		arguments = trim(resolvedCommandLine.substr(i + 1));
	}
	else
	{
		size_t i = resolvedCommandLine.find(L' ');
		if (i != resolvedCommandLine.npos)
		{
			executable = resolvedCommandLine.substr(0, i);
			arguments = trim(resolvedCommandLine.substr(i + 1));
		}
		else
			executable = resolvedCommandLine;
	}

	char wd[512] = { 0 };
	Path awd = FileSystem::getInstance().getAbsolutePath(workingDirectory);
	strcpy(wd, wstombs(awd.getPathNameNoVolume()).c_str());

	// Convert all arguments; append bash if executing shell script.
	if (endsWith(executable, L".sh"))
		argv[argc++] = strdup("/bin/sh");
	else
	{
		// Ensure file has executable permission.
		struct stat st;
		if (stat(wstombs(executable).c_str(), &st) == 0)
			chmod(wstombs(executable).c_str(), st.st_mode | S_IXUSR);
	}
	
	argv[argc++] = strdup(wstombs(executable).c_str());
	size_t i = 0;
	while (i < arguments.length())
	{
		if (arguments[i] == L'\"')
		{
			size_t j = arguments.find(L'\"', i + 1);
			if (j != arguments.npos)
			{
				std::wstring tmp = trim(arguments.substr(i + 1, j - i - 1));
				argv[argc++] = strdup(wstombs(tmp).c_str());
				i = j + 1;
			}
			else
				return 0;
		}
		else
		{
			size_t j = arguments.find(L' ', i + 1);
			if (j != arguments.npos)
			{
				std::wstring tmp = trim(arguments.substr(i, j - i));
				argv[argc++] = strdup(wstombs(tmp).c_str());
				i = j + 1;
			}
			else
			{
				std::wstring tmp = trim(arguments.substr(i));
				argv[argc++] = strdup(wstombs(tmp).c_str());
				break;
			}
		}
	}

	// Convert environment variables; don't pass "DYLIB_LIBRARY_PATH" along as we
	// don't want child process searching our products by default.
	if (env)
	{
		const auto& v = env->get();
		for (auto i = v.begin(); i != v.end(); ++i)
		{
			if (i->first != L"DYLD_LIBRARY_PATH")
				envv[envc++] = strdup(wstombs(i->first + L"=" + i->second).c_str());
		}
	}
	else
	{
		Ref< Environment > env2 = getEnvironment();
		const auto& v = env2->get();
		for (auto i = v.begin(); i != v.end(); ++i)
		{
			if (i->first != L"DYLD_LIBRARY_PATH")
				envv[envc++] = strdup(wstombs(i->first + L"=" + i->second).c_str());
		}
	}

	// Terminate argument and environment vectors.
	envv[envc] = nullptr;
	argv[argc] = nullptr;

	// Redirect standard IO.
	if ((flags & EfRedirectStdIO) != 0)
	{
		pipe(childStdOut);
		pipe(childStdErr);

		fileActions = new posix_spawn_file_actions_t;
		posix_spawn_file_actions_init(fileActions);
		posix_spawn_file_actions_addchdir_np(fileActions, wd);
		posix_spawn_file_actions_adddup2(fileActions, childStdOut[1], STDOUT_FILENO);
		posix_spawn_file_actions_addclose(fileActions, childStdOut[0]);
		posix_spawn_file_actions_adddup2(fileActions, childStdErr[1], STDERR_FILENO);
		posix_spawn_file_actions_addclose(fileActions, childStdErr[0]);

		// Spawn process.
		err = posix_spawn(&pid, argv[0], fileActions, 0, argv, envv);
	}
	else
	{
		fileActions = new posix_spawn_file_actions_t;
		posix_spawn_file_actions_init(fileActions);
		posix_spawn_file_actions_addchdir_np(fileActions, wd);

		// Spawn process.
		err = posix_spawn(&pid, argv[0], fileActions, 0, argv, envv);
	}
	
	if (err != 0)
		return nullptr;

	return new ProcessOsX(pid, fileActions, childStdOut[0], childStdErr[0]);
	
#else
	return nullptr;
#endif
}

Ref< ISharedMemory > OS::createSharedMemory(const std::wstring& name, uint32_t size) const
{
	Ref< SharedMemoryOsX > shm = new SharedMemoryOsX();
	if (shm->create(name, size))
		return shm;
	else
		return nullptr;	
}

bool OS::setOwnProcessPriorityBias(int32_t priorityBias)
{
	return false;
}

bool OS::whereIs(const std::wstring& executable, Path& outPath) const
{
	if (executable == L"blender")
	{
		outPath = L"/Applications/Blender.app/Contents/MacOS/Blender";
		return true;
	}
	return false;
}

OS::OS()
:	m_handle(0)
{
}

OS::~OS()
{
}

void OS::destroy()
{
	T_SAFE_RELEASE(this);
}

}
