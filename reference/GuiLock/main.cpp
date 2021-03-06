#include <CtrlLib/CtrlLib.h>

using namespace Upp;

struct App : TopWindow {
	Thread work;

	void Work();
	
	ArrayCtrl list;
	
	typedef App CLASSNAME;

	App();	
	~App();
};

void App::Work()
{
	for(;;) {
		Sleep(1);
		GuiLock __;
		if(Thread::IsShutdownThreads())
			break;
		if(list.GetCount() > 1000) {
			if(PromptYesNo("Quit?"))
				Break();
			list.Clear();
		}
		list.Add((int64)Random());
	}
}

App::App()
{
	list.AddColumn("Test");
	Add(list.SizePos());
	work.Run(THISBACK(Work));
}

App::~App()
{
	Thread::ShutdownThreads();
	work.Wait();
}

GUI_APP_MAIN
{
	App app;
	app.Run();
}
