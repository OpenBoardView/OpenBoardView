#ifdef __APPLE__

#include <string>
#import <Cocoa/Cocoa.h>

char *show_file_picker() {
	std::string filename;
	NSOpenPanel *op = [NSOpenPanel openPanel];

	// Filter file types
	[op setCanChooseFiles:YES];
	[op setAllowedFileTypes:@[ @"brd" ]];

	if ([op runModal] == NSModalResponseOK) {
		NSURL *nsurl = [[op URLs] objectAtIndex:0];
		filename = std::string([[nsurl path] UTF8String]);
	}

	size_t len = filename.length();
	char *path = (char *)malloc(len+1);
	memcpy(path, filename.c_str(), len+1);

	return path;
}

const std::string get_font_path(const std::string &name) {
	std::string filename;

	NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
	if (!name.empty())
		attrs[(id)kCTFontDisplayNameAttribute] =
		    [NSString stringWithUTF8String:name.c_str()]; // Match the font display name
	attrs[(id)kCTFontFormatAttribute] = [NSNumber
	    numberWithUnsignedInt:kCTFontFormatOpenTypeTrueType]; // Match OpenType TrueType fonts
	                                                          // (won't work with plain TrueType)

	CTFontDescriptorRef fontDescriptor =
	    CTFontDescriptorCreateWithAttributes((CFDictionaryRef)attrs);
	CFMutableSetRef mandatoryAttributes = CFSetCreateMutable(NULL, 0, &kCFTypeSetCallBacks);
	CFSetAddValue(mandatoryAttributes, kCTFontFormatAttribute); // We want TrueType font exclusively
	if (!name.empty())
		CFSetAddValue(
		    mandatoryAttributes,
		    kCTFontDisplayNameAttribute); // Font display name must be exactly what we requested

	CTFontDescriptorRef matchDescriptor = CTFontDescriptorCreateMatchingFontDescriptor(
	    fontDescriptor, mandatoryAttributes); // Search for the font
	if (!matchDescriptor)
		return filename;

	NSURL *url = (NSURL *)CTFontDescriptorCopyAttribute(
	    matchDescriptor, kCTFontURLAttribute); // Get URL of matched font
	if (url)
		filename = std::string([[url path] UTF8String]); // Extract path from URL
	return filename;
}

#endif // __APPLE__
