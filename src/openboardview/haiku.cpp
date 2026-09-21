#ifdef __HAIKU__

#include "platform.h"

#include <FilePanel.h>
#include <FindDirectory.h>
#include <Looper.h>
#include <Path.h>
#include <Messenger.h>
#include <Window.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <stdio.h>
#include <compat/sys/stat.h>
#include <string.h>

extern "C" char *open_file_dialog(void);

class FileFilter: public BRefFilter
{
	public:
	bool Filter(const entry_ref *ref,
		BNode *node,
		struct stat_beos *stat,
		const char *mimeType)
	{
		int len = strlen(ref->name);
		if (S_ISDIR(stat->st_mode))
			return true;
		if(len > 2 && !strcasecmp(ref->name + len - 3, ".fz"))
			return true;
		if (len > 3) {
			const char *ext = ref->name + len - 4;
			if (!strcasecmp(ext, ".asc") ||
				!strcasecmp(ext, ".bom") ||
				
				!strcasecmp(ext, ".cae") ||
				!strcasecmp(ext, ".adf") ||
				!strcasecmp(ext, ".cad") ||
				!strcasecmp(ext, ".cae") ||
				!strcasecmp(ext, ".cst") ||
				!strcasecmp(ext, ".brd") ||
				!strcasecmp(ext, ".bdv") ||
				!strcasecmp(ext, ".bvr") ||
				!strcasecmp(ext, ".xzz") ||
				!strcasecmp(ext, ".pcb"))
				return true;
		}
		if (len > 7 && !strcasecmp(ref->name + len - 8, ".pcbdoc"))
			return true;
		return false;
	}
};

class FileHandler: public BLooper
{
	public:

	FileHandler()
		: BLooper("filepanel listener")
	{
		lock = create_sem(0, "filepanel sem");
		path = NULL;
		Run();
	}

	~FileHandler()
	{
		delete_sem(lock);
		if (path != NULL)
			free(path);
	}
	void MessageReceived(BMessage* message)
	{
		switch(message->what)
		{
			case B_REFS_RECEIVED:
			{
				entry_ref ref;

				message->FindRef("refs", 0, &ref);
				BEntry entry(&ref, true);
				BPath xpath;
				entry.GetPath(&xpath);
				path = strdup(xpath.Path());
				break;
			}
			case B_CANCEL:
				break;
			default:
				BHandler::MessageReceived(message);
				return;
		}
		release_sem(lock);
	}

	void Wait() {
		acquire_sem(lock);
	}

	char* path;

	private:
		sem_id lock;
};

static char ret[PATH_MAX];

char *open_file_dialog(void)
{
	FileHandler *handler = new FileHandler();
	FileFilter filter;
	BMessenger messenger(handler);
	BFilePanel panel(B_OPEN_PANEL,
		&messenger,
		NULL,
		B_FILE_NODE,
		false,
		NULL,
		&filter,
		true);
	panel.Show();
	handler->Wait();
	if (handler->path == NULL) {
		messenger.SendMessage(B_QUIT_REQUESTED);
		return NULL;
	}
	snprintf(ret, sizeof(ret), "%s", handler->path);
	ret[sizeof(ret) - 1] = 0;
	messenger.SendMessage(B_QUIT_REQUESTED);
	return ret;
}

const filesystem::path show_file_picker(bool filterBoards){
    char *path = open_file_dialog();
    if (path == NULL) return std::string();
    return std::string(path);
}

#endif