#include <list>
#include <Core/Io/FileSystem.h>
#include <Core/Io/Reader.h>
#include <Core/Io/StreamCopy.h>
#include <Core/Io/Writer.h>
#include <Core/Log/Log.h>
#include <Core/Misc/Adler32.h>
#include <Core/Misc/CommandLine.h>
#include <Core/Misc/SafeDestroy.h>
#include <Core/Settings/PropertyString.h>
#include <Core/System/IProcess.h>
#include <Core/System/OS.h>
#include <Core/Thread/Thread.h>
#include <Core/Thread/ThreadManager.h>
#include <Net/Network.h>
#include <Net/SocketAddressIPv4.h>
#include <Net/SocketStream.h>
#include <Net/TcpSocket.h>
#include <Net/Discovery/DiscoveryManager.h>
#include <Net/Discovery/NetworkService.h>
#include <Ui/Application.h>
#include <Ui/Bitmap.h>
#include <Ui/Clipboard.h>
#include <Ui/Command.h>
#include <Ui/FunctionHandler.h>
#include <Ui/MenuItem.h>
#include <Ui/MessageBox.h>
#include <Ui/NotificationIcon.h>
#include <Ui/PopupMenu.h>
#include <Ui/Events/MouseEvent.h>
#if defined(_WIN32)
#	include <Ui/Win32/EventLoopWin32.h>
#	include <Ui/Win32/WidgetFactoryWin32.h>
#	undef MessageBox
#endif

#if defined(_WIN32)
#	include "Resources/NotificationBusy.h"
#	include "Resources/NotificationIdle.h"
#endif

using namespace traktor;

namespace
{

const uint16_t c_listenPort = 50001;
const uint8_t c_msgDeploy = 1;
const uint8_t c_msgLaunchProcess = 2;
const uint8_t c_errNone = 0;
const uint8_t c_errIoFailed = 1;
const uint8_t c_errLaunchFailed = 2;
const uint8_t c_errUnknown = 255;

std::wstring g_scratchPath;
std::map< std::wstring, uint32_t > g_fileHashes;

#if defined(_WIN32)
Ref< ui::PopupMenu > g_popupMenu;
Ref< ui::NotificationIcon > g_notificationIcon;

void eventNotificationButtonDown(ui::Event* event)
{
	ui::MouseEvent* mouseEvent = checked_type_cast< ui::MouseEvent* >(event);

	Ref< ui::MenuItem > item = g_popupMenu->show(mouseEvent->getPosition());
	if (!item)
		return;

	if (item->getCommand() == L"RemoteServer.CopyScratch")
	{
		ui::Clipboard* clipboard = ui::Application::getInstance()->getClipboard();
		if (clipboard)
			clipboard->setText(g_scratchPath);
	}
	else if (item->getCommand() == L"RemoteServer.Exit")
	{
		if (ui::MessageBox::show(L"Sure you want to exit RemoteServer?", L"Exit", ui::MbIconQuestion | ui::MbYesNo) == ui::DrYes)
			ui::Application::getInstance()->exit(0);
	}
}
#endif

uint8_t handleDeploy(net::TcpSocket* clientSocket)
{
	net::SocketStream clientStream(clientSocket, true, true, 1000);
	Reader reader(&clientStream);
	Writer writer(&clientStream);
	std::wstring pathName;
	uint32_t size;
	uint32_t hash;

	reader >> pathName;
	reader >> size;
	reader >> hash;

	traktor::log::info << L"Receiving file \"" << pathName << L"\", " << size << L" byte(s)" << Endl;

	Path path(g_scratchPath + L"/" + pathName);
	bool outOfSync = true;

	std::map< std::wstring, uint32_t >::const_iterator i = g_fileHashes.find(path.getPathName());
	if (i != g_fileHashes.end())
	{
		// File has already been hashed once; check only against previous hash.
		if (i->second == hash)
		{
			// Hashes match; finally ensure file still exist, could have been manually removed.
			if (FileSystem::getInstance().exist(path))
			{
				traktor::log::info << L"File up-to-date; skipping (1)" << Endl;
				outOfSync = false;
			}
		}
	}
	else
	{
		// File hasn't been hashed; calculate hash from file.
		Ref< traktor::IStream > fileStream = FileSystem::getInstance().open(path, File::FmRead);
		if (fileStream)
		{
			Adler32 adler;
			adler.begin();

			uint8_t buffer[4096];
			int32_t nread;
			while ((nread = fileStream->read(buffer, sizeof(buffer))) > 0)
				adler.feed(buffer, nread);

			adler.end();

			fileStream->close();
			fileStream = 0;

			if (adler.get() == hash)
			{
				traktor::log::info << L"File up-to-date; skipping (2)" << Endl;
				outOfSync = false;
			}
		}
	}

	if (outOfSync)
	{
		writer << uint8_t(1);

		FileSystem::getInstance().makeAllDirectories(path.getPathOnly());

		Ref< traktor::IStream > fileStream = FileSystem::getInstance().open(path, File::FmWrite);
		if (!fileStream)
		{
			traktor::log::error << L"Unable to create file \"" << pathName << L"\"" << Endl;
			return c_errIoFailed;
		}

		if (!StreamCopy(fileStream, &clientStream).execute(size))
		{
			traktor::log::error << L"Unable to receive file \"" << pathName << L"\"" << Endl;
			return c_errIoFailed;
		}

		traktor::log::info << L"File \"" << pathName << L"\" received successfully" << Endl;

		fileStream->close();
		fileStream = 0;
	}
	else
		writer << uint8_t(0);

	g_fileHashes[path.getPathName()] = hash;

	return c_errNone;
}

uint8_t handleLaunchProcess(net::Socket* clientSocket)
{
	net::SocketStream clientStream(clientSocket, true, true, 1000);
	Reader reader(&clientStream);
	Writer writer(&clientStream);
	std::wstring pathName;
	std::wstring arguments;
	bool wait;
	int32_t exitCode;

	reader >> pathName;
	reader >> arguments;
	reader >> wait;

	traktor::log::info << L"Launching \"" << pathName << L"\"" << Endl;
	traktor::log::info << L"\targuments \"" << arguments << L"\"" << Endl;

	Path path(g_scratchPath + L"/" + pathName);
	Ref< IProcess > process = OS::getInstance().execute(path, arguments, g_scratchPath, 0, false, false, false);
	if (!process)
	{
		traktor::log::error << L"Unable to launch process \"" << pathName << L"\"" << Endl;
		writer << uint8_t(c_errLaunchFailed);
		return c_errLaunchFailed;
	}

	if (wait)
	{
		process->wait();
		exitCode = process->exitCode();
	}

	writer << uint8_t(c_errNone);
	return c_errNone;
}

void threadProcessClient(Ref< net::TcpSocket > clientSocket)
{
	uint8_t msg;
	uint8_t ret;

	while (!ThreadManager::getInstance().getCurrentThread()->stopped())
	{
		int32_t res = clientSocket->select(true, false, false, 1000);
		if (res < 0)
		{
			traktor::log::info << L"Client terminated (1)" << Endl;
			break;
		}

		if (res == 0)
			continue;

		int32_t md = clientSocket->recv();
		if (md < 0)
		{
			traktor::log::info << L"Client terminated (2)" << Endl;
			break;
		}

		msg = uint8_t(md);
		switch (msg)
		{
		case c_msgDeploy:
			ret = handleDeploy(clientSocket);
			break;

		case c_msgLaunchProcess:
			ret = handleLaunchProcess(clientSocket);
			break;

		default:
			traktor::log::error << L"Invalid message ID from client; " << int32_t(msg) << Endl;
			ret = c_errUnknown;
			break;
		}

		if (ret != c_errNone)
			break;
	}

	clientSocket->close();
	clientSocket = 0;
}

}

#if !defined(_WIN32) || defined(_CONSOLE)
int main(int argc, const char** argv)
{
	CommandLine cmdLine(argc, argv);
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR szCmdLine, int)
{
	std::vector< std::wstring > argv;

	TCHAR szFilename[MAX_PATH] = _T("");
	GetModuleFileName(NULL, szFilename, sizeof(szFilename));
	argv.push_back(tstows(szFilename));

	Split< std::wstring >::any(mbstows(szCmdLine), L" \t", argv);
	CommandLine cmdLine(argv);
#endif

#if defined(_WIN32)
	ui::Application::getInstance()->initialize(
		new ui::EventLoopWin32(),
		new ui::WidgetFactoryWin32()
	);
#endif

	traktor::log::info << L"Traktor RemoteServer 1.3" << Endl;

	if (cmdLine.getCount() <= 0)
	{
		traktor::log::error << L"Usage: RemoteServer (Scratch directory)" << Endl;
		return 1;
	}

	g_scratchPath = cmdLine.getString(0);

#if defined(_WIN32)
	g_popupMenu = new ui::PopupMenu();
	g_popupMenu->create();
	g_popupMenu->add(new ui::MenuItem(ui::Command(L"RemoteServer.CopyScratch"), L"Copy Scratch Directory"));
	g_popupMenu->add(new ui::MenuItem(ui::Command(L"RemoteServer.Exit"), L"Exit"));

	g_notificationIcon = new ui::NotificationIcon();
	g_notificationIcon->create(L"Traktor RemoteServer 1.3 (" + g_scratchPath + L")", ui::Bitmap::load(c_ResourceNotificationIdle, sizeof(c_ResourceNotificationIdle), L"png"));
	g_notificationIcon->addButtonDownEventHandler(ui::createFunctionHandler(&eventNotificationButtonDown));
#endif

	net::Network::initialize();

	// Create server socket.
	Ref< net::TcpSocket > serverSocket = new net::TcpSocket();
	if (!serverSocket->bind(net::SocketAddressIPv4(c_listenPort)))
	{
		traktor::log::error << L"Unable to bind server socket to port " << c_listenPort << Endl;
		return 1;
	}

	if (!serverSocket->listen())
	{
		traktor::log::error << L"Unable to listen on server socket" << Endl;
		return 2;
	}

	// Create discovery manager and publish ourself.
	Ref< net::DiscoveryManager > discoveryManager = new net::DiscoveryManager();
	if (!discoveryManager->create(false))
	{
		traktor::log::error << L"Unable to create discovery manager" << Endl;
		return 3;
	}

	net::SocketAddressIPv4::Interface itf;
	if (!net::SocketAddressIPv4::getBestInterface(itf))
	{
		traktor::log::error << L"Unable to get interfaces" << Endl;
		return 4;
	}

	Ref< PropertyGroup > properties = new PropertyGroup();
	properties->setProperty< PropertyString >(L"Host", itf.addr->getHostName());
	properties->setProperty< PropertyString >(L"Description", OS::getInstance().getComputerName());
#if defined(_WIN32)
	properties->setProperty< PropertyString >(L"OS", L"win32");
#elif defined(__APPLE__)
	properties->setProperty< PropertyString >(L"OS", L"osx");
#else
	properties->setProperty< PropertyString >(L"OS", L"linux");
#endif

	discoveryManager->addService(new net::NetworkService(L"RemoteTools/Server", properties));

	traktor::log::info << L"Discoverable as \"RemoteTools/Server\", host \"" << itf.addr->getHostName() << L"\"" << Endl;
	traktor::log::info << L"Waiting for client(s)..." << Endl;

	std::list< Thread* > clientThreads;
	int32_t iconState = 0;

#if defined(_WIN32)
	while (ui::Application::getInstance()->process())
#else
	for (;;)
#endif
	{
#if defined(_WIN32)
		// Update notification icon if necessary.
		if (clientThreads.empty() && iconState != 0)
		{
			// Set idle icon.
			g_notificationIcon->setImage(ui::Bitmap::load(c_ResourceNotificationIdle, sizeof(c_ResourceNotificationIdle), L"png"));
			iconState = 0;
		}
		else if (!clientThreads.empty() && iconState == 0)
		{
			// Set busy icon.
			g_notificationIcon->setImage(ui::Bitmap::load(c_ResourceNotificationBusy, sizeof(c_ResourceNotificationBusy), L"png"));
			iconState = 1;
		}
#endif

		// Check for events on server socket; if none we cleanup disconnected clients.
		if (serverSocket->select(true, false, false, 1000) <= 0)
		{
			for (std::list< Thread* >::iterator i = clientThreads.begin(); i != clientThreads.end(); )
			{
				if ((*i)->wait(0))
				{
					traktor::log::info << L"Client thread destroyed" << Endl;
					ThreadManager::getInstance().destroy(*i);
					i = clientThreads.erase(i);
				}
				else
					++i;
			}
			continue;
		}

		Ref< net::TcpSocket > clientSocket = serverSocket->accept();
		if (!clientSocket)
			continue;

		traktor::log::info << L"Client connected; spawning thread..." << Endl;

		Thread* clientThread = ThreadManager::getInstance().create(
			makeStaticFunctor(&threadProcessClient, clientSocket),
			L"Client thread"
		);
		if (!clientThread)
		{
			traktor::log::error << L"Unable to create client thread" << Endl;
			continue;
		}

		clientThread->start();
		clientThreads.push_back(clientThread);
	}

#if defined(_WIN32)
	safeDestroy(g_notificationIcon);
	safeDestroy(g_popupMenu);
#endif

	net::Network::finalize();

#if defined(_WIN32)
	ui::Application::getInstance()->finalize();
#endif

	return 0;
}
