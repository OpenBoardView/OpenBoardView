#include "platform.h"
#include <string>
#import <Cocoa/Cocoa.h>

std::string show_file_picker() {
	std::string filename;
	NSOpenPanel *op = [NSOpenPanel openPanel];

	// Filter file types
	[op setCanChooseFiles:YES];
	[op setAllowedFileTypes:@[ @"brd" ]];

	if ([op runModal] == NSModalResponseOK) {
		NSURL *nsurl = [[op URLs] objectAtIndex:0];
		filename = std::string([[nsurl path] UTF8String]);
	}

	return filename;
}
