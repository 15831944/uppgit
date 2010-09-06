#ifndef _TreeTest_TreeTest_h
#define _TreeTest_TreeTest_h

#include <CtrlLib/CtrlLib.h>

using namespace Upp;

#define LAYOUTFILE <TreeTest/TreeTest.lay>
#include <CtrlCore/lay.h>

#include <Tree/Tree.h>

class Element
{
public:
	typedef Element CLASSNAME;
	String name;
	Value value;
};

///

class TreeTest : public WithLayout<TopWindow> {
public:
	typedef TreeTest CLASSNAME;
	TreeTest();
	
	Node<One<Element> > root;
	Node<Element> roota;
};

#endif
