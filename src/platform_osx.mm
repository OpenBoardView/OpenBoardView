#ifdef __APPLE__

#include <string>
#import <Cocoa/Cocoa.h>

char *show_file_picker() {
	std::string filename;
	NSOpenPanel *op = [NSOpenPanel openPanel];

	// Filter file types
	[op setCanChooseFiles:YES];
	[op setAllowedFileTypes:@[ @"brd", @"bdv" ]];

	if ([op runModal] == NSModalResponseOK) {
		NSURL *nsurl = [[op URLs] objectAtIndex:0];
		filename = std::string([[nsurl path] UTF8String]);
	}

	size_t len = filename.length();
	char *path = (char *)malloc(len+1);
	memcpy(path, filename.c_str(), len+1);

	return path;
}

#endif // __APPLE__
