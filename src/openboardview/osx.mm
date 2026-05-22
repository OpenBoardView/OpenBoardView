#ifdef __APPLE__

#include "platform.h"
#include <string>
#include <SDL.h>
#import <objc/Object.h>
#import <Cocoa/Cocoa.h>

#ifndef ENABLE_GTK
const filesystem::path show_file_picker(bool filterBoards) {
	std::string filename;
	NSOpenPanel *op = [NSOpenPanel openPanel];

	if ([op runModal] == NSModalResponseOK) {
		NSURL *nsurl = [[op URLs] objectAtIndex:0];
		filename = std::string([[nsurl path] UTF8String]);
	}

	return filename;
}
#endif

#ifndef ENABLE_FONTCONFIG
const std::string get_font_path(const std::string &name) {
	std::string filename;

	NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
	if (!name.empty()) attrs[(id)kCTFontDisplayNameAttribute] = [NSString stringWithUTF8String:name.c_str()]; // Match the font display name
	attrs[(id)kCTFontFormatAttribute] = [NSNumber numberWithUnsignedInt:kCTFontFormatOpenTypeTrueType]; // Match OpenType TrueType fonts (won't work with plain TrueType)

	CTFontDescriptorRef fontDescriptor = CTFontDescriptorCreateWithAttributes((CFDictionaryRef) attrs);
	CFMutableSetRef mandatoryAttributes = CFSetCreateMutable(NULL, 0, &kCFTypeSetCallBacks);
	CFSetAddValue(mandatoryAttributes, kCTFontFormatAttribute); // We want TrueType font exclusively
	if(!name.empty()) CFSetAddValue(mandatoryAttributes, kCTFontDisplayNameAttribute); // Font display name must be exactly what we requested

	CTFontDescriptorRef matchDescriptor = CTFontDescriptorCreateMatchingFontDescriptor(fontDescriptor, mandatoryAttributes); // Search for the font
	if(!matchDescriptor) return filename;

	NSURL *url = (NSURL *) CTFontDescriptorCopyAttribute(matchDescriptor, kCTFontURLAttribute); // Get URL of matched font
	if (url) filename = std::string([[url path] UTF8String]); // Extract path from URL
	return filename;
}
#endif

// Inspired by https://developer.apple.com/library/mac/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/ManagingFIlesandDirectories/ManagingFIlesandDirectories.html
// userdir is ignored for now since common usage puts both config file and history file in ApplicationSupport directory
const std::string get_user_dir(const UserDir userdir) {
	std::string configPath;
	NSFileManager *fm = [NSFileManager defaultManager];
	NSURL *configURL = nil;

	// Find the application support directory in the home directory.
	NSArray *appSupportDir = [fm URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];

	if ([appSupportDir count] > 0) {
		// Append OpenBoardView to the Application Support directory path
		configURL = [[appSupportDir objectAtIndex:0] URLByAppendingPathComponent:@"OpenBoardView/"];

		// If the directory does not exist, this method creates it.
		// This method is only available in OS X v10.7 and iOS 5.0 or later.
		NSError *theError = nil;
		if (![fm createDirectoryAtURL:configURL withIntermediateDirectories:YES attributes:nil error:&theError]) {
			NSLog(@"%@", theError);
		} else if (configURL)
			configPath = std::string([[configURL path] UTF8String]); // Extract path from URL
	}
	if (!configPath.empty())
		return configPath + "/";
	else
		return "./"; // Something went wrong, use current dir.
}

static void newInstance(id self, SEL _cmd) {
	NSURL *executableURL = [[NSRunningApplication currentApplication] executableURL];
        const char *executable = [[executableURL path] UTF8String];

	if (fork() == 0) {
		execl(executable, executable, nullptr);
	};
}

static void openFile(id self, SEL _cmd) {
	auto filename = show_file_picker().string();
	if (!filename.empty()) {
		// Trigger DropFile event which will call BoardView::LoadFile()
		SDL_Event event;
		event.type = SDL_DROPFILE;
		event.drop.file = SDL_strdup(filename.c_str());
		SDL_PushEvent(&event);
	}
}

static NSMenu *applicationDockMenu(id self, SEL _cmd) {
	NSMenu *dockMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
	[dockMenu addItemWithTitle:@"New window" action:@selector(newInstance:) keyEquivalent:@""];
	return dockMenu;
}

void configureMenuBar() {
	// We cannot extend SDLAppDelegate at compile-time since it is not exposed in the API, so add new methods at runtime instead
	auto appDelegate = [NSApp delegate];
	Class delegate_class = object_getClass(appDelegate);

	// Callbacks for menu item clicks
	class_addMethod(delegate_class, @selector(newInstance:), reinterpret_cast<IMP>(newInstance), "v@:");
	class_addMethod(delegate_class, @selector(openFile:), reinterpret_cast<IMP>(openFile), "v@:");

	// Dock menu builder
	class_addMethod(delegate_class, @selector(applicationDockMenu:), reinterpret_cast<IMP>(applicationDockMenu), "v@:");

	NSMenu *fileMenu = [[NSMenu alloc] initWithTitle:@"File"];

	/* Add menu items in File menu */
	[fileMenu addItemWithTitle:@"New window" action:@selector(newInstance:) keyEquivalent:@"n"];
	[fileMenu addItemWithTitle:@"Open file" action:@selector(openFile:) keyEquivalent:@"o"];

	NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:fileMenu];

	// Add File menu to menu bar, just after Application menu and before Window menu (both defined by SDL)
	[[NSApp mainMenu] insertItem:menuItem atIndex:1];
}

#endif
