#import <Cocoa/Cocoa.h>

#include "Core/Misc/TString.h"
#include "Render/OpenGL/Std/OsX/CGLWindow.h"

namespace traktor
{
	namespace render
	{

void* cglwCreateWindow(const std::wstring& title, uint32_t width, uint32_t height, bool fullscreen)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	[NSApplication sharedApplication];
	[NSApp finishLaunching];

	std::string mbs = wstombs(title);
	NSString* titleStr = [[[NSString alloc] initWithCString: mbs.c_str() encoding: NSUTF8StringEncoding] autorelease];

	NSWindow* window = 0;
	
	if (fullscreen)
	{
		CGDisplayCapture(kCGDirectMainDisplay);
		NSInteger windowLevel = CGShieldingWindowLevel();

		NSRect frame = [[NSScreen mainScreen] frame];

		window = [[NSWindow alloc]
			initWithContentRect: frame
			styleMask:NSBorderlessWindowMask
			backing: NSBackingStoreBuffered
			defer: YES
			screen: [NSScreen mainScreen]
		];
		
		[window setLevel: windowLevel];
	}
	else
	{
		window = [[NSWindow alloc]
			initWithContentRect: NSMakeRect(50, 50, width, height)
			styleMask: NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask
			backing: NSBackingStoreBuffered
			defer: YES
		];
	}

	[window setBackgroundColor: [NSColor blackColor]];
	[window setAcceptsMouseMovedEvents: YES];
	[window setTitle: titleStr];
	[window center];
	
	[window makeKeyAndOrderFront: nil];
	//[window makeMainWindow];
	
	cglwUpdateWindow(window);
	
	[pool release];	
	
	return window;
}

void cglwDestroyWindow(void* windowHandle)
{
	NSWindow* window = (NSWindow*)windowHandle;
	[window release];
}

void cglwUpdateWindow(void* windowHandle)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSEvent* event = [NSApp nextEventMatchingMask: NSAnyEventMask untilDate: nil inMode: NSDefaultRunLoopMode dequeue: YES];
	if (event != nil)
	{
		[NSApp sendEvent: event];
		[NSApp updateWindows];
	}
	[pool release];
}

void* cglwGetWindowView(void* windowHandle)
{
	NSWindow* window = (NSWindow*)windowHandle;
	return [window contentView];
}
	
	}
}
