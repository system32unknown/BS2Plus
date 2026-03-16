#ifndef TRIGGER_H
#define TRIGGER_H
#include <iostream>

#include "compiler.h"
#include <list>

#define ACTION_SET_VAR_BRUSH 1
#define ACTION_SET_VAR_BRUSHSIZE 2
#define ACTION_SET_VAR_ELEMENT1 3
#define ACTION_SET_VAR_ELEMENT2 4
#define ACTION_SET_VAR_ELEMENT3 5
#define ACTION_SET_VAR_ZOOM 6
#define ACTION_SET_VAR_SPEED 7

#define ACTION_ELEMENT_WEIGHT 1
#define ACTION_ELEMENT_SPRAY 2
#define ACTION_ELEMENT_SLIDE 3
#define ACTION_ELEMENT_VISCOUSITY 4
#define ACTION_ELEMENT_COLOR 5

#define ACTION_SAVE_SCREEN_ALL 1
#define ACTION_SAVE_SCREEN_SAND 2
#define ACTION_SAVE_STAMP 3
#define ACTION_QUICKSAVE_SCREEN_SAND 4
#define ACTION_SAVE_VAR 5
#define ACTION_SAVE_FGLAYER 6
#define ACTION_SAVE_BGLAYER 7
#define ACTION_SAVE_FONT 8
#define ACTION_SAVE_MENUFONT 9
#define ACTION_SAVE_TIMERS 10

#define ACTION_TIMER_FRAMES 1
#define ACTION_TIMER_SEC 2
#define ACTION_TIMER_MSEC 3

#define ACTION_GROUP_ADD 1
#define ACTION_GROUP_SET_ICON 2
#define ACTION_GROUP_CLEAR 3
#define ACTION_GROUP_CLEARALL 4

#define ACTION_LIST_GROUP 1
#define ACTION_LIST_GROUPS 2
#define ACTION_LIST_ELEMENTS 3
#define ACTION_LIST_VARS 4
#define ACTION_LIST_INTERACTIONS 5
#define ACTION_LIST_DIETOS 6
#define ACTION_LIST_ELEMENTGROUPS 7
#define ACTION_LIST_TRIGGERS 8
#define ACTION_LIST_ACTIONS 9
#define ACTION_LIST_TIMERS 10
#define ACTION_LIST_TRIGGEREXECS 11

#define ACTION_FILE_OPEN 1
#define ACTION_FILE_CLOSE 2
#define ACTION_FILE_DELETE 3

#define ACTION_MENU_CLEAR 1
#define ACTION_MENU_REFRESH 2

#define ACTION_BUTTON_TIPTYPE_TEXT 0
#define ACTION_BUTTON_TIPTYPE_ELEMENT 1
#define ACTION_BUTTON_TIPTYPE_GROUP 2

#define STATUS_CLEAR 1
#define STATUS_ADDTEXT 2
#define STATUS_ADDNUMBER 3
#define STATUS_ADDELEMENT 4
#define STATUS_ADDGROUP 5
#define STATUS_MOUSEOVER 6

#define MESSAGE_CLEAR 1
#define MESSAGE_ADDTEXT 2
#define MESSAGE_ADDNUMBER 3
#define MESSAGE_ADDELEMENT 4
#define MESSAGE_ADDGROUP 5
#define MESSAGE_SEND 6
#define MESSAGE_SAVE 7
#define MESSAGE_EXEC 8
#define MESSAGE_SENDTEXT 9
#define MESSAGE_MESSAGEBOX 10
#define MESSAGE_BS1 11
#define MESSAGE_ADDLINE 12
#define MESSAGE_SYSTEM 13
#define MESSAGE_ADDELEMENTTEXT 14
#define MESSAGE_SAVESTRING 15
#define MESSAGE_ADDSTRING 16

#define WRITE_TEXT 1
#define WRITE_NUMBER 2
#define WRITE_ELEMENT 3
#define WRITE_GROUP 4
#define WRITE_MESSAGE 5
#define WRITE_STRING 6

#define REMOTE_SET 1
#define REMOTE_EXEC 2

#define FOR_TO 1
#define FOR_DOWNTO 2

#define MAX_STRINGS 10000
extern char* strings[MAX_STRINGS];

#include "sand.h"
#include "menu.h"
#include "vars.h"
#include "loadsave.h"
#include "elements.h"

char* messageReplace(char* text);

struct Action {
	virtual void exec();
	virtual ~Action();
	virtual char* toString();
	int owner;
};

bool threadrun();
void setthreadrun(bool b);
void frametimer();
void msectimer();

void addAction(char* name, Action* trigger, int owner);

extern Var* debugframe;
extern long execcounter;
extern bool setreturn;

void deleteparams(Varint** v);

inline int* calcparams(Varint** v) {
	if (v == NULL) return NULL;
	int* r = new int[11];
	if (v[0]) r[0] = v[0]->val();
	if (v[1]) r[1] = v[1]->val();
	if (v[2]) r[2] = v[2]->val(); else return r;
	if (v[3]) r[3] = v[3]->val();
	if (v[4]) r[4] = v[4]->val();
	if (v[5]) r[5] = v[5]->val();
	if (v[6]) r[6] = v[6]->val(); else return r;
	if (v[7]) r[7] = v[7]->val();
	if (v[8]) r[8] = v[8]->val();
	if (v[9]) r[9] = v[9]->val();
	return r;
}

struct Trigger {
	char* name;
	int actioncount;
	int execcount;
	std::list<Action*> actions;
	int exec();
	inline void exec(int x, int y, int b, int c) {
		static Var* vx = (Var*)setVar("X", 0);
		static Var* vy = (Var*)setVar("Y", 0);
		static Var* vb = (Var*)setVar("BUTTON", 0);
		static Var* vc = (Var*)setVar("CLICKED", 0);
		vx->value = x;
		vy->value = y;
		vb->value = b;
		vc->value = c;
		this->exec();
	};
	long deleted;
	inline bool operator<(const Trigger& t) {
		return true;
	}
};

Trigger* findTrigger(char* name, int owner);

struct ActionReturn : Action {
	Varint* var;
	void exec();
	char* toString();
	~ActionReturn();
};

struct ActionExit : Action {
	void exec();
	char* toString();
};

struct ActionRestart : Action {
	char* parameter;
	void exec();
	char* toString();
};

struct ActionSetVar : Action {
	char* var;
	Varint* value;
	int* t;
	void exec();
	~ActionSetVar();
	char* toString();
};

struct ActionInc : Action {
	char* var;
	Varint* value;
	int* t;
	void exec();
	~ActionInc();
	char* toString();
};

struct ActionCount : Action {
	char* var;
	Varint* element;
	Varint* x;
	Varint* y;
	Varint* w;
	Varint* h;
	void exec();
	~ActionCount();
	char* toString();
};

struct ActionClosest : Action {
	Varint* x;
	Varint* y;
	Varint* e;
	Varint* d;
	unsigned int lx[10], ly[10], li;
	void exec();
	~ActionClosest();
	char* toString();
};

struct ActionGetVar : Action {
	char* value;
	void exec();
	~ActionGetVar();
	char* toString();
};

struct ActionGetFile : Action {
	char* filename;
	void exec();
	~ActionGetFile();
	char* toString();
};

struct ActionResize : Action {
	Varint* w, * h;
	void exec();
	~ActionResize();
	char* toString();
};

struct ActionScroll : Action {
	Varint* x, * y;
	void exec();
	~ActionScroll();
	char* toString();
};

struct ActionWhile : Action {
	Varint* value;
	Varint** params;
	Trigger* trigger;
	inline void exec() {
		while (value->val()) {
			if (params) addparams(calcparams(params));
			trigger->exec();
			if (params) removeparams();
		}
	}
	~ActionWhile();
	char* toString();
};

struct ActionFor : Action {
	int function;
	char* value;
	Varint* fromvalue;
	Varint* tovalue;
	Varint* step;
	Varint** params;
	Trigger* trigger;
	void exec();
	~ActionFor();
	char* toString();
};

struct ActionForEach : Action {
	Varint* element;
	Varint** params;
	Trigger* trigger;
	void exec();
	~ActionForEach();
	char* toString();
};

struct ActionSystem : Action {
	char* cmd;
	void exec();
	~ActionSystem();
	char* toString();
};

struct ActionIf : Action {
	Varint* value;
	Varint** params;
	Trigger* trigger;
	Trigger* elsetrigger;
	inline void exec() {
		static Var* messageid = (Var*)setVar("MESSAGEID", 0);
		messageid->value = owner;
		if (value->val()) {
			if (params) {
				addparams(calcparams(params));
				trigger->exec();
				removeparams();
			} else
				trigger->exec();
		} else if (elsetrigger)
			elsetrigger->exec();
	}
	~ActionIf();
	char* toString();
};

struct ActionExec : Action {
	Varint** params;
	Trigger* trigger;
	inline void exec() {
		if (!trigger) return;
		if (params) addparams(calcparams(params));
		trigger->exec();
		if (params) removeparams();
	};
	~ActionExec();
	char* toString();
};

struct ActionRemoveTrigger : Action {
	char* trigger;
	void exec();
	~ActionRemoveTrigger();
	char* toString();
};

struct ActionElement : Action {
	char* elementname;
	int attribute;
	Varint* value1;
	Varint* value2;
	Varint* value3;
	void exec();
	~ActionElement();
	char* toString();
};

struct ActionElementBS1 : Action {
	char* elementname;
	char* group;
	char* dieto;
	Varint* dierate;
	Pic* icon;
	Varint* rvalue;
	Varint* gvalue;
	Varint* bvalue;
	Varint* weight;
	Varint* spay;
	Varint* slide;
	Varint* viscousity;
	Varint* menuorder;
	void exec();
	~ActionElementBS1();
	char* toString();
};

struct ActionElementDie : Action {
	char* elementname;
	char* dieto;
	Varint* rate;
	void exec();
	~ActionElementDie();
	char* toString();
};

struct ActionInteraction : Action {
	std::list<char*> elements1;
	std::list<char*> elements2;
	std::list<char*> toselfs;
	std::list<char*> toothers;
	char* except;
	std::list<Varint*> rates;
	Varint* at;
	std::list<Trigger*> triggers;
	void exec();
	~ActionInteraction();
	char* toString();
};

struct ActionRemoveInteraction : Action {
	char* element;
	Varint* index;
	void exec();
	~ActionRemoveInteraction();
	char* toString();
};

struct ActionClearDie : Action {
	char* element;
	void exec();
	~ActionClearDie();
	char* toString();
};

struct ActionClearElements : Action {
	void exec();
	char* toString();
};

struct ActionSave : Action {
	char* filename;
	int screenid;
	Varint* id;
	void exec();
	~ActionSave();
	char* toString();
};

struct ActionFile : Action {
	int function;
	char* filename;
	char* param;
	void exec();
	~ActionFile();
	char* toString();
};

struct ActionLoad : Action {
	char* filename;
	int screenid;
	Varint* id;
	void exec();
	~ActionLoad();
	char* toString();
};

struct ActionButton : Action {
	Varint** params;
	void* button;
	int bar;
	int tiptype;
	Varint* r, * g, * b;
	void exec();
	char* toString();
	~ActionButton();
};

struct ActionTrigger : Action {
	char* triggername;
	Action* action;
	inline void exec() { if (triggername) addAction(triggername, action, owner); };
	~ActionTrigger();
	char* toString();
};

struct ActionStatus : Action {
	char* text;
	Varint* v;
	int function;
	void exec();
	~ActionStatus();
	char* toString();
};

struct ActionRemote : Action {
	int type;
	char* text;
	Varint* val;
	Varint* mid;
	void exec();
	~ActionRemote();
	char* toString();
};

struct ActionConnect : Action {
	char* host;
	Varint* port;
	char* var;
	void exec();
	~ActionConnect();
	char* toString();
};

struct ActionWrite : Action {
	char* text;
	int type;
	int align;
	Varint* v;
	Varint* element, * x, * y, * size;
	void exec();
	~ActionWrite();
	char* toString();
};

struct ActionDraw : Action {
	Varint* element;
	int brush;
	Varint* drawx, * drawy, * dx, * dy, * a1, * a2;
	void exec();
	~ActionDraw();
	char* toString();
};

struct ActionDrawPoints : Action {
	Varint* element;
	Varint* xoffset, * yoffset;
	std::list<int> x;
	std::list<int> y;
	void exec();
	~ActionDrawPoints();
	char* toString();
};

struct ActionDrawObject : Action {
	Varint* xoffset, * yoffset;
	Varint* elements[256];
	Varint* sizex, * sizey;
	std::list<char*> data;
	void exec();
	~ActionDrawObject();
	char* toString();
};

struct ActionTimer : Action {
	int type;
	Varint* value;
	Trigger* trigger;
	Varint** params;
	void exec();
	~ActionTimer();
	char* toString();
};

struct ActionClearTimer : Action {
	Trigger* trigger;
	int removeall;
	void exec();
	char* toString();
};

struct ActionGroup : Action {
	char* groupname;
	Varint* order;
	int function;
	char* element;
	Pic* icon;
	void exec();
	~ActionGroup();
	char* toString();
};

struct ActionKey : Action {
	Varint* v;
	char* keyname;
	void exec();
	~ActionKey();
	char* toString();
};

struct ActionList : Action {
	int function;
	char* element;
	void exec();
	~ActionList();
	char* toString();
};

struct ActionMessage : Action {
	int function;
	char* message;
	Varint* varint;
	void exec();
	~ActionMessage();
	char* toString();
};

struct ActionInclude : Action {
	char* filename;
	char* param;
	void exec();
	~ActionInclude();
	char* toString();
};

struct ActionWind : Action {
	Varint* x;
	Varint* y;
	Varint* d;
	void exec();
	char* toString();
};

struct ActionNoBias : Action {
	Varint* e;
	void exec();
	char* toString();
	~ActionNoBias();
};

struct ActionMenu : Action {
	int bar;
	int action;
	void exec();
	char* toString();
	~ActionMenu();
};

struct ActionSubMenu : Action {
	void exec();
	Varint* x;
	Varint* y;
	int stay;
	int align;
	int function;
	char* toString();
	~ActionSubMenu();
};

struct Timer {
	Uint32 value;
	Trigger* trigger;
	int* params;
};

#endif