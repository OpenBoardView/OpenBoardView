#ifdef __APPLE__

#include "platform.h"
#include <string>
#import <Cocoa/Cocoa.h>

#ifndef ENABLE_GTK
const std::string show_file_picker() {
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

#endif
