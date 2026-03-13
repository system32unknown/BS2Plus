#include "trigger.h"
#include "config.h"
#include "menu.h"
#include <stack>
#include <queue>
#include <vector>
#include "win.h"
#include "keys.h"
#include "network.h"
#include <math.h>

std::list<Trigger *> triggers;

struct mycomparison
{
	bool operator()(const Timer *lhs, const Timer *rhs)
	{
		return lhs->value > rhs->value;
	}
};

std::priority_queue<Timer *, std::vector<Timer *>, mycomparison> frametriggers;
std::list<Timer *> msecriggers;

std::stack<Timer *> timerStack;

char messagestring[32000];

Uint32 thisframe = 0;

bool timercleared;
bool threadrunning = true;

bool threadrun() { return threadrunning; }
void setthreadrun(bool b) { threadrunning = b; }

long execcounter = 0;

bool setreturn;
int globalreturn;

std::ofstream savefile;

Var *debugframe;
int rec = -1;
char *strings[MAX_STRINGS];

char *messageReplace(char *text)
{
	if (!strcmp(text, "MESSAGE"))
	{
		return messagestring;
	}
	else
		return text;
}

void deleteparams(Varint **v)
{
	if (v == NULL)
		return;
	if (v[0])
		delete (v[0]);
	if (v[1])
		delete (v[1]);
	if (v[2])
		delete (v[2]);
	if (v[3])
		delete (v[3]);
	if (v[4])
		delete (v[4]);
	if (v[5])
		delete (v[5]);
	if (v[6])
		delete (v[6]);
	if (v[7])
		delete (v[7]);
	if (v[8])
		delete (v[8]);
	if (v[9])
		delete (v[9]);
	delete (v);
}

void Action::exec()
{
}

Action::~Action()
{
}

char *Action::toString()
{
	char *tmp = new char[1024];
	sprintf(tmp, "NO ACTION");
	return tmp;
}

Trigger *findTrigger(char *triggername, int owner)
{
	if (triggername == 0)
		return 0;
	char *name = messageReplace(triggername);
	if (name[0] == '<')
	{
		name[strlen(name) - 1] = 0;
		Token tokens(name + 1);
		Action *action = parseaction(&tokens, owner);
		if (action == NULL)
			return NULL;
		else
		{
			Trigger *t = new Trigger;
			t->execcount = 0;
			t->deleted = 0;
			t->name = "NO NAME";
			t->actions.push_back(action);
			t->actioncount = t->actions.size();
			return t;
		}
	}
	std::list<Trigger *>::iterator it = triggers.begin();
	while (it != triggers.end())
	{
		if (!strcmp((*it)->name, name))
			return *it;
		it++;
	}
	Trigger *trigger = new Trigger;
	trigger->deleted = 0;
	trigger->execcount = 0;
	trigger->name = new char[strlen(name) + 1];
	trigger->actioncount = 0;
	strcpy(trigger->name, name);
	triggers.push_back(trigger);
	return trigger;
}

void addAction(char *trigger, Action *action, int owner)
{
	Trigger *t = findTrigger(trigger, owner);
	t->actions.push_back(action);
	t->actioncount = t->actions.size();
}

int Trigger::exec()
{
	execcount++;
	if (!actioncount)
		return 0;
	static Var *debugtrigger = (Var *)setVar("DEBUGTRIGGER", 0);
	static Var *debugactions = (Var *)setVar("DEBUGACTION", 0);
	setreturn = false;
	rec++;
	if (debugtrigger->value)
	{
		static char tmp[512];
		if (name)
			sprintf(tmp, "executing trigger: %s actions: %i", name, actions.size());
		else
			sprintf(tmp, "executing trigger: noname, actions: %i", actions.size());
		if (rec < 0)
			rec = 0;
		for (int i = 0; i < rec; i++)
			std::cout << "  ";
		std::cout << tmp << std::endl;
	}
	if (actioncount == 1)
	{
		if (debugactions->value)
		{
			char *t = (*actions.begin())->toString();
			char tmp[512];
			sprintf(tmp, "executing action: messagid: %i action: ", (*actions.begin())->owner);
			static int i;
			for (i = 0; i <= rec; i++)
				std::cout << "  ";
			std::cout << tmp << t << std::endl;
			delete (t);
		}
		(*actions.begin())->exec();
		rec--;

		if (setreturn)
			return globalreturn;
		else
			return 0;
	}
	long thisexecounter = ++execcounter;
	std::list<Action *>::iterator it = actions.begin();
	while ((it != actions.end()) && (thisexecounter > deleted))
	{
		if (debugactions->value)
		{
			char *t = (*it)->toString();
			char tmp[512];
			sprintf(tmp, "executing action: messagid: %i action: ", (*it)->owner);
			static int i;
			for (i = 0; i <= rec; i++)
				std::cout << "  ";
			std::cout << tmp << t << std::endl;
			delete (t);
		}
		(*it)->exec();
		if (setreturn)
			return globalreturn;
		if (actions.size() == 0)
		{
			rec--;
			return 0;
		}
		it++;
	}
	rec--;
	return 0;
}

void ActionReturn::exec()
{
	globalreturn = var->val();
	setreturn = true;
}

char *ActionReturn::toString()
{
	char *tmp = new char[1024 + strlen(var->text)];
	sprintf(tmp, "RETURN %s", var->text);
	return tmp;
}

ActionReturn::~ActionReturn()
{
	delete (var);
}

void ActionExit::exec()
{
	if (threadrunning)
	{
		threadrunning = false;
		while (threadrunning == false);
	}
	exit(0);
}

char *ActionExit::toString()
{
	char *tmp = new char[1024];
	sprintf(tmp, "EXIT");
	return tmp;
}

void ActionRestart::exec()
{
#ifdef COMPILER_SYSTEM
	if (parameter)
	{
		ossystem("BS2Plus.exe", parameter);
	}
	else
		ossystem("BS2Plus.exe", "");
#endif
	if (threadrunning)
	{
		threadrunning = false;
		while (threadrunning == false);
	}
	exit(0);
}

char *ActionRestart::toString()
{
	char *tmp = new char[1024 + strlen(parameter)];
	if (parameter)
		sprintf(tmp, "RESTART \"%s\"", parameter);
	else
		sprintf(tmp, "RESTART");
	return tmp;
}

void ActionSetVar::exec()
{
	if (!t)
	{
		if (!strchr(var, '['))
		{
			static Var *v;
			v = (Var *)setVar(var, 0, false);
			if (v)
				t = &(v->value);
			else
				t = (int *)1;
		}
		else
			t = (int *)1;
	}
	static Var *messageid = (Var *)setVar("MESSAGEID", 0);
	messageid->value = owner;
	if (t == (int *)1)
		setVar(var, value->val());
	else
		*t = value->val();
}

char *ActionSetVar::toString()
{
	char *tmp = new char[1024 + strlen(var) + strlen(value->text)];
	sprintf(tmp, "SET \"%s\" %s", var, value->text);
	return tmp;
}

ActionSetVar::~ActionSetVar()
{
	delete (var);
	delete (value);
}

void ActionInc::exec()
{
	if (!t)
	{
		if (!strchr(var, '['))
		{
			static Var *v;
			v = (Var *)setVar(var, 0, false);
			if (v)
				t = &(v->value);
			else
				t = (int *)1;
		}
		else
			t = (int *)1;
	}
	static Var *messageid = (Var *)setVar("MESSAGEID", 0);
	messageid->value = owner;
	if (t == (int *)1)
		setVar(var, value->val() + 1);
	else
		(*t)++;
}

char *ActionInc::toString()
{
	char *tmp = new char[1024 + strlen(var)];
	sprintf(tmp, "INC \"%s\"", var);
	return tmp;
}

ActionInc::~ActionInc()
{
	delete (var);
	delete (value);
}

void ActionCount::exec()
{
	SDL_Surface *screen = getRealSandSurface();
	Uint16 *t;
	int e = element->val();
	int i = 0;
	if (x)
	{
		int starty = y->val();
		int startx = x->val();
		int endy = starty + h->val();
		int endx = startx + w->val();
		for (int y = starty; y < endy; y++)
		{
			t = ((Uint16 *)screen->pixels + (y)*screen->pitch / 2);
			for (int x = startx; x < endx; x++)
				if (*(t + x) == e)
					i++;
		}
	}
	else
	{
		Uint16 *end = (Uint16 *)screen->pixels + screen->pitch / 2 * (screen->h - 2) + (screen->w - 1) - 1;
		t = (Uint16 *)screen->pixels - 1 + screen->pitch / 2;
		while (end != ++t)
			if (*t == e)
				i++;
	}
	setVar(var, i);
}

char *ActionCount::toString()
{
	char *tmp = new char[1024 + strlen(var) + strlen(element->text)];
	sprintf(tmp, "COUNT \"%s\" %s", var, element->text);
	return tmp;
}

ActionCount::~ActionCount()
{
	delete (var);
	delete (element);
	delete (x);
	delete (y);
	delete (w);
	delete (h);
}

void ActionClosest::exec()
{
	static Var *varx = (Var *)setVar("CLOSESTX", 0);
	static Var *vary = (Var *)setVar("CLOSESTY", 0);
	static Var *vard = (Var *)setVar("CLOSESTD", 0);
	SDL_Surface *screen = getRealSandSurface();
	int ne = e->val();
	if (!used[ne])
	{
		varx->value = -1;
		vary->value = -1;
		vard->value = (int)sqrt((float)1000000000);
		return;
	}
	int w = screen->w - 2;
	int h = screen->h - 2;
	int nx = x->val();
	int ny = y->val();
	int nd = d->val();
	int d = 1000000000;
	for (int i = 0; i < 5; i++)
		if ((lx[i] < (unsigned int)w) && (ly[i] < (unsigned int)h) && (*(((Uint16 *)screen->pixels + (ly[i]) * screen->pitch / 2) + lx[i]) == ne))
		{
			int tmp = (int)sqrt((nx - lx[i]) * (nx - lx[i]) + (ny - ly[i]) * (ny - ly[i])) + 2;
			if (tmp < nd)
				nd = tmp;
		}

	int lastx = -1;
	int lasty = -1;
	Uint16 *t;
	int d2 = 32;
	if (d2 > nd)
		d2 = nd;
	while ((d2 <= nd) && (d2 <= (int)(sqrt((float)d) + d2)) && (d != 999999999))
	{
		int startx = nx - d2;
		int endx = nx + d2;
		int starty = ny - d2;
		int endy = ny + d2;
		if ((startx < 1) && (endx > w) && (starty < 1) && (endy > h))
			d = 999999999;
		if (startx < 1)
			startx = 1;
		if (endx > w)
			endx = w;
		if (starty < 1)
			starty = 1;
		if (endy > h)
			endy = h;
		for (int y = starty; y <= endy; y++)
		{
			t = ((Uint16 *)screen->pixels + (y)*screen->pitch / 2);
			for (int x = startx; x <= endx; x++)
				if (*(t + x) == ne)
				{
					if (((x - nx) * (x - nx) + (y - ny) * (y - ny)) < d)
					{
						d = ((x - nx) * (x - nx) + (y - ny) * (y - ny));
						lastx = x;
						lasty = y;
					}
				}
		}
		if (d2 == nd)
			break;
		d2 *= 2;
		if (d2 > nd)
			d2 = nd;
	}
	lx[li % 5] = varx->value = lastx;
	ly[li % 5] = vary->value = lasty;
	li++;
	vard->value = (int)sqrt((float)d);
}

char *ActionClosest::toString()
{
	char *tmp = new char[1024 + strlen(e->text) + strlen(x->text) + strlen(y->text)];
	sprintf(tmp, "CLOSEST %s %s %s", e->text, x->text, y->text);
	return tmp;
}

ActionClosest::~ActionClosest()
{
	delete (x);
	delete (y);
	delete (e);
	delete (d);
}

void ActionGetVar::exec()
{
	static Var *messageid = (Var *)setVar("MESSAGEID", 0);
	messageid->value = owner;
	char out[256];
	int i;
	int t = getVar(value, &i);
	if (t == 7)
		sprintf(out, "[%s] %s", value, (char *)i);
	else if (t)
		sprintf(out, "[%s] %i", value, i);
	else
		sprintf(out, "[%s] 0", value);
	print(out, owner);
}

char *ActionGetVar::toString()
{
	char *tmp = new char[1024 + strlen(value)];
	sprintf(tmp, "GET \"%s\"", value);
	return tmp;
}

ActionGetVar::~ActionGetVar()
{
	delete (value);
}

void ActionGetFile::exec()
{
	long size = 0;
	char *data = NULL;
	std::ifstream file;
	file.open(checkfilename(filename), std::ios::in | std::ios::ate | std::ios::binary);
	if (!file)
	{
		char tmp[10000];
		sprintf(tmp, "Error opening file \"%s\"", filename);
		error(tmp, owner);
		return;
	}
	size = file.tellg();
	file.seekg(0, std::ios::beg);
	data = new char[size + 2];
	file.read(data, size);
	data[size] = 0;
	file.close();
	char out[256];
	sprintf(out, "{%s} %i", filename, (int)size);
	print(out, owner);
	print(data, owner, size);
	delete (data);
}

char *ActionGetFile::toString()
{
	char *tmp = new char[1024 + strlen(filename)];
	sprintf(tmp, "GETFILE \"%s\"", filename);
	return tmp;
}

ActionGetFile::~ActionGetFile()
{
	delete (filename);
}

void ActionResize::exec()
{
	resize(w->val(), h->val());
}

char *ActionResize::toString()
{
	char *tmp = new char[1024 + strlen(w->text) + strlen(h->text)];
	sprintf(tmp, "GETFILE %s %s", w->text, h->text);
	return tmp;
}

ActionResize::~ActionResize()
{
	delete (w);
	delete (h);
}

void ActionScroll::exec()
{
	scrollto(x->val(), y->val());
}

char *ActionScroll::toString()
{
	char *tmp = new char[1024 + strlen(x->text) + strlen(y->text)];
	sprintf(tmp, "SCROLL %s %s", x->text, y->text);
	return tmp;
}

ActionScroll::~ActionScroll()
{
	delete (x);
	delete (y);
}

char *ActionWhile::toString()
{
	char *tmp = new char[1024 + strlen(value->text) + strlen(trigger->name)];
	sprintf(tmp, "WHILE %s \"%s\"", value->text, trigger->name);
	return tmp;
}

ActionWhile::~ActionWhile()
{
	delete (value);
	deleteparams(params);
}

void ActionFor::exec()
{
	Var *v = (Var *)setVar(value, 0);
	int end = tovalue->val();
	int s = step->val();
	if (function == FOR_TO)
		for (v->value = fromvalue->val(); v->value <= end; (v->value) += s)
		{
			if (params)
				addparams(calcparams(params));
			trigger->exec();
			if (params)
				removeparams();
		}
	else
		for (v->value = fromvalue->val(); v->value >= end; (v->value) -= s)
		{
			if (params)
				addparams(calcparams(params));
			trigger->exec();
			if (params)
				removeparams();
		}
}

char *ActionFor::toString()
{

	char *tmp = new char[1024 + strlen(value) + strlen(fromvalue->text) + strlen(tovalue->text) + strlen(trigger->name)];
	if (function == FOR_TO)
		sprintf(tmp, "FOR \"%s\" FROM %s TO %s DO \"%s\"", value, fromvalue->text, tovalue->text, trigger->name);
	else
		sprintf(tmp, "FOR \"%s\" FROM %s DOWNTO %s DO \"%s\"", value, fromvalue->text, tovalue->text, trigger->name);
	return tmp;
}

ActionFor::~ActionFor()
{
	delete (value);
	delete (fromvalue);
	delete (tovalue);
	deleteparams(params);
}

void ActionForEach::exec()
{
	static Var *varx = (Var *)setVar("FOREACHX", 0);
	static Var *vary = (Var *)setVar("FOREACHY", 0);
	SDL_Surface *screen = getRealSandSurface();
	int w = screen->w - 2;
	int h = screen->h - 2;
	int e = element->val();
	Uint16 *t;
	for (int y = 1; y <= h; y++)
	{
		t = ((Uint16 *)screen->pixels + (y)*screen->pitch / 2);
		for (int x = 1; x <= w; x++)
			if (*(t + x) == e)
			{
				varx->value = x;
				vary->value = y;
				if (params)
					addparams(calcparams(params));
				trigger->exec();
				if (params)
					removeparams();
			}
	}
}

char *ActionForEach::toString()
{
	char *tmp = new char[1024 + strlen(element->text) + strlen(trigger->name)];
	sprintf(tmp, "FOR EACH %s DO \"%s\"", element->text, trigger->name);
	return tmp;
}

ActionForEach::~ActionForEach()
{
	deleteparams(params);
}

void ActionSystem::exec()
{
#ifdef COMPILER_WINDOWS
	if (yesnobox(cmd, "Run Command?"))
	{
		std::ofstream batchfile2;
		batchfile2.open(checkfilename("tmp.cmd"), std::ios::out);
		batchfile2.write(cmd, strlen(cmd));
		batchfile2.close();
		ossystem("@tmp.cmd", 0, true);
		remove("tmp.cmd");
	}
#endif
}

char *ActionSystem::toString()
{

	char *tmp = new char[1024 + strlen(cmd)];
	sprintf(tmp, "SYSTEM \"%s\"", cmd);
	return tmp;
}

ActionSystem::~ActionSystem()
{
	delete (cmd);
	return;
}

char *ActionIf::toString()
{
	char *tmp;
	if (elsetrigger)
	{
		tmp = new char[1024 + strlen(value->text) + strlen(trigger->name) + strlen(elsetrigger->name)];
		sprintf(tmp, "IF %s \"%s\" ELSE \"%s\"", value->text, trigger->name, elsetrigger->name);
	}
	else
	{
		tmp = new char[1024 + strlen(value->text) + strlen(trigger->name)];
		sprintf(tmp, "IF %s \"%s\"", value->text, trigger->name);
	}
	return tmp;
}

ActionIf::~ActionIf()
{
	delete (value);
	deleteparams(params);
}

void ActionRemoveTrigger::exec()
{
	Trigger *t = findTrigger(trigger, owner);
	t->actions.clear();
	t->deleted = execcounter;
	t->actioncount = 0;
}

char *ActionRemoveTrigger::toString()
{
	char *tmp = new char[1024 + strlen(trigger)];
	sprintf(tmp, "REMOVETRIGGER \"%s\"", trigger);
	return tmp;
}

ActionRemoveTrigger::~ActionRemoveTrigger()
{
	delete (trigger);
}

char *ActionExec::toString()
{
	char *tmp = new char[1024 + strlen(trigger->name)];
	sprintf(tmp, "EXEC \"%s\"", trigger->name);
	return tmp;
}

ActionExec::~ActionExec()
{
	deleteparams(params);
}

void ActionElement::exec()
{
	Uint16 e = findElement(elementname, true);
	switch (attribute)
	{
	case ACTION_ELEMENT_WEIGHT:
		setElementWeight(e, value1->val());
		break;
	case ACTION_ELEMENT_SPRAY:
		setElementSpray(e, value1->val());
		break;
	case ACTION_ELEMENT_SLIDE:
		setElementSlide(e, value1->val());
		break;
	case ACTION_ELEMENT_VISCOUSITY:
		setElementViscousity(e, value1->val());
		break;
	case ACTION_ELEMENT_COLOR:
		setElementColor(e, value1->val(), value2->val(), value3->val());
		break;
	}
#if LOGLEVEL > 7
	print(tmp);
#endif
	precalc(3);
}

char *ActionElement::toString()
{
	char *tmp = new char[1024];
	sprintf(tmp, "ELEMENT");
	return tmp;
}

ActionElement::~ActionElement()
{
	delete (elementname);
	delete (value1);
	delete (value2);
	delete (value3);
}

void ActionElementBS1::exec()
{
	// ICON && menuorder
	Uint16 e = findElement(elementname, true);
	if (e == 1) return;
	setElementWeight(e, weight->val());
	setElementSpray(e, spay->val());
	setElementSlide(e, slide->val());
	setElementViscousity(e, viscousity->val());
	setElementColor(e, rvalue->val(), gvalue->val(), bvalue->val());
	setElementIcon(e, getPic(icon));
	addDie(e, findElement(dieto, true), dierate->val());
	addElementToGroup(getGroup(findGroup(group, true, -1)), e, menuorder->val());
}

char *ActionElementBS1::toString()
{
	char *icontext = icon->toString();
	char *tmp = new char[1024 + strlen(elementname) + strlen(group) + strlen(rvalue->text) + strlen(gvalue->text) + strlen(bvalue->text) + strlen(weight->text) + strlen(spay->text) + strlen(slide->text) + strlen(viscousity->text) + strlen(dierate->text) + strlen(dieto) + strlen(menuorder->text) + strlen(icontext)];
	sprintf(tmp, "ELEMENT \"%s\" \"%s\" %s %s %s %s %s %s %s %s \"%s\" %s %s", elementname, group,
			rvalue->text, gvalue->text, bvalue->text, weight->text, spay->text,
			slide->text, viscousity->text, dierate->text, dieto, menuorder->text, icontext);
	return tmp;
}

ActionElementBS1::~ActionElementBS1()
{
	delete (elementname);
	delete (group);
	delete (dieto);
	delete (dierate);
	delete (rvalue);
	delete (gvalue);
	delete (bvalue);
	delete (weight);
	delete (spay);
	delete (slide);
	delete (viscousity);
	delete (menuorder);
}

void ActionElementDie::exec()
{
	addDie(findElement(elementname, true), findElement(dieto, true), rate->val());
}

char *ActionElementDie::toString()
{
	char *tmp = new char[1024 + strlen(elementname) + strlen(dieto) + strlen(rate->text)];
	sprintf(tmp, "ELEMENT \"%s\" DIE \"%s\" %s", elementname, dieto, rate->text);
	return tmp;
}

ActionElementDie::~ActionElementDie()
{
	delete (elementname);
	delete (dieto);
	delete (rate);
}

void ActionInteraction::exec()
{
	Uint16 ex;
	Group *g;
	Group *g2 = 0;

	std::list<Uint16> e1s;
	std::list<Uint16> e2s;
	std::list<Uint16> e3s;
	std::list<Uint16> e4s;
	std::list<int> rs;
	std::list<int>::iterator it;
	std::list<int>::iterator it2;
	std::list<Varint *>::iterator rate;
	std::list<Trigger *>::iterator trigger;
	std::list<Uint16>::iterator e1;
	std::list<Uint16>::iterator e2;
	std::list<Uint16>::iterator e3;
	std::list<Uint16>::iterator e4;
	std::list<char *>::iterator i;
	std::list<char *>::iterator i2;

	for (i = elements1.begin(); i != elements1.end(); i++)
		if (strstr(*i, "GROUP:") == *i)
		{
			g = getGroup(findGroup(*i + 6, false, -1));
			for (it = g->elements.begin(); it != g->elements.end(); it++)
				e1s.push_front(*it);
		}
		else
			e1s.push_back(findElement(*i, true));

	for (i = elements2.begin(); i != elements2.end(); i++)
		if (strstr(*i, "GROUP:") == *i)
		{
			g = getGroup(findGroup(*i + 6, false, -1));
			for (it = g->elements.begin(); it != g->elements.end(); it++)
				e2s.push_front(*it);
		}
		else
			e2s.push_back(findElement(*i, true));
	i2 = toothers.begin();
	for (i = toselfs.begin(); i != toselfs.end(); i++)
	{
		if (*i && (strstr(*i, "GROUP:") == *i))
		{
			g = getGroup(findGroup(*i + 6, false, -1));
			for (it = g->elements.begin(); it != g->elements.end(); it++)
				if (*i2 && (strstr(*i2, "GROUP:") == *i2))
				{
					if (g2 == 0)
						g2 = getGroup(findGroup(*i2 + 6, false, -1));
					for (it2 = g2->elements.begin(); it2 != g2->elements.end(); it2++)
					{
						e3s.push_back(*it);
						e4s.push_back(*it2);
					}
				}
				else
				{
					e3s.push_back(*it);
					e4s.push_back(findElement(*i2, true));
				}
		}
		else if (*i2 && (strstr(*i2, "GROUP:") == *i2))
		{
			g = getGroup(findGroup(*i2 + 6, false, -1));
			for (it2 = g->elements.begin(); it2 != g->elements.end(); it2++)
			{
				e3s.push_back(findElement(*i));
				e4s.push_back(*it2);
			}
		}
		else
		{
			e3s.push_back(findElement(*i, true));
			e4s.push_back(findElement(*i2, true));
		}
		i2++;
		e3s.push_back(32767);
		e4s.push_back(32767);
	}

	int restrate = 32768;
	for (rate = rates.begin(); rate != rates.end(); rate++)
	{
		int tmp = (*rate)->val();
		int tmp2;
		if (restrate > 0)
			tmp2 = tmp * 32768 / restrate;
		else
			tmp2 = 0;
		if (tmp2 > 32768)
			tmp2 = 32768;
		rs.push_back(tmp2);
		restrate -= tmp;
	}
	if (except)
		ex = findElement(except, true);
	else
		ex = 1;

	for (e1 = e1s.begin(); e1 != e1s.end(); e1++)
	{
		for (e2 = e2s.begin(); e2 != e2s.end(); e2++)
		{
			it = rs.begin();
			e4 = e4s.begin();
			for (e3 = e3s.begin(); e3 != e3s.end(); e3++)
			{
				if ((*e3 == 32767) && (*e4 == 32767))
					it++;
				else
					addInteraction(*e1, *e2, *e3, *e4, *it, ex, 0, at->val());
				e4++;
			}
			for (trigger = triggers.begin(); trigger != triggers.end(); trigger++)
			{
				addInteraction(*e1, *e2, 0, 0, *it, ex, *trigger, at->val());
				it++;
			}
		}
	}
}

char *ActionInteraction::toString()
{
	char *tmp;
	char *e;
	if (except)
	{
		e = new char[128 + strlen(except)];
		sprintf(e, " \"%s\"", except);
	}
	else
		e = "";
	char *a;
	if (at->val() != -1)
	{
		a = new char[128 + strlen(at->text)];
		sprintf(a, "AT %s", at->text);
	}
	else
		a = "";
	std::list<char *>::iterator i4 = elements1.begin();
	std::list<char *>::iterator i5 = elements2.begin();
	if (!triggers.size())
	{
		std::list<char *>::iterator i1 = toselfs.begin();
		std::list<char *>::iterator i2 = toothers.begin();
		std::list<Varint *>::iterator i3 = rates.begin();
		int len = 0;
		while (i1 != toselfs.end())
		{
			len += strlen(*i1) + strlen(*i2) + strlen((*i3)->text);
			i1++;
			i2++;
			i3++;
			len += 10;
		}

		tmp = new char[1024 + strlen(*i4) + strlen(*i5) + strlen(at->text) + strlen(e) + len];
		char *tmp2 = new char[1024 + strlen(*i4) + strlen(*i5) + strlen(at->text) + strlen(e)];
		sprintf(tmp, "INTERACTION%s \"%s\" \"%s\"", a, *i4, *i5);

		i1 = toselfs.begin();
		i2 = toothers.begin();
		i3 = rates.begin();
		while (i1 != toselfs.end())
		{
			strcpy(tmp2, tmp);
			sprintf(tmp, "%s \"%s\" \"%s\" %s", tmp2, *i1, *i2, (*i3)->text);
			i1++;
			i2++;
			i3++;
		}

		strcpy(tmp2, tmp);
		sprintf(tmp, "%s%s", tmp2, e);
		delete (tmp2);
	}
	else
	{
		std::list<Trigger *>::iterator i1 = triggers.begin();
		std::list<Varint *>::iterator i3 = rates.begin();
		int len = 0;
		while (i1 != triggers.end())
		{
			len += strlen((*i1)->name) + strlen((*i3)->text);
			i1++;
			i3++;
			len += 10;
		}

		tmp = new char[1024 + strlen(*i4) + strlen(*i5) + strlen(at->text) + strlen(e) + len];
		char *tmp2 = new char[1024 + strlen(*i4) + strlen(*i5) + strlen(at->text) + strlen(e)];
		sprintf(tmp, "INTERACTIONTRIGGER%s \"%s\" \"%s\"", a, *i4, *i5);

		i1 = triggers.begin();
		i3 = rates.begin();
		while (i1 != triggers.end())
		{
			strcpy(tmp2, tmp);
			sprintf(tmp, "%s \"%s\" %s", tmp2, (*i1)->name, (*i3)->text);
			i1++;
			i3++;
		}

		strcpy(tmp2, tmp);
		sprintf(tmp, "%s%s", tmp2, e);
		delete (tmp2);
	}
	if (except)
		delete (e);
	if (at->val() != -1)
		delete (a);
	return tmp;
}

ActionInteraction::~ActionInteraction()
{
	std::list<char *>::iterator it;
	std::list<Varint *>::iterator it2;
	for (it = elements1.begin(); it != elements1.end(); it++)
		delete (*it);
	for (it = elements2.begin(); it != elements2.end(); it++)
		delete (*it);
	for (it = toselfs.begin(); it != toselfs.end(); it++)
		delete (*it);
	for (it = toothers.begin(); it != toothers.end(); it++)
		delete (*it);
	for (it2 = rates.begin(); it2 != rates.end(); it2++)
		delete (*it2);
	delete (except);
	delete (at);
}

void ActionRemoveInteraction::exec()
{
	if (index == NULL)
		clearInteraction(findElement(element, true));
	else
		removeInteraction(findElement(element, true), index->val());
}

char *ActionRemoveInteraction::toString()
{
	char *tmp;
	if (index)
	{
		tmp = new char[1024 + strlen(element) + strlen(index->text)];
		sprintf(tmp, "INTERACTIONREMOVE \"%s\" %s", element, index->text);
	}
	else
	{
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "INTERACTIONCLEAR \"%s\"", element);
	}
	return tmp;
}

ActionRemoveInteraction::~ActionRemoveInteraction()
{
	delete (element);
	delete (index);
}

void ActionClearDie::exec()
{
	clearDie(findElement(element, true));
}

char *ActionClearDie::toString()
{
	char *tmp = new char[1024 + strlen(element)];
	sprintf(tmp, "DIECLEAR \"%s\"", element);
	return tmp;
}

ActionClearDie::~ActionClearDie()
{
	delete (element);
}

void ActionClearElements::exec()
{
	clearelements();
}

char *ActionClearElements::toString()
{
	char *tmp = new char[1024];
	sprintf(tmp, "ELEMENTSCLEAR");
	return tmp;
}

void ActionSave::exec()
{
	char *t;
	switch (screenid)
	{
	case ACTION_SAVE_SCREEN_ALL:
		break;
	case ACTION_SAVE_SCREEN_SAND:
		if (!strcmp(filename, "FILEDIALOG"))
		{
			char *text;
			if (text = savedialog("PNG File\0*.png\0BMP File\0*.bmp\0BS2 Config\0*.bs2\0", 0))
				save(getRealSandSurface(), text);
		}
		else
		{
			char *t = messageReplace(filename);
			checkfile(t, true);
			save(getRealSandSurface(), t);
		}
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		quicksave(((Varint *)filename)->val());
		break;
	case ACTION_SAVE_STAMP:
		t = messageReplace(filename);
		checkfile(t, true);
		if ((id->val() > 0) && (id->val() < MAX_STAMPS))
			if (stamps[id->val()])
				SDL_SaveBMP(stamps[id->val()], checkfilename(t));
		break;
	case ACTION_SAVE_VAR:
	{
		char tmp[255];
		sprintf(tmp, "SET \"%s\" %i\n", filename, id->val());
		savefile.write(tmp, strlen(tmp));
	}
	break;
	case ACTION_SAVE_TIMERS:
	{
		std::list<Timer *>::iterator it;
		std::list<Timer *> timerlist;
		timerlist.clear();
		char tmp[4096];
		while (!frametriggers.empty())
		{
			Timer *t = frametriggers.top();
			if (t && t->trigger && t->trigger->name)
			{
				if (t->params)
					sprintf(tmp, "TIMER \"%s\" %i %i %i %i %i %i %i %i %i %i\n", t->trigger->name,
							t->params[0], t->params[1], t->params[2], t->params[3], t->params[4],
							t->params[5], t->params[6], t->params[7], t->params[8], t->params[9]);
				else
					sprintf(tmp, "TIMER \"%s\"\n", t->trigger->name);
				savefile.write(tmp, strlen(tmp));
				timerlist.push_back(t);
			}
			frametriggers.pop();
		}
		for (it = timerlist.begin(); it != timerlist.end(); it++)
			frametriggers.push(*it);
	}
	break;
	}
}

char *ActionSave::toString()
{
	char *tmp = 0;
	switch (screenid)
	{
	case ACTION_SAVE_SCREEN_SAND:
		tmp = new char[1024];
		sprintf(tmp, "SAVE SAND \"%s\"", filename);
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		tmp = new char[1024];
		sprintf(tmp, "SAVE QUICKSAND \"%s\"", ((Varint *)filename)->text);
		break;
	case ACTION_SAVE_STAMP:
		tmp = new char[1024];
		sprintf(tmp, "SAVE STAMP \"%s\" %s", filename, id->text);
		break;
	case ACTION_SAVE_VAR:
		tmp = new char[1024];
		sprintf(tmp, "SAVE VAR \"%s\" %s", filename, id->text);
		break;
	}
	return tmp;
}

ActionSave::~ActionSave()
{
	delete (id);
}

void ActionFile::exec()
{
#ifdef COMPILER_REMOVE
	if (function == ACTION_FILE_DELETE)
	{
		char *t = messageReplace(filename);
		checkfile(t, true);
		remove(t);
	}
#endif
	if (function == ACTION_FILE_OPEN)
	{
		if (!strcmp(filename, "FILEDIALOG"))
		{
			char *text;
			if (text = savedialog("*.*\0*.*\0", 0))
			{
				checkfile(text, false);
				if ((param) && !strcmp(param, "NEW"))
				{
					remove(checkfilename(text));
				}
				savefile.open(checkfilename(text), std::ios::app | std::ios::out | std::ios::ate | std::ios::binary);
			}
		}
		else
		{
			char *t = messageReplace(filename);
			checkfile(t, true);
			if ((param) && !strcmp(param, "NEW"))
			{
				remove(checkfilename(filename));
			}
			savefile.open(checkfilename(t), std::ios::app | std::ios::out | std::ios::ate | std::ios::binary);
		}
	}
	if (function == ACTION_FILE_CLOSE)
		savefile.close();
}

char *ActionFile::toString()
{
	char *tmp = 0;
	switch (function)
	{
	case ACTION_FILE_DELETE:
		tmp = new char[1024];
		sprintf(tmp, "FILE DELETE \"%s\"", filename);
		break;
	case ACTION_FILE_OPEN:
		tmp = new char[1024];
		sprintf(tmp, "FILE OPEN \"%s\"", filename);
		break;
	case ACTION_FILE_CLOSE:
		tmp = new char[1024];
		sprintf(tmp, "FILE CLOSE");
		break;
	}
	return tmp;
}

ActionFile::~ActionFile()
{
	delete (filename);
}

void ActionLoad::exec()
{
#if LOGLEVEL > 7
	char tmp[255];
	tmp[0] = 0;
#endif
	char *t;
	switch (screenid)
	{
	case ACTION_SAVE_SCREEN_ALL:
		break;
	case ACTION_SAVE_SCREEN_SAND:
		if (!strcmp(filename, "FILEDIALOG"))
		{
			char *text;
			if (text = opendialog("PNG File\0*.png\0BMP File\0*.bmp\0BS2 Config\0*.bs2\0", 0))
			{
				checkfile(text, false);
				load(text);
			}
		}
		else
		{
			char *t = messageReplace(filename);
			checkfile(t, true);
			load(t);
		}
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		quickload(((Varint *)filename)->val());
		break;
	case ACTION_SAVE_STAMP:
		t = messageReplace(filename);
		checkfile(t, true);
		if ((id->val() > 0) && (id->val() < MAX_STAMPS))
			stamps[id->val()] = SDL_LoadBMP(checkfilename(t));
		break;
	case ACTION_SAVE_FGLAYER:
		if (fglayer)
			SDL_FreeSurface(fglayer);
		fglayer = 0;
		if (!strcmp(filename, "FILEDIALOG"))
		{
			char *text;
			if (text = opendialog("BMP File\0*.bmp\0", 0))
			{
				checkfile(text, false);
				fglayer = SDL_LoadBMP(checkfilename(text));
			}
		}
		else
		{
			checkfile(filename, true);
			fglayer = SDL_LoadBMP(checkfilename(filename));
		}
		break;
	case ACTION_SAVE_BGLAYER:
		if (bglayer)
			SDL_FreeSurface(bglayer);
		bglayer = 0;
		if (!strcmp(filename, "FILEDIALOG"))
		{
			char *text;
			if (text = opendialog("BMP File\0*.bmp\0", 0))
			{
				checkfile(text, false);
				bglayer = SDL_LoadBMP(checkfilename(text));
			}
		}
		else
		{
			checkfile(filename, true);
			bglayer = SDL_LoadBMP(checkfilename(filename));
		}
		break;
	case ACTION_SAVE_FONT:
		fontname = filename;
		break;
	case ACTION_SAVE_MENUFONT:
		loadMenuFont(filename, id->val());
		break;
	}
#if LOGLEVEL > 7
	print(tmp);
#endif
}

char *ActionLoad::toString()
{
	char *tmp = 0;
	switch (screenid)
	{
	case ACTION_SAVE_SCREEN_SAND:
		tmp = new char[1024];
		sprintf(tmp, "LOAD SAND \"%s\"", filename);
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		tmp = new char[1024];
		sprintf(tmp, "LOAD QUICKSAND \"%s\"", ((Varint *)filename)->text);
		break;
	case ACTION_SAVE_STAMP:
		tmp = new char[1024];
		sprintf(tmp, "LOAD STAMP \"%s\" %s", filename, id->text);
		break;
	case ACTION_SAVE_FGLAYER:
		tmp = new char[1024];
		sprintf(tmp, "LOAD FGLAYER \"%s\"", filename);
		break;
	case ACTION_SAVE_BGLAYER:
		tmp = new char[1024];
		sprintf(tmp, "LOAD BGLAYER \"%s\"", filename);
		break;
	case ACTION_SAVE_FONT:
		tmp = new char[1024];
		sprintf(tmp, "LOAD FONT \"%s\"", filename);
		break;
	case ACTION_SAVE_MENUFONT:
		tmp = new char[1024];
		sprintf(tmp, "LOAD MENUFONT \"%s\" %s", filename, id->text);
		break;
	}
	return tmp;
}

ActionLoad::~ActionLoad()
{
	delete (id);
}

void ActionButton::exec()
{
	Button *btn = new Button((Button *)button);
	if (tiptype)
	{
		int i;
		getVar(btn->tiptext, &i);
		if (tiptype == 1)
			btn->tiptext = getElement(i)->name;
		if (tiptype == 2)
			btn->tiptext = getGroup(i)->name;
	}
	if (btn->tiptext && !strcmp(btn->tiptext, "MESSAGE"))
	{
		delete (btn->tiptext);
		btn->tiptext = new char[strlen(messagestring) + 1];
		strcpy(btn->tiptext, messagestring);
	}
	if (btn->icon->type && ((!strcmp(btn->icon->type, "TEXT")) || (!strcmp(btn->icon->type, "Text"))) && btn->icon->text && !strcmp(btn->icon->text, "MESSAGE"))
	{
		strcpy(btn->icon->text, messagestring);
	}
	btn->border = true;
	if (r)
		btn->r = r->val();
	else
		btn->border = false;
	if (g)
		btn->g = g->val();
	else
		btn->border = false;
	if (b)
		btn->b = b->val();
	else
		btn->border = false;
	btn->params = calcparams(params);
	btn->icon->calc();
	addButtonToMenuBar(bar, btn);
	redrawmenu(3);
}

char *ActionButton::toString()
{
	char *w = 0;
	if (bar == MENU_BAR_TOP)
		w = "TOP";
	if (bar == MENU_BAR_LEFT)
		w = "LEFT";
	if (bar == MENU_BAR_RIGHT)
		w = "RIGHT";
	if (bar == MENU_BAR_BOTTOM)
		w = "BOTTOM";
	if (bar == MENU_BAR_SUB)
		w = "SUB";
	char *a = "ADD";
	if (((Button *)button)->mode)
		a = "ADDDRAG";
	if (r)
		a = "ADDBORDER";
	char *tt = 0;
	char *ttt = "";
	if (tiptype == ACTION_BUTTON_TIPTYPE_TEXT)
	{
		tt = "TEXT";
		ttt = "\"";
	}
	if (tiptype == ACTION_BUTTON_TIPTYPE_ELEMENT)
	{
		tt = "ELEMENT";
	}
	if (tiptype == ACTION_BUTTON_TIPTYPE_GROUP)
	{
		tt = "GROUP";
	}
	char *tmp = new char[1024];
	char *i = ((Button *)button)->icon->toString();
	sprintf(tmp, "MENU %s %s %s %s%s%s %s", w, a, tt, ttt, ((Button *)button)->tiptext, ttt, i);
	delete (i);
	return tmp;
}

ActionButton::~ActionButton()
{
	delete (r);
	delete (g);
	delete (b);
	deleteparams(params);
}

char *ActionTrigger::toString()
{
	char *tmp;
	if (triggername)
	{
		tmp = new char[1024 + strlen(triggername)];
		sprintf(tmp, "ON \"%s\" %s", triggername, action->toString());
	}
	else
	{
		tmp = new char[1024];
		sprintf(tmp, "TRIGGER");
	}
	return tmp;
}

ActionTrigger::~ActionTrigger()
{
	delete (triggername);
}

void ActionStatus::exec()
{
	char *pos = (status_text + strlen(status_text));
	static Element *e;
	static Group *g;
	switch (function)
	{
	case STATUS_CLEAR:
		status_text[0] = 0;
		break;
	case STATUS_ADDTEXT:
		strcpy(pos, text);
		break;
	case STATUS_ADDNUMBER:
		char t[256];
		sprintf(t, "%i", v->val());
		strcpy(pos, t);
		break;
	case STATUS_ADDELEMENT:
		e = getElement(v->val());
		if (e)
			strcpy(pos, e->name);
		break;
	case STATUS_ADDGROUP:
		g = getGroup(v->val());
		strcpy(pos, g->name);
		break;
	case STATUS_MOUSEOVER:
		char *tmp;
		if (tmp = getmouseover())
			strcpy(pos, tmp);
		break;
	}
}

char *ActionStatus::toString()
{
	char *tmp = 0;
	switch (function)
	{
	case STATUS_CLEAR:
		tmp = new char[1024];
		sprintf(tmp, "STATUS CLEAR");
		break;
	case STATUS_ADDTEXT:
		tmp = new char[1024];
		sprintf(tmp, "STATUS ADDTEXT \"%s\"", text);
		break;
	case STATUS_ADDNUMBER:
		tmp = new char[1024];
		sprintf(tmp, "STATUS ADDNUMBER %s", v->text);
		break;
	case STATUS_ADDELEMENT:
		tmp = new char[1024];
		sprintf(tmp, "STATUS ADDELEMENT %s", v->text);
		break;
	case STATUS_ADDGROUP:
		tmp = new char[1024];
		sprintf(tmp, "STATUS ADDGROUP %s", v->text);
		break;
	case STATUS_MOUSEOVER:
		tmp = new char[1024];
		sprintf(tmp, "STATUS ADDMOUSEOVER");
		break;
	}
	return tmp;
}

ActionStatus::~ActionStatus()
{
	delete (text);
	delete (v);
}

void ActionRemote::exec()
{
	char *tmp;
	if (type == REMOTE_SET)
	{
		tmp = new char[1024 + strlen(text) + strlen(val->text)];
		sprintf(tmp, "SET \"%s\" %i", text, val->val());
		print(tmp, mid->val(), strlen(tmp));
		delete (tmp);
	}
	if (type == REMOTE_EXEC)
	{
		tmp = new char[1024 + strlen(text)];
		sprintf(tmp, "EXEC \"%s\"", text);
		print(tmp, mid->val(), strlen(tmp));
		delete (tmp);
	}
}

char *ActionRemote::toString()
{
	char *tmp = 0;
	if (type == REMOTE_SET)
	{
		tmp = new char[1024 + strlen(text) + strlen(mid->text) + strlen(val->text)];
		sprintf(tmp, "REMOTE %s SET \"%s\" %s", mid->text, text, val->text);
	}
	if (type == REMOTE_EXEC)
	{
		tmp = new char[1024 + strlen(text) + strlen(mid->text)];
		sprintf(tmp, "REMOTE %s EXEC \"%s\"", val->text, text);
	}
	return tmp;
}

ActionRemote::~ActionRemote()
{
	delete (text);
	delete (mid);
	delete (val);
}

void ActionConnect::exec()
{
	setVar(var, connect(host, port->val()), true);
}

char *ActionConnect::toString()
{
	char *tmp;
	tmp = new char[1024 + strlen(host) + strlen(port->text) + strlen(var)];
	sprintf(tmp, "CONNECT \"%s\" %s \"%s\"", host, port->text, var);
	return tmp;
}

ActionConnect::~ActionConnect()
{
	delete (var);
	delete (host);
	delete (port);
}

void ActionWrite::exec()
{
	switch (type)
	{
	case WRITE_TEXT:
		sandwrite(element->val(), x->val(), y->val(), size->val(), text, align);
		break;
	case WRITE_NUMBER:
		char tmp[255];
		sprintf(tmp, "%i", v->val());
		sandwrite(element->val(), x->val(), y->val(), size->val(), tmp, align);
		break;
	case WRITE_ELEMENT:
		sandwrite(element->val(), x->val(), y->val(), size->val(), getElement(v->val())->name, align);
		break;
	case WRITE_GROUP:
		sandwrite(element->val(), x->val(), y->val(), size->val(), getGroup(v->val())->name, align);
		break;
	case WRITE_MESSAGE:
		sandwrite(element->val(), x->val(), y->val(), size->val(), messagestring, align);
		break;
	case WRITE_STRING:
		int i = v->val();
		if (strings[i])
			sandwrite(element->val(), x->val(), y->val(), size->val(), strings[i], align);
		break;
	}
}

char *ActionWrite::toString()
{
	char *tmp = 0;
	switch (type)
	{
	case WRITE_TEXT:
		tmp = new char[1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text) + strlen(text)];
		sprintf(tmp, "WRITE %s %s %s %s TEXT	\"%s\"", element->text, x->text, y->text, size->text, text);
		break;
	case WRITE_NUMBER:
		tmp = new char[1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text) + strlen(v->text)];
		sprintf(tmp, "WRITE %s %s %s %s NUMBER %s", element->text, x->text, y->text, size->text, v->text);
		break;
	case WRITE_ELEMENT:
		tmp = new char[1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text) + strlen(v->text)];
		sprintf(tmp, "WRITE %s %s %s %s ELEMENT %s", element->text, x->text, y->text, size->text, v->text);
		break;
	case WRITE_GROUP:
		tmp = new char[1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text) + strlen(v->text)];
		sprintf(tmp, "WRITE %s %s %s %s GROUP %s", element->text, x->text, y->text, size->text, v->text);
		break;
	case WRITE_MESSAGE:
		tmp = new char[1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text) + strlen(v->text)];
		sprintf(tmp, "WRITE %s %s %s %s MESSAGE 0", element->text, x->text, y->text, size->text);
		break;
	case WRITE_STRING:
		tmp = new char[1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text) + strlen(v->text)];
		sprintf(tmp, "WRITE %s %s %s %s STRING %s", element->text, x->text, y->text, size->text, v->text);
		break;
	}
	return tmp;
}

ActionWrite::~ActionWrite()
{
	delete (v);
	delete (x);
	delete (y);
	delete (size);
	delete (element);
	delete (text);
}

void ActionDraw::exec()
{
	sanddraw(element->val(), brush, drawx->val(), drawy->val(), dx->val(), dy->val(), a1->val(), a2->val());
};

char *ActionDraw::toString()
{
	char *b = "", *p1 = "", *p2 = "", *p3 = "", *p4 = "", *p5 = "", *p6 = "";
	if (brush == BRUSH_FILLEDCIRCLE)
		b = "FILLEDCIRCLE";
	if (brush == BRUSH_CIRCLE)
		b = "CIRCLE";
	if (brush == BRUSH_RECT)
		b = "RECT";
	if (brush == BRUSH_FILLEDRECT)
		b = "FILLEDRECT";
	if (brush == BRUSH_LINE)
		b = "LINE";
	if (brush == BRUSH_FILL)
		b = "FILL";
	if (brush == REPLACE_FILLEDCIRCLE)
		b = "REPLACEFILLEDCIRCLE";
	if (brush == REPLACE_LINE)
		b = "REPLACELINE";
	if (brush == BRUSH_FILLEDCIRCLE)
		b = "FILLEDCIRCLE";
	if (brush == COPY_RECT)
		b = "COPYRECT";
	if (brush == ROTATE_RECT)
		b = "ROTATE";
	if (brush == BRUSH_POINT)
		b = "POINT";
	if (brush == BRUSH_RADNOMFILLEDCIRCLE)
		b = "RANDOMFILLEDCIRCLE";
	if (brush == COPY_STAMP)
		b = "COPYSTAMP";
	if (brush == PASTE_STAMP)
		b = "PASTESTAMP";
	if (brush == BRUSH_SWAPPOINTS)
		b = "SWAPPOINTS";
	if (brush == BRUSH_FILLEDELLIPSE)
		b = "FILLEDELLIPSE";
	if (brush == BRUSH_ELLIPSE)
		b = "ELLIPSE";
	if (brush == REPLACE_FILLEDELLIPSE)
		b = "REPLACEFILLEDELLIPSE";
	if (brush == BRUSH_RANDOMFILLEDELLIPSE)
		b = "RANDOMFILLEDELLIPSE";
	if (brush == BRUSH_POINTS)
		b = "FILLEDCIRCLE";
	if (brush == BRUSH_OBJECT)
		b = "OBJECT";
	if (brush == ROTATE_RECT)
		b = "ROTATERECT";
	if (drawx && drawx->text)
		p1 = drawx->text;
	if (drawy && drawy->text)
		p2 = drawy->text;
	if (dx && dx->text)
		p3 = dx->text;
	if (dy && dy->text)
		p4 = dy->text;
	if (a1 && a1->text)
		p5 = a1->text;
	if (a2 && a2->text)
		p6 = a2->text;
	char *tmp = new char[1024 + strlen(element->text) + strlen(p1) + strlen(p1) + strlen(p2) + strlen(p3) + strlen(p4) + strlen(p5) + strlen(p6)];
	sprintf(tmp, "DRAW %s %s %s %s %s %s %s %s", element->text, b, p1, p2, p3, p4, p5, p6);
	return tmp;
}

ActionDraw::~ActionDraw()
{
	delete (drawx);
	delete (drawy);
	delete (dx);
	delete (dy);
	delete (element);
	delete (a1);
	delete (a2);
}

void ActionDrawPoints::exec()
{
	std::list<int>::iterator xit = x.begin();
	std::list<int>::iterator yit = y.begin();
	int startx = xoffset->val();
	int starty = yoffset->val();
	while (xit != x.end())
	{
		sanddraw(element->val(), BRUSH_POINT, startx + *xit, starty + *yit, 0, 0, 0, 0);
		xit++;
		yit++;
	}
};

char *ActionDrawPoints::toString()
{
	char *tmp = new char[1024 + strlen(element->text) + x.size() * 20];
	char *tmp2 = new char[1024 + strlen(element->text) + x.size() * 20];
	std::list<int>::iterator xit = x.begin();
	std::list<int>::iterator yit = y.begin();
	sprintf(tmp, "DRAW %s POINTS %s %s", element->text, xoffset->text, yoffset->text);
	while (xit != x.end())
	{
		strcpy(tmp2, tmp);
		sprintf(tmp, "%s %i %i", tmp2, *xit, *yit);
		xit++;
		yit++;
	}
	delete (tmp2);
	return tmp;
}

ActionDrawPoints::~ActionDrawPoints()
{
	x.clear();
	y.clear();
	delete (xoffset);
	delete (yoffset);
	delete (element);
}

void ActionDrawObject::exec()
{
	std::list<char *>::iterator dit = data.begin();
	int startx = xoffset->val();
	int starty = yoffset->val();
	Uint16 e[256];
	for (int i = 0; i < 256; i++)
		if (elements[i])
			e[i] = elements[i]->val();
		else
			e[i] = 1;
	int y = 0;
	int sx = 1;
	if (sizex)
		sx = sizex->val();
	int sy = 1;
	if (sizey)
		sy = sizey->val();
	int tmp;
	bool scale = false;
	if ((sx == 1) && (sy == 1))
		scale = true;
	int xstep = 1, ystep = 1;
	if (sx < 0)
	{
		xstep = -sx;
		sx = 1;
	}
	if (sy < 0)
	{
		ystep = -sy;
		sy = 1;
	}
	while (dit != data.end())
	{
		char *c = *dit;
		int l = strlen(c);
		if (scale)
		{
			for (int i = 0; i < l; i++)
				if (e[(int)*(c + i)] != 1)
					sanddraw(e[(int)*(c + i)], BRUSH_POINT, startx + i, starty + y, 0, 0, 0, 0);
		}
		else
		{
			for (int y2 = y; y2 < y + sy; y2 += 1)
				for (int i = 0; i < l; i += xstep)
					if (e[(int)*(c + i)] != 1)
						for (int x2 = i * sx / xstep; x2 < i * sx / xstep + sx; x2++)
							sanddraw(e[(int)*(c + i)], BRUSH_POINT, startx + x2, starty + y2, 0, 0, 0, 0);
		}
		for (tmp = 0; tmp < ystep; tmp++)
		{
			dit++;
			if (dit == data.end())
				return;
		}
		y += sy;
	}
};

char *ActionDrawObject::toString()
{
	int len = 0;
	for (int i = 0; i < 256; i++)
		if (elements[i])
			len += strlen(elements[i]->text);
	std::list<char *>::iterator dit = data.begin();
	while (dit != data.end())
	{
		len += strlen(*dit) + 3;
		dit++;
	}
	char *tmp = new char[1024 + strlen(xoffset->text) + strlen(yoffset->text) + len];
	char *tmp2 = new char[1024 + strlen(xoffset->text) + strlen(yoffset->text) + len];
	sprintf(tmp, "DRAW 0 OBJECT %s %s", xoffset->text, yoffset->text);
	for (int i2 = 0; i2 < 256; i2++)
	{
		if (elements[i2])
		{
			strcpy(tmp2, tmp);
			sprintf(tmp, "%s %c %s", tmp2, (char)i2, elements[i2]->text);
		}
	}
	strcpy(tmp2, tmp);
	sprintf(tmp, "%s {\n", tmp2);
	dit = data.begin();
	while (dit != data.end())
	{
		strcpy(tmp2, tmp);
		sprintf(tmp, "%s \"%s\"\n", tmp2, *dit);
		dit++;
	}
	strcpy(tmp2, tmp);
	sprintf(tmp, "%s\n}", tmp2);
	delete (tmp2);
	return tmp;
}

ActionDrawObject::~ActionDrawObject()
{
	delete (xoffset);
	delete (yoffset);
	delete (sizex);
	delete (sizey);
	for (int i = 0; i < 256; i++)
		delete (elements[i]);
	std::list<char *>::iterator it;
	for (it = data.begin(); it != data.end(); it++)
		delete (*it);
}

void ActionTimer::exec()
{
	Timer *t;
	if (!timerStack.empty())
	{
		t = timerStack.top();
		timerStack.pop();
	}
	else
		t = new Timer();
	t->trigger = trigger;
	if (params)
		t->params = calcparams(params);
	else
		t->params = 0;
	switch (type)
	{
	case ACTION_TIMER_FRAMES:
		t->value = thisframe + value->val();
		frametriggers.push(t);
		break;
	case ACTION_TIMER_SEC:
		break;
	case ACTION_TIMER_MSEC:
		break;
	default:
		timerStack.push(t);
		break;
	}
}

char *ActionTimer::toString()
{
	char *tmp = 0;
	switch (type)
	{
	case ACTION_TIMER_FRAMES:
		tmp = new char[1024 + strlen(value->text) + strlen(trigger->name)];
		sprintf(tmp, "TIMER %s FRAMES \"%s\"", value->text, trigger->name);
		break;
	case ACTION_TIMER_SEC:
		break;
	case ACTION_TIMER_MSEC:
		break;
	}
	return tmp;
}

ActionTimer::~ActionTimer()
{
	delete (value);
	deleteparams(params);
}

void ActionClearTimer::exec()
{
	timercleared = true;
	if (!trigger)
	{
		while (!frametriggers.empty())
			frametriggers.pop();
	}
	else
	{
		std::list<Timer *> tmptimers;
		int c = frametriggers.size();
		for (int i = 0; i < c; i++)
		{
			tmptimers.push_back(frametriggers.top());
			frametriggers.pop();
		}
		std::list<Timer *>::iterator it = tmptimers.begin();
		int b = 0;
		while (it != tmptimers.end())
		{
			if (trigger != (*it)->trigger)
				frametriggers.push(*it);
			else if (!removeall && b++)
				frametriggers.push(*it);
			it++;
		}
	}
}

char *ActionClearTimer::toString()
{
	char *tmp;
	if (!trigger)
	{
		tmp = new char[1024];
		sprintf(tmp, "TIMER CLEAR");
	}
	else
	{
		if (removeall)
		{
			tmp = new char[1024 + strlen(trigger->name)];
			sprintf(tmp, "TIMER REMOVEALL \"%s\"", trigger->name);
		}
		else
		{
			tmp = new char[1024 + strlen(trigger->name)];
			sprintf(tmp, "TIMER REMOVE \"%s\"", trigger->name);
		}
	}
	return tmp;
}

void ActionGroup::exec()
{
	Uint16 i;
	switch (function)
	{
	case ACTION_GROUP_ADD:
		i = findElement(element, true);
		addElementToGroup(getGroup(findGroup(groupname, true, -1)), i, order->val());
		break;
	case ACTION_GROUP_CLEAR:
		clearGroup(groupname);
		break;
	case ACTION_GROUP_SET_ICON:
		getGroup(findGroup(groupname, true, order->val()))->icon = icon;
		break;
	case ACTION_GROUP_CLEARALL:
		clearGroups();
		break;
	}
}

char *ActionGroup::toString()
{
	char *tmp = 0;
	switch (function)
	{
	case ACTION_GROUP_ADD:
		tmp = new char[1024 + strlen(groupname) + strlen(order->text)];
		sprintf(tmp, "GROUP \"%s\" ADD \"%s\" %s", groupname, element, order->text);
		break;
	case ACTION_GROUP_CLEAR:
		tmp = new char[1024 + strlen(groupname)];
		sprintf(tmp, "GROUP \"%s\" CLEAR", groupname);
		break;
	case ACTION_GROUP_SET_ICON:
		tmp = new char[1024 + strlen(groupname) + strlen(order->text) + strlen(icon->toString())];
		sprintf(tmp, "GROUP \"%s\" %s %s", groupname, order->text, icon->toString());
		break;
	case ACTION_GROUP_CLEARALL:
		tmp = new char[1024];
		sprintf(tmp, "GROUP CLEARALL");
		break;
	}
	return tmp;
}

ActionGroup::~ActionGroup()
{
	delete (order);
	delete (element);
	delete (groupname);
}

void ActionKey::exec()
{
	addkey(keyname, v->val());
}

char *ActionKey::toString()
{
	char *tmp = new char[1024 + strlen(keyname) + strlen(v->text)];
	sprintf(tmp, "KEYCODE %s %s", keyname, v->text);
	return tmp;
}

ActionKey::~ActionKey()
{
	delete (v);
}

void ActionList::exec()
{
	int c, i, i2;
	Element *e;
	Group *g;
	char tmp[255];
	std::list<int>::iterator it;
	std::list<Var *>::iterator it2;
	std::list<Var *> *vars;
	std::list<Interaction *>::iterator it3;
	std::list<Die *>::iterator it4;
	std::list<Trigger *>::iterator it5;
	std::list<Action *>::iterator it6;
	std::list<Timer *>::iterator it7;
	std::list<Timer *> timerlist;
	Trigger *trigger;
	switch (function)
	{
	case ACTION_LIST_ELEMENTS:
		print("(ELEMENTS)", owner);
		e = getElement(0);
		for (i = 2; e[i].name; i++)
			print(e[i].name, owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_GROUP:
		print("(GROUP)", owner);
		e = getElement(0);
		g = getGroup(findGroup(element, false, -1));
		for (it = g->elements.begin(); it != g->elements.end(); it++)
			print(e[*it].name, owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_GROUPS:
		print("(GROUP)", owner);
		c = countGroups();
		for (i = 0; i < c; i++)
			print(getGroup(i)->name, owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_VARS:
		print("(VARS)", owner);
		vars = getVars();
		for (it2 = vars->begin(); it2 != vars->end(); it2++)
			print((*it2)->name, owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_INTERACTIONS:
		print("(INTERACTIONS)", owner);
		e = getElement(findElement(element, false));
		for (it3 = e->interactions->begin(); it3 != e->interactions->end(); it3++)
		{
			char *tmp = (*it3)->toString(element);
			print((*it3)->toString(element), owner);
			delete (tmp);
		}
		print("(END)", owner);
		break;
	case ACTION_LIST_ELEMENTGROUPS:
		sprintf(tmp, "(ELEMENTGROUP:%s)", element);
		print(tmp, owner);
		i2 = findElement(element, false);
		c = countGroups();
		for (i = 0; i < c; i++)
		{
			if (isElementInGroup(getGroup(i), i2))
				print(getGroup(i)->name, owner);
		}
		print("(END)", owner);
		break;
	case ACTION_LIST_DIETOS:
		sprintf(tmp, "(ELEMENTDIETOS:%s)", element);
		print(tmp, owner);
		e = getElement(findElement(element, false));
		for (it4 = e->dies->begin(); it4 != e->dies->end(); it4++)
			print(getElement((*it4)->dieto)->name, owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_TRIGGERS:
		print("(TRIGGERS)", owner);
		for (it5 = triggers.begin(); it5 != triggers.end(); it5++)
			print((*it5)->name, owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_TRIGGEREXECS:
		print("(TRIGGEREXECS)", owner);
		i = 0;
		for (it5 = triggers.begin(); it5 != triggers.end(); it5++)
		{
			sprintf(tmp, "%10u %s", (*it5)->execcount, (*it5)->name);
			i += (*it5)->execcount;
			print(tmp, owner);
		}
		print("(END)", owner);
		sprintf(tmp, "(ALLEXECS) %i", i);
		print(tmp, owner);
		break;
	case ACTION_LIST_ACTIONS:
		trigger = findTrigger(element, owner);
		sprintf(tmp, "(ACTIONS:%s)", element);
		print(tmp, owner);
		for (it6 = trigger->actions.begin(); it6 != trigger->actions.end(); it6++)
			print((*it6)->toString(), owner);
		print("(END)", owner);
		break;
	case ACTION_LIST_TIMERS:
		print("(TIMERS)", owner);
		timerlist.clear();
		while (!frametriggers.empty())
		{
			Timer *t = frametriggers.top();
			if (t && t->trigger && t->trigger->name)
			{
				sprintf(tmp, "%i %s", t->value - thisframe, t->trigger->name);
				print(tmp, owner);
				timerlist.push_back(t);
			}
			frametriggers.pop();
		}
		for (it7 = timerlist.begin(); it7 != timerlist.end(); it7++)
			frametriggers.push(*it7);
		print("(END)", owner);
		break;
	}
}

char *ActionList::toString()
{
	char *tmp = 0;
	switch (function)
	{
	case ACTION_LIST_ELEMENTS:
		tmp = new char[1024];
		sprintf(tmp, "LIST ELEMENTS");
		break;
	case ACTION_LIST_GROUP:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST GROUP %s", element);
		break;
	case ACTION_LIST_GROUPS:
		tmp = new char[1024];
		sprintf(tmp, "LIST GROUPS");
		break;
	case ACTION_LIST_VARS:
		tmp = new char[1024];
		sprintf(tmp, "LIST VARS");
		break;
	case ACTION_LIST_INTERACTIONS:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST INTERACTIONS %s", element);
		break;
	case ACTION_LIST_ELEMENTGROUPS:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST ELEMENTGROUPS %s", element);
		break;
	case ACTION_LIST_DIETOS:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST DIETOS %s", element);
		break;
	case ACTION_LIST_TRIGGERS:
		tmp = new char[1024];
		sprintf(tmp, "LIST TRIGGERS");
		break;
	case ACTION_LIST_ACTIONS:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST ACTIONS %s", element);
		break;
	case ACTION_LIST_TIMERS:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST TIMERS");
		break;
	case ACTION_LIST_TRIGGEREXECS:
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "LIST TRIGGEREXECS");
		break;
	}
	return tmp;
}

ActionList::~ActionList()
{
	return;
}

void ActionMessage::exec()
{
	char *pos = (messagestring + strlen(messagestring));
	static Element *e;
	int i;
	switch (function)
	{
	case MESSAGE_CLEAR:
		messagestring[0] = 0;
		break;
	case MESSAGE_ADDTEXT:
		strcpy(pos, message);
		break;
	case MESSAGE_ADDNUMBER:
		char t[256];
		sprintf(t, "%i", varint->val());
		strcpy(pos, t);
		break;
	case MESSAGE_ADDELEMENT:
		e = getElement(varint->val());
		if (e)
			strcpy(pos, e->name);
		break;
	case MESSAGE_ADDELEMENTTEXT:
		e = getElement(varint->val());
		if ((e) && e->icon && !strcmp(e->icon->type, "TEXT") && e->icon->text)
			strcpy(pos, e->icon->text);
		break;
	case MESSAGE_ADDGROUP:
		strcpy(pos, getGroup(varint->val())->name);
		break;
	case MESSAGE_SEND:
		if (varint->ok)
			print(messagestring, varint->val());
		else
			print(messagestring, owner);
		break;
	case MESSAGE_SAVE:
		savefile.write(messagestring, strlen(messagestring));
		savefile.write("\n", 1);
		break;
	case MESSAGE_EXEC:
		if (varint->ok)
			parseline(messagestring, 1, varint->val(), "MESSAGE");
		else
			parseline(messagestring, 1, owner, "MESSAGE");
		break;
	case MESSAGE_SENDTEXT:
		if (varint->ok)
			print(message, varint->val());
		else
			print(message, owner);
		break;
	case MESSAGE_BS1:
		messagebox(message, "Message");
		break;
	case MESSAGE_MESSAGEBOX:
		messagebox(messagestring, "Message");
		break;
	case MESSAGE_ADDLINE:
		strcpy(pos, "\n");
		break;
	case MESSAGE_SYSTEM:
#ifdef COMPILER_WINDOWS
		if (yesnobox(messagestring, "Run Command?"))
		{
			std::ofstream batchfile;
			batchfile.open(checkfilename("tmp.cmd"), std::ios::out);
			batchfile.write(messagestring, strlen(messagestring));
			batchfile.close();
			ossystem("tmp.cmd", 0);
			remove("tmp.cmd");
		}
#endif
		break;
	case MESSAGE_SAVESTRING:
		i = varint->val();
		if (strings[i])
			delete strings[i];
		strings[i] = new char[strlen(messagestring) + 1];
		strcpy(strings[i], messagestring);
		break;
	case MESSAGE_ADDSTRING:
		i = varint->val();
		if (strings[i])
			strcpy(pos, strings[i]);
		break;
	}
}

char *ActionMessage::toString()
{
	char *tmp = 0;
	switch (function)
	{
	case MESSAGE_CLEAR:
		tmp = new char[1024];
		sprintf(tmp, "MESSAGE CLEAR");
		break;
	case MESSAGE_ADDTEXT:
		tmp = new char[1024 + strlen(message)];
		sprintf(tmp, "MESSAGE ADDTEXT \"%s\"", message);
		break;
	case MESSAGE_ADDNUMBER:
		tmp = new char[1024 + strlen(varint->text)];
		sprintf(tmp, "MESSAGE ADDNUMBER %s", varint->text);
		break;
	case MESSAGE_ADDELEMENT:
		tmp = new char[1024 + strlen(varint->text)];
		sprintf(tmp, "MESSAGE ADDELENEMT %s", varint->text);
		break;
	case MESSAGE_ADDSTRING:
		tmp = new char[1024 + strlen(varint->text)];
		sprintf(tmp, "MESSAGE ADDSTRING %s", varint->text);
		break;
	case MESSAGE_SAVESTRING:
		tmp = new char[1024 + strlen(varint->text)];
		sprintf(tmp, "MESSAGE SAVESTRING %s", varint->text);
		break;
	case MESSAGE_ADDGROUP:
		tmp = new char[1024 + strlen(varint->text)];
		sprintf(tmp, "MESSAGE ADDGROUP %s", varint->text);
		break;
	case MESSAGE_SEND:
		if (varint->ok)
		{
			tmp = new char[1024 + strlen(varint->text)];
			sprintf(tmp, "MESSAGE SEND %s", varint->text);
		}
		else
		{
			tmp = new char[1024];
			sprintf(tmp, "MESSAGE SEND");
		}
		break;
	case MESSAGE_SAVE:
		tmp = new char[1024];
		sprintf(tmp, "MESSAGE SAVE");
		break;
	case MESSAGE_EXEC:
		if (varint->ok)
		{
			tmp = new char[1024 + strlen(varint->text)];
			sprintf(tmp, "MESSAGE EXEC %s", varint->text);
		}
		else
		{
			tmp = new char[1024];
			sprintf(tmp, "MESSAGE EXEC");
		}
		break;
	case MESSAGE_SENDTEXT:
		if (varint->ok)
		{
			tmp = new char[1024 + strlen(message) + strlen(varint->text)];
			sprintf(tmp, "MESSAGE SENDTEXT \"%s\" %s", message, varint->text);
		}
		else
		{
			tmp = new char[1024 + strlen(message)];
			sprintf(tmp, "MESSAGE SENDTEXT \"%s\"", message);
		}
		break;
	case MESSAGE_BS1:
		tmp = new char[1024 + strlen(message)];
		sprintf(tmp, "Message \"%s\"", message);
		break;
	case MESSAGE_MESSAGEBOX:
		tmp = new char[1024];
		sprintf(tmp, "MESSAGE MESSAGEBOX");
		break;
	case MESSAGE_ADDLINE:
		tmp = new char[1024];
		sprintf(tmp, "MESSAGE ADDLINE");
	case MESSAGE_SYSTEM:
		tmp = new char[1024];
		sprintf(tmp, "MESSAGE SYSTEM");
		break;
	}
	return tmp;
}

ActionMessage::~ActionMessage()
{
	if (function == MESSAGE_SENDTEXT)
		delete (message);
	delete (varint);
}

void ActionInclude::exec()
{
	if (!strcmp(filename, "CLIPBOARD"))
	{
		char *text;
		if (text = getStringFromClipboard())
		{
			if (strstr(text, "http://") == text)
			{
				checkfile(text, false);
				parsefile(text, owner);
			}
			else
			{
				parsechar(text, owner, filename);
			}
		}
	}
	else if (!strcmp(filename, "FILEDIALOG"))
	{
		char *text;
		if (param)
		{
			char *tmp = new char[strlen(param) + 2];
			strcpy(tmp, param);
			tmp[strlen(tmp) - 1] = 0;
			for (unsigned int i = 0; i < strlen(param); i++)
				if (tmp[i] == '|')
					tmp[i] = 0;
			if (text = opendialog(tmp, 0))
			{
				checkfile(text, false);
				parsefile(text, owner);
			}
			delete (tmp);
		}
		else if (text = opendialog("BS2 File\0*.bs2\0BS1 File\0*.cfg\0TXT File\0*.txt\0All\0*.*\0", 0))
		{
			checkfile(text, false);
			parsefile(text, owner);
		}
	}
	else
	{
		char *t = messageReplace(filename);
		checkfile(t, true);
		parsefile(t, owner);
	}
}

char *ActionInclude::toString()
{
	char *tmp = new char[1024 + strlen(filename)];
	sprintf(tmp, "INCLUDE \"%s\"", filename);
	return tmp;
}

ActionInclude::~ActionInclude()
{
	delete (filename);
}

void ActionWind::exec()
{
	Wind *w = new Wind();
	w->x = 240;
	w->y = 240;
	w->angle = (float)3.141592654;
	winds.push_back(w);
}

char *ActionWind::toString()
{
	char *tmp = new char[1];
	tmp[0] = 0;
	return tmp;
}

void ActionNoBias::exec()
{
	setElementBias(e->val(), true);
}

char *ActionNoBias::toString()
{
	char *tmp = new char[1024 + strlen(e->text)];
	sprintf(tmp, "NOBIAS %s", e->text);
	return tmp;
}

ActionNoBias::~ActionNoBias()
{
	delete (e);
}

void ActionMenu::exec()
{
	if (action == ACTION_MENU_CLEAR)
		clearMenuBar(bar);
	redrawmenu(3);
}

char *ActionMenu::toString()
{
	char *tmp = new char[1024];
	if (action == ACTION_MENU_CLEAR)
	{
		char *w = 0;
		if (bar == MENU_BAR_TOP)
			w = "TOP";
		if (bar == MENU_BAR_LEFT)
			w = "LEFT";
		if (bar == MENU_BAR_RIGHT)
			w = "RIGHT";
		if (bar == MENU_BAR_BOTTOM)
			w = "BOTTOM";
		if (bar == MENU_BAR_SUB)
			w = "SUB";
		sprintf(tmp, "MENU %s CLEAR", w);
	}
	else
	{
		sprintf(tmp, "MENU REFRESH");
	}
	return tmp;
}

ActionMenu::~ActionMenu()
{
}

void ActionSubMenu::exec()
{
	if (function)
	{
		hideSubMenu();
	}
	else
	{
		if ((x->ok) && (y->ok))
			showSubMenu(stay, align, x->val(), y->val());
		else
			showSubMenu(stay, align);
	}
}

char *ActionSubMenu::toString()
{
	char *tmp = new char[1024];
	if (function)
	{
		sprintf(tmp, "SUBMENU CLOSE");
	}
	else
	{
		if (stay)
		{
			if ((x->ok) && (y->ok))
				sprintf(tmp, "SUBMENU %s %s STAY", x->text, y->text);
			else
				sprintf(tmp, "SUBMENU STAY");
		}
		else
		{
			if ((x->ok) && (y->ok))
				sprintf(tmp, "SUBMENU %s %s", x->text, y->text);
			else
				sprintf(tmp, "SUBMENU");
		}
	}
	return tmp;
}

ActionSubMenu::~ActionSubMenu()
{
}

void frametimer()
{
	static Var *vt = (Var *)setVar("FRAME", 0);
	vt->value = thisframe;
	if (debugframe->value)
	{
		char tmp[512];
		sprintf(tmp, "frame: %i timers: %i", thisframe, frametriggers.size());
		std::cout << tmp << std::endl;
	}
	timercleared = false;
	if (frametriggers.size())
	{
		Timer *t = frametriggers.top();
		while (!frametriggers.empty() && (t->value <= thisframe))
		{
			if (t->params)
				addparams(t->params);
			t->trigger->exec();
			if (t->params)
				removeparams();
			if (timercleared)
			{
				if (frametriggers.size() == 0)
					return;
				if (t == frametriggers.top())
				{
					timerStack.push(t);
					frametriggers.pop();
				}
			}
			else
			{
				timerStack.push(t);
				frametriggers.pop();
			}
			t = frametriggers.top();
		}
	}
	thisframe++;
}

void msectimer()
{
	static unsigned int startsec = SDL_GetTicks();
	static Var *vms = (Var *)setVar("MSEC", 0);
	vms->value = SDL_GetTicks() - startsec;
}
