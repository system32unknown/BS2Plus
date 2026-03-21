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

std::list<Trigger*> triggers;

struct mycomparison {
	bool operator()(const Timer* lhs, const Timer* rhs) {
		return lhs->value > rhs->value;
	}
};

std::priority_queue<Timer*, std::vector<Timer*>, mycomparison> frametriggers;
std::list<Timer*> msecriggers;

std::stack<Timer*> timerStack;

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

Var* debugframe;
int rec = -1;
char* strings[MAX_STRINGS];

char* messageReplace(char* text) {
	if (!strcmp(text, "MESSAGE")) {
		return messagestring;
	} else return text;
}

void deleteparams(Varint** v) {
	if (v == nullptr) return;
	for (int i = 0; i < 10; i++)
		delete v[i];
	delete[] v;
}

void Action::exec() {}
Action::~Action() {}

char* Action::toString() {
	char* tmp = new char[1024];
	sprintf(tmp, "NO ACTION");
	return tmp;
}

Trigger* findTrigger(char* triggername, int owner) {
	if (!triggername) return nullptr;

	char* name = messageReplace(triggername);

	if (name[0] == '<') {
		name[strlen(name) - 1] = '\0';
		Token tokens(name + 1);
		Action* action = parseaction(&tokens, owner);
		if (!action) return nullptr;
		Trigger* t = new Trigger;
		t->execcount = 0;
		t->deleted = 0;
		t->name = "NO NAME";
		t->actions.push_back(action);
		t->actioncount = t->actions.size();
		return t;
	}

	for (auto* t : triggers)
		if (!strcmp(t->name, name)) return t;

	Trigger* t = new Trigger;
	t->deleted = 0;
	t->execcount = 0;
	t->actioncount = 0;
	t->name = new char[strlen(name) + 1];
	strcpy(t->name, name);
	triggers.push_back(t);
	return t;
}

void addAction(char* trigger, Action* action, int owner) {
	Trigger* t = findTrigger(trigger, owner);
	t->actions.push_back(action);
	t->actioncount = t->actions.size();
}

int Trigger::exec() {
	execcount++;
	if (!actioncount) return 0;

	static Var* debugtrigger = (Var*)setVar("DEBUGTRIGGER", 0);
	static Var* debugactions = (Var*)setVar("DEBUGACTION", 0);

	setreturn = false;
	rec++;

	if (debugtrigger->value) {
		static char tmp[512];
		if (name)
			snprintf(tmp, sizeof(tmp), "executing trigger: %s actions: %zu", name, actions.size());
		else snprintf(tmp, sizeof(tmp), "executing trigger: noname, actions: %zu", actions.size());
		if (rec < 0) rec = 0;
		for (int i = 0; i < rec; i++) std::cout << "  ";
		std::cout << tmp << std::endl;
	}

	if (actioncount == 1) {
		if (debugactions->value) {
			char* t = (*actions.begin())->toString();
			char tmp[512];
			snprintf(tmp, sizeof(tmp), "executing action: messagid: %i action: ", (*actions.begin())->owner);
			for (int i = 0; i <= rec; i++) std::cout << "  ";
			std::cout << tmp << t << std::endl;
			delete[] t;
		}
		(*actions.begin())->exec();
		rec--;
		return setreturn ? globalreturn : 0;
	}

	const long thisexecounter = ++execcounter;
	auto it = actions.begin();
	while (it != actions.end() && thisexecounter > deleted) {
		if (debugactions->value) {
			char* t = (*it)->toString();
			char tmp[512];
			snprintf(tmp, sizeof(tmp), "executing action: messagid: %i action: ", (*it)->owner);
			for (int i = 0; i <= rec; i++) std::cout << "  ";
			std::cout << tmp << t << std::endl;
			delete[] t;
		}
		(*it)->exec();
		if (setreturn) return globalreturn;
		if (actions.empty()) {
			rec--;
			return 0;
		}
		it++;
	}
	rec--;
	return 0;
}

void ActionReturn::exec() {
	globalreturn = var->val();
	setreturn = true;
}

char* ActionReturn::toString() {
	const size_t len = 1024 + strlen(var->text);
	char* tmp = new char[len];
	snprintf(tmp, len, "RETURN %s", var->text);
	return tmp;
}

ActionReturn::~ActionReturn() {
	delete (var);
}

void ActionExit::exec() {
	if (threadrunning) {
		threadrunning = false;
		while (!threadrunning);
	}
	exit(0);
}

char* ActionExit::toString() {
	char* tmp = new char[1024];
	sprintf(tmp, "EXIT");
	return tmp;
}

void ActionRestart::exec() {
#ifdef COMPILER_SYSTEM
	if (parameter) {
		ossystem("BS2CE.exe", parameter);
	} else ossystem("BS2CE.exe", "");
#endif
	if (threadrunning) {
		threadrunning = false;
		while (!threadrunning);
	}
	exit(0);
}

char* ActionRestart::toString() {
	char* tmp = new char[1024 + strlen(parameter)];
	if (parameter)
		sprintf(tmp, "RESTART \"%s\"", parameter);
	else
		sprintf(tmp, "RESTART");
	return tmp;
}

void ActionSetVar::exec() {
	if (!t) {
		if (!strchr(var, '[')) {
			static Var* v;
			v = (Var*)setVar(var, 0, false);
			if (v)
				t = &(v->value);
			else
				t = (int*)1;
		} else
			t = (int*)1;
	}
	static Var* messageid = (Var*)setVar("MESSAGEID", 0);
	messageid->value = owner;
	if (t == (int*)1)
		setVar(var, value->val());
	else
		*t = value->val();
}

char* ActionSetVar::toString() {
	char* tmp = new char[1024 + strlen(var) + strlen(value->text)];
	sprintf(tmp, "SET \"%s\" %s", var, value->text);
	return tmp;
}

ActionSetVar::~ActionSetVar() {
	delete (var);
	delete (value);
}

void ActionInc::exec() {
	if (!t) {
		if (!strchr(var, '[')) {
			static Var* v;
			v = (Var*)setVar(var, 0, false);
			if (v)
				t = &(v->value);
			else
				t = (int*)1;
		} else
			t = (int*)1;
	}
	static Var* messageid = (Var*)setVar("MESSAGEID", 0);
	messageid->value = owner;
	if (t == (int*)1)
		setVar(var, value->val() + 1);
	else
		(*t)++;
}

char* ActionInc::toString() {
	const size_t len = 1024 + strlen(var);
	char* tmp = new char[len];
	snprintf(tmp, len, "INC \"%s\"", var);
	return tmp;
}

ActionInc::~ActionInc() {
	delete[] var;
	delete value;
}

void ActionCount::exec() {
	SDL_Surface* screen = getRealSandSurface();
	const int e = element->val();
	int i = 0;

	if (x) {
		const int starty = y->val();
		const int startx = x->val();
		const int endy = starty + h->val();
		const int endx = startx + w->val();
		for (int row = starty; row < endy; row++) {
			const Uint16* row_ptr = static_cast<Uint16*>(screen->pixels) + row * screen->pitch / 2;
			for (int col = startx; col < endx; col++)
				if (*(row_ptr + col) == e) i++;
		}
	} else {
		const Uint16* end = static_cast<Uint16*>(screen->pixels) + screen->pitch / 2 * (screen->h - 2) + (screen->w - 1) - 1;
		const Uint16* t = static_cast<Uint16*>(screen->pixels) - 1 + screen->pitch / 2;
		while (end != ++t) if (*t == e) i++;
	}
	setVar(var, i);
}

char* ActionCount::toString() {
	const size_t len = 1024 + strlen(var) + strlen(element->text);
	char* tmp = new char[len];
	snprintf(tmp, len, "COUNT \"%s\" %s", var, element->text);
	return tmp;
}

ActionCount::~ActionCount() {
	delete[] var;
	delete element;
	delete x;
	delete y;
	delete w;
	delete h;
}

void ActionClosest::exec() {
	static Var* varx = reinterpret_cast<Var*>(setVar("CLOSESTX", 0));
	static Var* vary = reinterpret_cast<Var*>(setVar("CLOSESTY", 0));
	static Var* vard = reinterpret_cast<Var*>(setVar("CLOSESTD", 0));

	SDL_Surface* screen = getRealSandSurface();
	const int ne = e->val();

	if (!used[ne]) {
		varx->value = -1;
		vary->value = -1;
		vard->value = static_cast<int>(sqrt(1000000000.0));
		return;
	}

	const int w = screen->w - 2;
	const int h = screen->h - 2;
	const int nx = x->val();
	const int ny = y->val();
	int nd = d->val();
	int dist = 1000000000;

	for (int i = 0; i < 5; i++) {
		if (lx[i] < static_cast<unsigned int>(w) &&
			ly[i] < static_cast<unsigned int>(h) &&
			*(static_cast<Uint16*>(screen->pixels) + ly[i] * screen->pitch / 2 + lx[i]) == ne) {
			const int tmp = static_cast<int>(sqrt(static_cast<double>((nx - lx[i]) * (nx - lx[i]) + (ny - ly[i]) * (ny - ly[i])))) + 2;
			if (tmp < nd) nd = tmp;
		}
	}

	int lastx = -1;
	int lasty = -1;
	int d2 = (32 > nd) ? nd : 32;

	while (d2 <= nd &&
		d2 <= static_cast<int>(sqrt(static_cast<double>(dist))) + d2 &&
		dist != 999999999) {
		int startx = nx - d2;
		int endx = nx + d2;
		int starty = ny - d2;
		int endy = ny + d2;

		if (startx < 1 && endx > w && starty < 1 && endy > h) {
			dist = 999999999;
		} else {
			if (startx < 1) startx = 1;
			if (endx > w) endx = w;
			if (starty < 1) starty = 1;
			if (endy > h) endy = h;

			for (int row = starty; row <= endy; row++) {
				const Uint16* rowp = static_cast<Uint16*>(screen->pixels) + row * screen->pitch / 2;
				for (int col = startx; col <= endx; col++) {
					if (*(rowp + col) == ne) {
						const int dsq = (col - nx) * (col - nx) + (row - ny) * (row - ny);
						if (dsq < dist) {
							dist = dsq;
							lastx = col;
							lasty = row;
						}
					}
				}
			}
		}

		if (d2 == nd) break;
		d2 = (d2 * 2 > nd) ? nd : d2 * 2;
	}

	lx[li % 5] = varx->value = lastx;
	ly[li % 5] = vary->value = lasty;
	li++;
	vard->value = static_cast<int>(sqrt(static_cast<double>(dist)));
}

char* ActionClosest::toString() {
	char* tmp = new char[1024 + strlen(e->text) + strlen(x->text) + strlen(y->text)];
	sprintf(tmp, "CLOSEST %s %s %s", e->text, x->text, y->text);
	return tmp;
}

ActionClosest::~ActionClosest() {
	delete (x);
	delete (y);
	delete (e);
	delete (d);
}

void ActionGetVar::exec() {
	static Var* messageid = (Var*)setVar("MESSAGEID", 0);
	messageid->value = owner;
	char out[256];
	int i;
	int t = getVar(value, &i);
	if (t == 7)
		sprintf(out, "[%s] %s", value, (char*)i);
	else if (t)
		sprintf(out, "[%s] %i", value, i);
	else
		sprintf(out, "[%s] 0", value);
	print(out, owner);
}

char* ActionGetVar::toString() {
	char* tmp = new char[1024 + strlen(value)];
	sprintf(tmp, "GET \"%s\"", value);
	return tmp;
}

ActionGetVar::~ActionGetVar() {
	delete (value);
}

void ActionGetFile::exec() {
	std::ifstream file(checkfilename(filename), std::ios::in | std::ios::ate | std::ios::binary);
	if (!file) {
		char tmp[10000];
		snprintf(tmp, sizeof(tmp), "Error opening file \"%s\"", filename);
		error(tmp, owner);
		return;
	}

	const long size = (long)file.tellg();
	file.seekg(0, std::ios::beg);

	char* data = new char[size + 2];
	file.read(data, size);
	data[size] = '\0';
	file.close();

	char out[256];
	std::snprintf(out, sizeof(out), "{%s} %i", filename, (int)size);
	print(out, owner);
	print(data, owner, (int)size);
	delete[] data;
}

char* ActionGetFile::toString() {
	char* tmp = new char[1024 + strlen(filename)];
	sprintf(tmp, "GETFILE \"%s\"", filename);
	return tmp;
}

ActionGetFile::~ActionGetFile() {
	delete (filename);
}

void ActionResize::exec() {
	resize(w->val(), h->val());
}

char* ActionResize::toString() {
	char* tmp = new char[1024 + strlen(w->text) + strlen(h->text)];
	sprintf(tmp, "GETFILE %s %s", w->text, h->text);
	return tmp;
}

ActionResize::~ActionResize() {
	delete (w);
	delete (h);
}

void ActionScroll::exec() {
	scrollto(x->val(), y->val());
}

char* ActionScroll::toString() {
	char* tmp = new char[1024 + strlen(x->text) + strlen(y->text)];
	sprintf(tmp, "SCROLL %s %s", x->text, y->text);
	return tmp;
}

ActionScroll::~ActionScroll() {
	delete (x);
	delete (y);
}

char* ActionWhile::toString() {
	char* tmp = new char[1024 + strlen(value->text) + strlen(trigger->name)];
	sprintf(tmp, "WHILE %s \"%s\"", value->text, trigger->name);
	return tmp;
}

ActionWhile::~ActionWhile() {
	delete (value);
	deleteparams(params);
}

void ActionFor::exec() {
	Var* v = (Var*)setVar(value, 0);
	const int end = tovalue->val();
	const int s = step->val();

	if (s <= 0) return;

	if (function == FOR_TO) {
		for (v->value = fromvalue->val(); v->value <= end; v->value += s) {
			if (params) addparams(calcparams(params));
			trigger->exec();
			if (params) removeparams();
		}
	} else {
		for (v->value = fromvalue->val(); v->value >= end; v->value -= s) {
			if (params) addparams(calcparams(params));
			trigger->exec();
			if (params) removeparams();
		}
	}
}

char* ActionFor::toString() {
	char* tmp = new char[1024 + strlen(value) + strlen(fromvalue->text) + strlen(tovalue->text) + strlen(trigger->name)];
	if (function == FOR_TO)
		sprintf(tmp, "FOR \"%s\" FROM %s TO %s DO \"%s\"", value, fromvalue->text, tovalue->text, trigger->name);
	else
		sprintf(tmp, "FOR \"%s\" FROM %s DOWNTO %s DO \"%s\"", value, fromvalue->text, tovalue->text, trigger->name);
	return tmp;
}

ActionFor::~ActionFor() {
	delete (value);
	delete (fromvalue);
	delete (tovalue);
	deleteparams(params);
}

void ActionForEach::exec() {
	static Var* varx = (Var*)setVar("FOREACHX", 0);
	static Var* vary = (Var*)setVar("FOREACHY", 0);
	SDL_Surface* screen = getRealSandSurface();
	int w = screen->w - 2;
	int h = screen->h - 2;
	int e = element->val();
	Uint16* t;
	for (int y = 1; y <= h; y++) {
		t = ((Uint16*)screen->pixels + (y)*screen->pitch / 2);
		for (int x = 1; x <= w; x++)
			if (*(t + x) == e) {
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

char* ActionForEach::toString() {
	char* tmp = new char[1024 + strlen(element->text) + strlen(trigger->name)];
	sprintf(tmp, "FOR EACH %s DO \"%s\"", element->text, trigger->name);
	return tmp;
}

ActionForEach::~ActionForEach() {
	deleteparams(params);
}

void ActionSystem::exec() {
#ifdef COMPILER_WINDOWS
	if (yesnobox(cmd, "Run Command?")) {
		std::ofstream batchfile2;
		batchfile2.open(checkfilename("tmp.cmd"), std::ios::out);
		batchfile2.write(cmd, strlen(cmd));
		batchfile2.close();
		ossystem("@tmp.cmd", 0, true);
		remove("tmp.cmd");
	}
#endif
}

char* ActionSystem::toString() {
	char* tmp = new char[1024 + strlen(cmd)];
	snprintf(tmp, sizeof(tmp), "SYSTEM \"%s\"", cmd);
	return tmp;
}

ActionSystem::~ActionSystem() {
	delete (cmd);
	return;
}

char* ActionIf::toString() {
	char* tmp;
	if (elsetrigger) {
		tmp = new char[1024 + strlen(value->text) + strlen(trigger->name) + strlen(elsetrigger->name)];
		sprintf(tmp, "IF %s \"%s\" ELSE \"%s\"", value->text, trigger->name, elsetrigger->name);
	} else {
		tmp = new char[1024 + strlen(value->text) + strlen(trigger->name)];
		sprintf(tmp, "IF %s \"%s\"", value->text, trigger->name);
	}
	return tmp;
}

ActionIf::~ActionIf() {
	delete (value);
	deleteparams(params);
}

void ActionRemoveTrigger::exec() {
	Trigger* t = findTrigger(trigger, owner);
	t->actions.clear();
	t->deleted = execcounter;
	t->actioncount = 0;
}

char* ActionRemoveTrigger::toString() {
	char* tmp = new char[1024 + strlen(trigger)];
	sprintf(tmp, "REMOVETRIGGER \"%s\"", trigger);
	return tmp;
}

ActionRemoveTrigger::~ActionRemoveTrigger() {
	delete (trigger);
}

char* ActionExec::toString() {
	char* tmp = new char[1024 + strlen(trigger->name)];
	sprintf(tmp, "EXEC \"%s\"", trigger->name);
	return tmp;
}

ActionExec::~ActionExec() {
	deleteparams(params);
}

void ActionElement::exec() {
	Uint16 e = findElement(elementname, true);
	switch (attribute) {
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

char* ActionElement::toString() {
	char* tmp = new char[1024];
	sprintf(tmp, "ELEMENT");
	return tmp;
}

ActionElement::~ActionElement() {
	delete (elementname);
	delete (value1);
	delete (value2);
	delete (value3);
}

void ActionElementBS1::exec() {
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

char* ActionElementBS1::toString() {
	char* icontext = icon->toString();
	char* tmp = new char[1024 + strlen(elementname) + strlen(group) + strlen(rvalue->text) + strlen(gvalue->text) + strlen(bvalue->text) + strlen(weight->text) + strlen(spay->text) + strlen(slide->text) + strlen(viscousity->text) + strlen(dierate->text) + strlen(dieto) + strlen(menuorder->text) + strlen(icontext)];
	sprintf(tmp, "ELEMENT \"%s\" \"%s\" %s %s %s %s %s %s %s %s \"%s\" %s %s", elementname, group,
		rvalue->text, gvalue->text, bvalue->text, weight->text, spay->text,
		slide->text, viscousity->text, dierate->text, dieto, menuorder->text, icontext);
	return tmp;
}

ActionElementBS1::~ActionElementBS1() {
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

void ActionElementDie::exec() {
	addDie(findElement(elementname, true), findElement(dieto, true), rate->val());
}

char* ActionElementDie::toString() {
	char* tmp = new char[1024 + strlen(elementname) + strlen(dieto) + strlen(rate->text)];
	snprintf(tmp, sizeof(tmp), "ELEMENT \"%s\" DIE \"%s\" %s", elementname, dieto, rate->text);
	return tmp;
}

ActionElementDie::~ActionElementDie() {
	delete (elementname);
	delete (dieto);
	delete (rate);
}

void ActionInteraction::exec() {
	Uint16 ex = 0;
	Group* g = nullptr;
	Group* g2 = nullptr;

	std::list<Uint16> e1s, e2s, e3s, e4s;
	std::list<int> rs;

	auto isGroup = [](const char* s) {
		return s && strstr(s, "GROUP:") == s;
		};

	for (auto* s : elements1) {
		if (isGroup(s)) {
			g = getGroup(findGroup(s + 6, false, -1));
			for (auto v : g->elements) e1s.push_front(static_cast<Uint16>(v));
		} else e1s.push_back(findElement(s, true));
	}

	for (auto* s : elements2) {
		if (isGroup(s)) {
			g = getGroup(findGroup(s + 6, false, -1));
			for (auto v : g->elements) e2s.push_front(static_cast<Uint16>(v));
		} else e2s.push_back(findElement(s, true));
	}

	auto i2 = toothers.begin();
	for (auto* self : toselfs) {
		if (isGroup(self)) {
			g = getGroup(findGroup(self + 6, false, -1));
			for (auto ev : g->elements) {
				if (isGroup(*i2)) {
					if (!g2) g2 = getGroup(findGroup(*i2 + 6, false, -1));
					for (auto ev2 : g2->elements) {
						e3s.push_back(static_cast<Uint16>(ev));
						e4s.push_back(static_cast<Uint16>(ev2));
					}
				} else {
					e3s.push_back(static_cast<Uint16>(ev));
					e4s.push_back(findElement(*i2, true));
				}
			}
		} else if (isGroup(*i2)) {
			g = getGroup(findGroup(*i2 + 6, false, -1));
			for (auto ev2 : g->elements) {
				e3s.push_back(findElement(self));
				e4s.push_back(static_cast<Uint16>(ev2));
			}
		} else {
			e3s.push_back(findElement(self, true));
			e4s.push_back(findElement(*i2, true));
		}
		++i2;
		e3s.push_back(32767);
		e4s.push_back(32767);
	}

	int restrate = 32768;
	for (auto* r : rates) {
		const int tmp = r->val();
		const int tmp2 = (restrate > 0) ? std::min(tmp * 32768 / restrate, 32768) : 0;
		rs.push_back(tmp2);
		restrate -= tmp;
	}

	ex = except ? findElement(except, true) : 1;

	for (const auto e1 : e1s) {
		for (const auto e2 : e2s) {
			auto it = rs.begin();
			auto e4 = e4s.begin();
			for (const auto e3 : e3s) {
				if (e3 == 32767 && *e4 == 32767) ++it;
				else addInteraction(e1, e2, e3, *e4, *it, ex, nullptr, at->val());
				++e4;
			}
			for (auto* trig : triggers) {
				addInteraction(e1, e2, 0, 0, *it, ex, trig, at->val());
				++it;
			}
		}
	}
}

char* ActionInteraction::toString() {
	char* e;
	if (except) {
		const size_t elen = 128 + strlen(except);
		e = new char[elen];
		snprintf(e, elen, " \"%s\"", except);
	} else e = "";

	char* a;
	if (at->val() != -1) {
		const size_t alen = 128 + strlen(at->text);
		a = new char[alen];
		snprintf(a, alen, "AT %s", at->text);
	} else a = "";

	auto i4 = elements1.begin();
	auto i5 = elements2.begin();

	char* tmp;

	if (triggers.empty()) {
		auto i1 = toselfs.begin();
		auto i2 = toothers.begin();
		auto i3 = rates.begin();
		size_t len = 0;
		while (i1 != toselfs.end()) {
			len += strlen(*i1) + strlen(*i2) + strlen((*i3)->text) + 10;
			++i1; ++i2; ++i3;
		}

		const size_t tlen = 1024 + strlen(*i4) + strlen(*i5) + strlen(at->text) + strlen(e) + len;
		tmp = new char[tlen];
		char* tmp2 = new char[tlen];
		snprintf(tmp, tlen, "INTERACTION%s \"%s\" \"%s\"", a, *i4, *i5);

		i1 = toselfs.begin();
		i2 = toothers.begin();
		i3 = rates.begin();
		while (i1 != toselfs.end()) {
			strcpy(tmp2, tmp);
			snprintf(tmp, tlen, "%s \"%s\" \"%s\" %s", tmp2, *i1, *i2, (*i3)->text);
			++i1; ++i2; ++i3;
		}
		strcpy(tmp2, tmp);
		snprintf(tmp, tlen, "%s%s", tmp2, e);
		delete[] tmp2;
	} else {
		auto i1 = triggers.begin();
		auto i3 = rates.begin();
		size_t len = 0;
		while (i1 != triggers.end()) {
			len += strlen((*i1)->name) + strlen((*i3)->text) + 10;
			++i1; ++i3;
		}

		const size_t tlen = 1024 + strlen(*i4) + strlen(*i5) + strlen(at->text) + strlen(e) + len;
		tmp = new char[tlen];
		char* tmp2 = new char[tlen];
		snprintf(tmp, tlen, "INTERACTIONTRIGGER%s \"%s\" \"%s\"", a, *i4, *i5);

		i1 = triggers.begin();
		i3 = rates.begin();
		while (i1 != triggers.end()) {
			strcpy(tmp2, tmp);
			snprintf(tmp, tlen, "%s \"%s\" %s", tmp2, (*i1)->name, (*i3)->text);
			++i1; ++i3;
		}
		strcpy(tmp2, tmp);
		snprintf(tmp, tlen, "%s%s", tmp2, e);
		delete[] tmp2;
	}

	if (except) delete[] e;
	if (at->val() != -1) delete[] a;
	return tmp;
}

ActionInteraction::~ActionInteraction() {
	for (auto* p : elements1) delete[] p;
	for (auto* p : elements2) delete[] p;
	for (auto* p : toselfs) delete[] p;
	for (auto* p : toothers) delete[] p;
	for (auto* p : rates) delete p;
	delete[] except;
	delete at;
}

void ActionRemoveInteraction::exec() {
	if (index == NULL)
		clearInteraction(findElement(element, true));
	else
		removeInteraction(findElement(element, true), index->val());
}

char* ActionRemoveInteraction::toString() {
	char* tmp;
	if (index) {
		tmp = new char[1024 + strlen(element) + strlen(index->text)];
		sprintf(tmp, "INTERACTIONREMOVE \"%s\" %s", element, index->text);
	} else {
		tmp = new char[1024 + strlen(element)];
		sprintf(tmp, "INTERACTIONCLEAR \"%s\"", element);
	}
	return tmp;
}

ActionRemoveInteraction::~ActionRemoveInteraction() {
	delete (element);
	delete (index);
}

void ActionClearDie::exec() {
	clearDie(findElement(element, true));
}

char* ActionClearDie::toString() {
	char* tmp = new char[1024 + strlen(element)];
	sprintf(tmp, "DIECLEAR \"%s\"", element);
	return tmp;
}

ActionClearDie::~ActionClearDie() {
	delete (element);
}

void ActionClearElements::exec() {
	clearelements();
}

char* ActionClearElements::toString() {
	char* tmp = new char[1024];
	sprintf(tmp, "ELEMENTSCLEAR");
	return tmp;
}

void ActionSave::exec() {
	char* t;
	switch (screenid) {
	case ACTION_SAVE_SCREEN_ALL:
		break;
	case ACTION_SAVE_SCREEN_SAND:
		if (!strcmp(filename, "FILEDIALOG")) {
			char* text;
			if (text = savedialog("PNG File\0*.png\0BMP File\0*.bmp\0BS2 Config\0*.bs2\0", 0))
				save(getRealSandSurface(), text);
		} else {
			char* t = messageReplace(filename);
			checkfile(t);
			save(getRealSandSurface(), t);
		}
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		quicksave(((Varint*)filename)->val());
		break;
	case ACTION_SAVE_STAMP:
		t = messageReplace(filename);
		checkfile(t);
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
		std::list<Timer*>::iterator it;
		std::list<Timer*> timerlist;
		timerlist.clear();
		char tmp[4096];
		while (!frametriggers.empty()) {
			Timer* t = frametriggers.top();
			if (t && t->trigger && t->trigger->name) {
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

char* ActionSave::toString() {
	char* tmp = 0;
	switch (screenid) {
	case ACTION_SAVE_SCREEN_SAND:
		tmp = new char[1024];
		sprintf(tmp, "SAVE SAND \"%s\"", filename);
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		tmp = new char[1024];
		sprintf(tmp, "SAVE QUICKSAND \"%s\"", ((Varint*)filename)->text);
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

ActionSave::~ActionSave() {
	delete (id);
}

void ActionFile::exec() {
#ifdef COMPILER_REMOVE
	if (function == ACTION_FILE_DELETE) {
		char* t = messageReplace(filename);
		checkfile(t);
		remove(t);
	}
#endif
	if (function == ACTION_FILE_OPEN) {
		if (!strcmp(filename, "FILEDIALOG")) {
			char* text;
			if (text = savedialog("*.*\0*.*\0", 0)) {
				checkfile(text, false);
				if ((param) && !strcmp(param, "NEW")) {
					remove(checkfilename(text));
				}
				savefile.open(checkfilename(text), std::ios::app | std::ios::out | std::ios::ate | std::ios::binary);
			}
		} else {
			char* t = messageReplace(filename);
			checkfile(t);
			if ((param) && !strcmp(param, "NEW")) {
				remove(checkfilename(filename));
			}
			savefile.open(checkfilename(t), std::ios::app | std::ios::out | std::ios::ate | std::ios::binary);
		}
	}
	if (function == ACTION_FILE_CLOSE)
		savefile.close();
}

char* ActionFile::toString() {
	char* tmp = 0;
	switch (function) {
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

ActionFile::~ActionFile() {
	delete (filename);
}

void ActionLoad::exec() {
#if LOGLEVEL > 7
	char tmp[255];
	tmp[0] = 0;
#endif
	char* t;
	switch (screenid) {
	case ACTION_SAVE_SCREEN_ALL:
		break;
	case ACTION_SAVE_SCREEN_SAND:
		if (!strcmp(filename, "FILEDIALOG")) {
			char* text;
			if (text = opendialog("PNG File\0*.png\0BMP File\0*.bmp\0BS2 Config\0*.bs2\0", nullptr)) {
				checkfile(text, false);
				load(text);
			}
		} else {
			char* t = messageReplace(filename);
			checkfile(t);
			load(t);
		}
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		quickload(((Varint*)filename)->val());
		break;
	case ACTION_SAVE_STAMP:
		t = messageReplace(filename);
		checkfile(t);
		if ((id->val() > 0) && (id->val() < MAX_STAMPS))
			stamps[id->val()] = SDL_LoadBMP(checkfilename(t));
		break;
	case ACTION_SAVE_FGLAYER:
		if (fglayer) SDL_FreeSurface(fglayer);
		fglayer = 0;
		if (!strcmp(filename, "FILEDIALOG")) {
			char* text;
			if (text = opendialog("BMP File\0*.bmp\0", 0)) {
				checkfile(text, false);
				fglayer = SDL_LoadBMP(checkfilename(text));
			}
		} else {
			checkfile(filename);
			fglayer = SDL_LoadBMP(checkfilename(filename));
		}
		break;
	case ACTION_SAVE_BGLAYER:
		if (bglayer) SDL_FreeSurface(bglayer);
		bglayer = 0;
		if (!strcmp(filename, "FILEDIALOG")) {
			char* text;
			if (text = opendialog("BMP File\0*.bmp\0", 0)) {
				checkfile(text, false);
				bglayer = SDL_LoadBMP(checkfilename(text));
			}
		} else {
			checkfile(filename);
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

char* ActionLoad::toString() {
	char* tmp = nullptr;
	switch (screenid) {
	case ACTION_SAVE_SCREEN_SAND:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD SAND \"%s\"", filename);
		break;
	case ACTION_QUICKSAVE_SCREEN_SAND:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD QUICKSAND \"%s\"", reinterpret_cast<Varint*>(filename)->text);
		break;
	case ACTION_SAVE_STAMP:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD STAMP \"%s\" %s", filename, id->text);
		break;
	case ACTION_SAVE_FGLAYER:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD FGLAYER \"%s\"", filename);
		break;
	case ACTION_SAVE_BGLAYER:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD BGLAYER \"%s\"", filename);
		break;
	case ACTION_SAVE_FONT:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD FONT \"%s\"", filename);
		break;
	case ACTION_SAVE_MENUFONT:
		tmp = new char[1024];
		snprintf(tmp, 1024, "LOAD MENUFONT \"%s\" %s", filename, id->text);
		break;
	}
	return tmp;
}

ActionLoad::~ActionLoad() {
	delete id;
}
void ActionButton::exec() {
	Button* btn = new Button((Button*)button);

	if (tiptype) {
		int i = 0;
		getVar(btn->tiptext, &i);

		if (tiptype == ACTION_BUTTON_TIPTYPE_ELEMENT) btn->tiptext = getElement(i)->name;
		else if (tiptype == ACTION_BUTTON_TIPTYPE_GROUP) btn->tiptext = getGroup(i)->name;
	}

	if (btn->tiptext && !strcmp(btn->tiptext, "MESSAGE")) {
		delete[] btn->tiptext;
		btn->tiptext = new char[strlen(messagestring) + 1];
		std::strcpy(btn->tiptext, messagestring);
	}

	if (btn->icon->type && (!strcmp(btn->icon->type, "TEXT") || !strcmp(btn->icon->type, "Text")) && btn->icon->text && !strcmp(btn->icon->text, "MESSAGE")) {
		std::strcpy(btn->icon->text, messagestring);
	}

	btn->border = r && g && b;
	if (r) btn->r = r->val();
	if (g) btn->g = g->val();
	if (b) btn->b = b->val();

	btn->params = calcparams(params);
	btn->icon->calc();
	addButtonToMenuBar(bar, btn);
	redrawmenu(3);
}

char* ActionButton::toString() {
	char* w = 0;
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
	char* a = "ADD";
	if (((Button*)button)->mode)
		a = "ADDDRAG";
	if (r)
		a = "ADDBORDER";
	char* tt = 0;
	char* ttt = "";
	if (tiptype == ACTION_BUTTON_TIPTYPE_TEXT) {
		tt = "TEXT";
		ttt = "\"";
	}
	if (tiptype == ACTION_BUTTON_TIPTYPE_ELEMENT) {
		tt = "ELEMENT";
	}
	if (tiptype == ACTION_BUTTON_TIPTYPE_GROUP) {
		tt = "GROUP";
	}
	char* tmp = new char[1024];
	char* i = ((Button*)button)->icon->toString();
	sprintf(tmp, "MENU %s %s %s %s%s%s %s", w, a, tt, ttt, ((Button*)button)->tiptext, ttt, i);
	delete (i);
	return tmp;
}

ActionButton::~ActionButton() {
	delete (r);
	delete (g);
	delete (b);
	deleteparams(params);
}

char* ActionTrigger::toString() {
	char* tmp;
	if (triggername) {
		tmp = new char[1024 + strlen(triggername)];
		sprintf(tmp, "ON \"%s\" %s", triggername, action->toString());
	} else {
		tmp = new char[1024];
		sprintf(tmp, "TRIGGER");
	}
	return tmp;
}

ActionTrigger::~ActionTrigger() {
	delete (triggername);
}

void ActionStatus::exec() {
	char* pos = (status_text + strlen(status_text));
	static Element* e;
	static Group* g;
	switch (function) {
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
		char* tmp;
		if (tmp = getmouseover())
			strcpy(pos, tmp);
		break;
	}
}

char* ActionStatus::toString() {
	char* tmp = 0;
	switch (function) {
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

ActionStatus::~ActionStatus() {
	delete (text);
	delete (v);
}

void ActionRemote::exec() {
	char* tmp;
	if (type == REMOTE_SET) {
		tmp = new char[1024 + strlen(text) + strlen(val->text)];
		sprintf(tmp, "SET \"%s\" %i", text, val->val());
		print(tmp, mid->val(), strlen(tmp));
		delete (tmp);
	}
	if (type == REMOTE_EXEC) {
		tmp = new char[1024 + strlen(text)];
		sprintf(tmp, "EXEC \"%s\"", text);
		print(tmp, mid->val(), strlen(tmp));
		delete (tmp);
	}
}

char* ActionRemote::toString() {
	char* tmp = 0;
	if (type == REMOTE_SET) {
		tmp = new char[1024 + strlen(text) + strlen(mid->text) + strlen(val->text)];
		sprintf(tmp, "REMOTE %s SET \"%s\" %s", mid->text, text, val->text);
	}
	if (type == REMOTE_EXEC) {
		tmp = new char[1024 + strlen(text) + strlen(mid->text)];
		sprintf(tmp, "REMOTE %s EXEC \"%s\"", val->text, text);
	}
	return tmp;
}

ActionRemote::~ActionRemote() {
	delete (text);
	delete (mid);
	delete (val);
}

void ActionConnect::exec() {
	setVar(var, connect(host, port->val()), true);
}

char* ActionConnect::toString() {
	char* tmp;
	tmp = new char[1024 + strlen(host) + strlen(port->text) + strlen(var)];
	sprintf(tmp, "CONNECT \"%s\" %s \"%s\"", host, port->text, var);
	return tmp;
}

ActionConnect::~ActionConnect() {
	delete (var);
	delete (host);
	delete (port);
}

void ActionWrite::exec() {
	switch (type) {
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

char* ActionWrite::toString() {
	const size_t base = 1024 + strlen(element->text) + strlen(x->text) + strlen(y->text) + strlen(size->text);
	char* tmp = nullptr;

	switch (type) {
	case WRITE_TEXT: {
		const size_t len = base + strlen(text);
		tmp = new char[len];
		snprintf(tmp, len, "WRITE %s %s %s %s TEXT \"%s\"", element->text, x->text, y->text, size->text, text);
		break;
	}
	case WRITE_NUMBER: {
		const size_t len = base + strlen(v->text);
		tmp = new char[len];
		snprintf(tmp, len, "WRITE %s %s %s %s NUMBER %s", element->text, x->text, y->text, size->text, v->text);
		break;
	}
	case WRITE_ELEMENT: {
		const size_t len = base + strlen(v->text);
		tmp = new char[len];
		snprintf(tmp, len, "WRITE %s %s %s %s ELEMENT %s", element->text, x->text, y->text, size->text, v->text);
		break;
	}
	case WRITE_GROUP: {
		const size_t len = base + strlen(v->text);
		tmp = new char[len];
		snprintf(tmp, len, "WRITE %s %s %s %s GROUP %s", element->text, x->text, y->text, size->text, v->text);
		break;
	}
	case WRITE_MESSAGE: {
		tmp = new char[base + 2];
		snprintf(tmp, base + 2, "WRITE %s %s %s %s MESSAGE 0", element->text, x->text, y->text, size->text);
		break;
	}
	case WRITE_STRING: {
		const size_t len = base + strlen(v->text);
		tmp = new char[len];
		snprintf(tmp, len, "WRITE %s %s %s %s STRING %s", element->text, x->text, y->text, size->text, v->text);
		break;
	}
	}
	return tmp;
}

ActionWrite::~ActionWrite() {
	delete (v);
	delete (x);
	delete (y);
	delete (size);
	delete (element);
	delete (text);
}

void ActionDraw::exec() {
	sanddraw(element->val(), brush, drawx->val(), drawy->val(), dx->val(), dy->val(), a1->val(), a2->val());
};

char* ActionDraw::toString() {
	char* b;
	switch (brush) {
	case BRUSH_FILLEDCIRCLE:
		b = "FILLEDCIRCLE";
		break;
	case BRUSH_CIRCLE:
		b = "CIRCLE";
		break;
	case BRUSH_RECT:
		b = "RECT";
		break;
	case BRUSH_FILLEDRECT:
		b = "FILLEDRECT";
		break;
	case BRUSH_LINE:
		b = "LINE";
		break;
	case BRUSH_FILL:
		b = "FILL";
		break;
	case REPLACE_FILLEDCIRCLE:
		b = "REPLACEFILLEDCIRCLE";
		break;
	case REPLACE_LINE:
		b = "REPLACELINE";
		break;
	case COPY_RECT:
		b = "COPYRECT";
		break;
	case ROTATE_RECT:
		b = "ROTATERECT";
		break;
	case BRUSH_POINT:
		b = "POINT";
		break;
	case BRUSH_RADNOMFILLEDCIRCLE:
		b = "RANDOMFILLEDCIRCLE";
		break;
	case COPY_STAMP:
		b = "COPYSTAMP";
		break;
	case PASTE_STAMP:
		b = "PASTESTAMP";
		break;
	case BRUSH_SWAPPOINTS:
		b = "SWAPPOINTS";
		break;
	case BRUSH_FILLEDELLIPSE:
		b = "FILLEDELLIPSE";
		break;
	case BRUSH_ELLIPSE:
		b = "ELLIPSE";
		break;
	case REPLACE_FILLEDELLIPSE:
		b = "REPLACEFILLEDELLIPSE";
		break;
	case BRUSH_RANDOMFILLEDELLIPSE:
		b = "RANDOMFILLEDELLIPSE";
		break;
	case BRUSH_POINTS:
		b = "FILLEDCIRCLE";
		break;
	case BRUSH_OBJECT:
		b = "OBJECT";
		break;
	default:
		b = "";
		break;
	}

	const char* p1 = (drawx && drawx->text) ? drawx->text : "";
	const char* p2 = (drawy && drawy->text) ? drawy->text : "";
	const char* p3 = (dx && dx->text) ? dx->text : "";
	const char* p4 = (dy && dy->text) ? dy->text : "";
	const char* p5 = (a1 && a1->text) ? a1->text : "";
	const char* p6 = (a2 && a2->text) ? a2->text : "";

	const size_t len = 1024 + strlen(element->text) + strlen(b) + strlen(p1) + strlen(p2) + strlen(p3) + strlen(p4) + strlen(p5) + strlen(p6);
	char* tmp = new char[len];
	snprintf(tmp, len, "DRAW %s %s %s %s %s %s %s %s", element->text, b, p1, p2, p3, p4, p5, p6);
	return tmp;
}

ActionDraw::~ActionDraw() {
	delete (drawx);
	delete (drawy);
	delete (dx);
	delete (dy);
	delete (element);
	delete (a1);
	delete (a2);
}

void ActionDrawPoints::exec() {
	std::list<int>::iterator xit = x.begin();
	std::list<int>::iterator yit = y.begin();
	int startx = xoffset->val();
	int starty = yoffset->val();
	while (xit != x.end()) {
		sanddraw(element->val(), BRUSH_POINT, startx + *xit, starty + *yit, 0, 0, 0, 0);
		xit++;
		yit++;
	}
};

char* ActionDrawPoints::toString() {
	char* tmp = new char[1024 + strlen(element->text) + x.size() * 20];
	char* tmp2 = new char[1024 + strlen(element->text) + x.size() * 20];
	std::list<int>::iterator xit = x.begin();
	std::list<int>::iterator yit = y.begin();
	sprintf(tmp, "DRAW %s POINTS %s %s", element->text, xoffset->text, yoffset->text);
	while (xit != x.end()) {
		strcpy(tmp2, tmp);
		sprintf(tmp, "%s %i %i", tmp2, *xit, *yit);
		xit++;
		yit++;
	}
	delete (tmp2);
	return tmp;
}

ActionDrawPoints::~ActionDrawPoints() {
	x.clear();
	y.clear();
	delete (xoffset);
	delete (yoffset);
	delete (element);
}

void ActionDrawObject::exec() {
	const int startx = xoffset->val();
	const int starty = yoffset->val();

	Uint16 e[256];
	for (int i = 0; i < 256; i++)
		e[i] = elements[i] ? static_cast<Uint16>(elements[i]->val()) : 1;

	int sx = sizex ? sizex->val() : 1;
	int sy = sizey ? sizey->val() : 1;

	int xstep = 1, ystep = 1;
	if (sx < 0) {
		xstep = -sx;
		sx = 1;
	}
	if (sy < 0) {
		ystep = -sy;
		sy = 1;
	}

	const bool scale = (sx == 1) && (sy == 1);

	int y = 0;
	for (auto dit = data.begin(); dit != data.end(); ) {
		const char* c = *dit;
		const int l = static_cast<int>(strlen(c));

		if (scale) {
			for (int i = 0; i < l; i++) {
				const Uint16 ei = e[static_cast<unsigned char>(c[i])];
				if (ei != 1) sanddraw(ei, BRUSH_POINT, startx + i, starty + y, 0, 0, 0, 0);
			}
		} else {
			for (int y2 = y; y2 < y + sy; y2++) {
				for (int i = 0; i < l; i += xstep) {
					const Uint16 ei = e[static_cast<unsigned char>(c[i])];
					if (ei != 1) {
						const int x0 = i * sx / xstep;
						for (int x2 = x0; x2 < x0 + sx; x2++)
							sanddraw(ei, BRUSH_POINT, startx + x2, starty + y2, 0, 0, 0, 0);
					}
				}
			}
		}

		for (int tmp = 0; tmp < ystep; tmp++) {
			++dit;
			if (dit == data.end()) return;
		}
		y += sy;
	}
}

char* ActionDrawObject::toString() {
	size_t len = 0;
	for (int i = 0; i < 256; i++)
		if (elements[i]) len += strlen(elements[i]->text);
	for (const auto* s : data) len += strlen(s) + 3;

	const size_t tlen = 1024 + strlen(xoffset->text) + strlen(yoffset->text) + len;
	char* tmp = new char[tlen];
	char* tmp2 = new char[tlen];

	snprintf(tmp, tlen, "DRAW 0 OBJECT %s %s", xoffset->text, yoffset->text);

	for (int i = 0; i < 256; i++) {
		if (elements[i]) {
			strcpy(tmp2, tmp);
			snprintf(tmp, tlen, "%s %c %s", tmp2,
				static_cast<char>(i), elements[i]->text);
		}
	}

	strcpy(tmp2, tmp);
	snprintf(tmp, tlen, "%s {\n", tmp2);

	for (const auto* s : data) {
		strcpy(tmp2, tmp);
		snprintf(tmp, tlen, "%s \"%s\"\n", tmp2, s);
	}

	strcpy(tmp2, tmp);
	snprintf(tmp, tlen, "%s\n}", tmp2);
	delete[] tmp2;
	return tmp;
}

ActionDrawObject::~ActionDrawObject() {
	delete (xoffset);
	delete (yoffset);
	delete (sizex);
	delete (sizey);
	for (int i = 0; i < 256; i++)
		delete (elements[i]);
	std::list<char*>::iterator it;
	for (it = data.begin(); it != data.end(); it++)
		delete (*it);
}

void ActionTimer::exec() {
	Timer* t;
	if (!timerStack.empty()) {
		t = timerStack.top();
		timerStack.pop();
	} else
		t = new Timer();
	t->trigger = trigger;
	if (params)
		t->params = calcparams(params);
	else
		t->params = 0;
	switch (type) {
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

char* ActionTimer::toString() {
	char* tmp = 0;
	switch (type) {
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

ActionTimer::~ActionTimer() {
	delete (value);
	deleteparams(params);
}

void ActionClearTimer::exec() {
	timercleared = true;
	if (!trigger) {
		while (!frametriggers.empty())
			frametriggers.pop();
	} else {
		std::list<Timer*> tmptimers;
		int c = frametriggers.size();
		for (int i = 0; i < c; i++) {
			tmptimers.push_back(frametriggers.top());
			frametriggers.pop();
		}
		std::list<Timer*>::iterator it = tmptimers.begin();
		int b = 0;
		while (it != tmptimers.end()) {
			if (trigger != (*it)->trigger)
				frametriggers.push(*it);
			else if (!removeall && b++)
				frametriggers.push(*it);
			it++;
		}
	}
}

char* ActionClearTimer::toString() {
	char* tmp;
	if (!trigger) {
		tmp = new char[1024];
		sprintf(tmp, "TIMER CLEAR");
	} else {
		if (removeall) {
			tmp = new char[1024 + strlen(trigger->name)];
			sprintf(tmp, "TIMER REMOVEALL \"%s\"", trigger->name);
		} else {
			tmp = new char[1024 + strlen(trigger->name)];
			sprintf(tmp, "TIMER REMOVE \"%s\"", trigger->name);
		}
	}
	return tmp;
}

void ActionGroup::exec() {
	Uint16 i;
	switch (function) {
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

char* ActionGroup::toString() {
	char* tmp = 0;
	switch (function) {
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

ActionGroup::~ActionGroup() {
	delete (order);
	delete (element);
	delete (groupname);
}

void ActionKey::exec() {
	addkey(keyname, v->val());
}

char* ActionKey::toString() {
	char* tmp = new char[1024 + strlen(keyname) + strlen(v->text)];
	sprintf(tmp, "KEYCODE %s %s", keyname, v->text);
	return tmp;
}

ActionKey::~ActionKey() {
	delete (v);
}

void ActionList::exec() {
	char tmp[255];

	switch (function) {
	case ACTION_LIST_ELEMENTS: {
		print("(ELEMENTS)", owner);
		Element* e = getElement(0);
		for (int i = 2; e[i].name; i++) print(e[i].name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_GROUP: {
		print("(GROUP)", owner);
		Element* e = getElement(0);
		Group* g = getGroup(findGroup(element, false, -1));
		for (auto v : g->elements) print(e[v].name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_GROUPS: {
		print("(GROUP)", owner);
		for (int i = 0; i < countGroups(); i++)
			print(getGroup(i)->name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_VARS: {
		print("(VARS)", owner);
		for (auto* v : *getVars()) print(v->name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_INTERACTIONS: {
		print("(INTERACTIONS)", owner);
		Element* e = getElement(findElement(element));
		for (auto* inter : *e->interactions) {
			char* s = inter->toString(element);
			print(s, owner);
			delete[] s;
		}
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_ELEMENTGROUPS: {
		snprintf(tmp, sizeof(tmp), "(ELEMENTGROUP:%s)", element);
		print(tmp, owner);
		const int i2 = findElement(element);
		for (int i = 0; i < countGroups(); i++)
			if (isElementInGroup(getGroup(i), i2))
				print(getGroup(i)->name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_DIETOS: {
		snprintf(tmp, sizeof(tmp), "(ELEMENTDIETOS:%s)", element);
		print(tmp, owner);
		Element* e = getElement(findElement(element));
		for (auto* d : *e->dies) print(getElement(d->dieto)->name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_TRIGGERS: {
		print("(TRIGGERS)", owner);
		for (auto* trig : triggers) print(trig->name, owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_TRIGGEREXECS: {
		print("(TRIGGEREXECS)", owner);
		int total = 0;
		for (auto* trig : triggers) {
			snprintf(tmp, sizeof(tmp), "%10u %s", trig->execcount, trig->name);
			total += trig->execcount;
			print(tmp, owner);
		}
		print("(END)", owner);
		snprintf(tmp, sizeof(tmp), "(ALLEXECS) %i", total);
		print(tmp, owner);
		break;
	}
	case ACTION_LIST_ACTIONS: {
		Trigger* trig = findTrigger(element, owner);
		snprintf(tmp, sizeof(tmp), "(ACTIONS:%s)", element);
		print(tmp, owner);
		for (auto* act : trig->actions) print(act->toString(), owner);
		print("(END)", owner);
		break;
	}
	case ACTION_LIST_TIMERS: {
		print("(TIMERS)", owner);
		std::list<Timer*> timerlist;
		while (!frametriggers.empty()) {
			Timer* t = frametriggers.top();
			frametriggers.pop();
			if (t && t->trigger && t->trigger->name) {
				snprintf(tmp, sizeof(tmp), "%i %s", t->value - thisframe, t->trigger->name);
				print(tmp, owner);
				timerlist.push_back(t);
			}
		}
		for (auto* t : timerlist) frametriggers.push(t);
		print("(END)", owner);
		break;
	}
	}
}

char* ActionList::toString() {
	char* tmp = 0;
	switch (function) {
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

ActionList::~ActionList() {}
void ActionMessage::exec() {
	char* pos = messagestring + strlen(messagestring);
	int i;

	switch (function) {
	case MESSAGE_CLEAR:
		messagestring[0] = '\0';
		break;
	case MESSAGE_ADDTEXT:
		strcpy(pos, message);
		break;
	case MESSAGE_ADDNUMBER: {
		char t[256];
		snprintf(t, sizeof(t), "%i", varint->val());
		strcpy(pos, t);
		break;
	}
	case MESSAGE_ADDELEMENT: {
		const Element* e = getElement(varint->val());
		if (e) strcpy(pos, e->name);
		break;
	}
	case MESSAGE_ADDELEMENTTEXT: {
		const Element* e = getElement(varint->val());
		if (e && e->icon && !strcmp(e->icon->type, "TEXT") && e->icon->text)
			strcpy(pos, e->icon->text);
		break;
	}
	case MESSAGE_ADDGROUP: {
		const Group* g = getGroup(varint->val());
		if (g) strcpy(pos, g->name);
		break;
	}
	case MESSAGE_SEND:
		print(messagestring, varint->ok ? varint->val() : owner);
		break;
	case MESSAGE_SAVE:
		savefile.write(messagestring, (std::streamsize)strlen(messagestring));
		savefile.write("\n", 1);
		break;
	case MESSAGE_EXEC:
		parseline(messagestring, 1, varint->ok ? varint->val() : owner, "MESSAGE");
		break;
	case MESSAGE_SENDTEXT:
		print(message, varint->ok ? varint->val() : owner);
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
		if (yesnobox(messagestring, "Run Command?")) {
			std::ofstream batchfile;
			batchfile.open(checkfilename("tmp.cmd"), std::ios::out);
			batchfile.write(messagestring, strlen(messagestring));
			batchfile.close();
			ossystem("tmp.cmd", nullptr);
			std::remove("tmp.cmd");
		}
#endif
		break;
	case MESSAGE_SAVESTRING:
		i = varint->val();
		delete[] strings[i];
		strings[i] = new char[strlen(messagestring) + 1];
		strcpy(strings[i], messagestring);
		break;
	case MESSAGE_ADDSTRING:
		i = varint->val();
		if (strings[i]) strcpy(pos, strings[i]);
		break;
	}
}

char* ActionMessage::toString() {
	char* tmp = nullptr;
	std::size_t len;

	switch (function) {
	case MESSAGE_CLEAR:
		tmp = new char[1024];
		snprintf(tmp, 1024, "MESSAGE CLEAR");
		break;
	case MESSAGE_ADDTEXT:
		len = 1024 + strlen(message);
		tmp = new char[len];
		snprintf(tmp, len, "MESSAGE ADDTEXT \"%s\"", message);
		break;
	case MESSAGE_ADDNUMBER:
		len = 1024 + strlen(varint->text);
		tmp = new char[len];
		snprintf(tmp, len, "MESSAGE ADDNUMBER %s", varint->text);
		break;
	case MESSAGE_ADDELEMENT:
		len = 1024 + strlen(varint->text);
		tmp = new char[len];
		snprintf(tmp, len, "MESSAGE ADDELEMENT %s", varint->text);
		break;
	case MESSAGE_ADDSTRING:
		len = 1024 + strlen(varint->text);
		tmp = new char[len];
		snprintf(tmp, len, "MESSAGE ADDSTRING %s", varint->text);
		break;
	case MESSAGE_SAVESTRING:
		len = 1024 + strlen(varint->text);
		tmp = new char[len];
		snprintf(tmp, len, "MESSAGE SAVESTRING %s", varint->text);
		break;
	case MESSAGE_ADDGROUP:
		len = 1024 + strlen(varint->text);
		tmp = new char[len];
		snprintf(tmp, len, "MESSAGE ADDGROUP %s", varint->text);
		break;
	case MESSAGE_SEND:
		if (varint->ok) {
			len = 1024 + strlen(varint->text);
			tmp = new char[len];
			snprintf(tmp, len, "MESSAGE SEND %s", varint->text);
		} else {
			tmp = new char[1024];
			snprintf(tmp, 1024, "MESSAGE SEND");
		}
		break;
	case MESSAGE_SAVE:
		tmp = new char[1024];
		snprintf(tmp, 1024, "MESSAGE SAVE");
		break;
	case MESSAGE_EXEC:
		if (varint->ok) {
			len = 1024 + strlen(varint->text);
			tmp = new char[len];
			snprintf(tmp, len, "MESSAGE EXEC %s", varint->text);
		} else {
			tmp = new char[1024];
			snprintf(tmp, 1024, "MESSAGE EXEC");
		}
		break;
	case MESSAGE_SENDTEXT:
		if (varint->ok) {
			len = 1024 + strlen(message) + strlen(varint->text);
			tmp = new char[len];
			snprintf(tmp, len, "MESSAGE SENDTEXT \"%s\" %s", message, varint->text);
		} else {
			len = 1024 + strlen(message);
			tmp = new char[len];
			snprintf(tmp, len, "MESSAGE SENDTEXT \"%s\"", message);
		}
		break;
	case MESSAGE_BS1:
		len = 1024 + strlen(message);
		tmp = new char[len];
		snprintf(tmp, len, "Message \"%s\"", message);
		break;
	case MESSAGE_MESSAGEBOX:
		tmp = new char[1024];
		snprintf(tmp, 1024, "MESSAGE MESSAGEBOX");
		break;
	case MESSAGE_ADDLINE:
		tmp = new char[1024];
		snprintf(tmp, 1024, "MESSAGE ADDLINE");
		break;
	case MESSAGE_SYSTEM:
		tmp = new char[1024];
		snprintf(tmp, 1024, "MESSAGE SYSTEM");
		break;
	}
	return tmp;
}

ActionMessage::~ActionMessage() {
	if (function == MESSAGE_SENDTEXT)
		delete (message);
	delete (varint);
}

void ActionInclude::exec() {
	if (!strcmp(filename, "CLIPBOARD")) {
		char* text;
		if (text = getStringFromClipboard()) {
			if (strstr(text, "http://") == text) {
				checkfile(text, false);
				parsefile(text, owner);
			} else parsechar(text, owner, filename);
		}
	} else if (!strcmp(filename, "FILEDIALOG")) {
		char* text;
		if (param) {
			char* tmp = new char[strlen(param) + 2];
			strcpy(tmp, param);
			tmp[strlen(tmp) - 1] = 0;
			for (unsigned int i = 0; i < strlen(param); i++)
				if (tmp[i] == '|') tmp[i] = 0;
			if (text = opendialog(tmp, 0)) {
				checkfile(text, false);
				parsefile(text, owner);
			}
			delete (tmp);
		} else if (text = opendialog("BS2 File\0*.bs2\0BS1 File\0*.cfg\0TXT File\0*.txt\0All\0*.*\0", 0)) {
			checkfile(text, false);
			parsefile(text, owner);
		}
	} else {
		char* t = messageReplace(filename);
		checkfile(t);
		parsefile(t, owner);
	}
}

char* ActionInclude::toString() {
	const int len = 1024 + strlen(filename);
	char* tmp = new char[len];
	snprintf(tmp, len, "INCLUDE \"%s\"", filename);
	return tmp;
}

ActionInclude::~ActionInclude() {
	delete (filename);
}

void ActionWind::exec() {
	Wind* w = new Wind();
	w->x = 240;
	w->y = 240;
	w->angle = (float)3.141592654;
	winds.push_back(w);
}

char* ActionWind::toString() {
	char* tmp = new char[1];
	tmp[0] = 0;
	return tmp;
}

void ActionNoBias::exec() {
	setElementBias(e->val(), true);
}

char* ActionNoBias::toString() {
	const int len = 1024 + strlen(e->text);
	char* tmp = new char[len];
	snprintf(tmp, len, "NOBIAS %s", e->text);
	return tmp;
}

ActionNoBias::~ActionNoBias() {
	delete e;
}

void ActionMenu::exec() {
	if (action == ACTION_MENU_CLEAR) clearMenuBar(bar);
	redrawmenu(3);
}

char* ActionMenu::toString() {
	char* tmp = new char[1024];
	if (action == ACTION_MENU_CLEAR) {
		const char* w = nullptr;
		if (bar == MENU_BAR_TOP) w = "TOP";
		else if (bar == MENU_BAR_LEFT) w = "LEFT";
		else if (bar == MENU_BAR_RIGHT) w = "RIGHT";
		else if (bar == MENU_BAR_BOTTOM) w = "BOTTOM";
		else if (bar == MENU_BAR_SUB) w = "SUB";
		snprintf(tmp, 1024, "MENU %s CLEAR", w ? w : "");
	} else {
		snprintf(tmp, 1024, "MENU REFRESH");
	}
	return tmp;
}

ActionMenu::~ActionMenu() {}

void ActionSubMenu::exec() {
	if (function) {
		hideSubMenu();
	} else {
		if (x->ok && y->ok)
			showSubMenu(stay, align, x->val(), y->val());
		else showSubMenu(stay, align);
	}
}

char* ActionSubMenu::toString() {
	char* tmp = new char[1024];
	if (function) {
		snprintf(tmp, 1024, "SUBMENU CLOSE");
	} else if (stay) {
		if (x->ok && y->ok)
			snprintf(tmp, 1024, "SUBMENU %s %s STAY", x->text, y->text);
		else snprintf(tmp, 1024, "SUBMENU STAY");
	} else {
		if (x->ok && y->ok)
			snprintf(tmp, 1024, "SUBMENU %s %s", x->text, y->text);
		else snprintf(tmp, 1024, "SUBMENU");
	}
	return tmp;
}

ActionSubMenu::~ActionSubMenu() {}

void frametimer() {
	static Var* vt = (Var*)setVar("FRAME", 0);
	vt->value = thisframe;
	if (debugframe->value) {
		char tmp[512];
		snprintf(tmp, sizeof(tmp), "frame: %i timers: %i", thisframe, frametriggers.size());
		std::cout << tmp << std::endl;
	}
	timercleared = false;
	if (frametriggers.size()) {
		Timer* t = frametriggers.top();
		while (!frametriggers.empty() && (t->value <= thisframe)) {
			if (t->params) addparams(t->params);
			t->trigger->exec();
			if (t->params) removeparams();

			if (timercleared) {
				if (frametriggers.size() == 0) return;
				if (t == frametriggers.top()) {
					timerStack.push(t);
					frametriggers.pop();
				}
			} else {
				timerStack.push(t);
				frametriggers.pop();
			}
			t = frametriggers.top();
		}
	}
	thisframe++;
}

void msectimer() {
	static unsigned int startsec = SDL_GetTicks();
	static Var* vms = (Var*)setVar("MSEC", 0);
	vms->value = SDL_GetTicks() - startsec;
}