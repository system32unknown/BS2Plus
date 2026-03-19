#include "elements.h"
#include "trigger.h"
#include <cstring>

Element* elements;
std::list<Group*> groups;
std::list<int> grouporder;
Pic* clearpic, * nopic;
int elementsmax;
char* Interaction::toString(char* e) {
	const char* ename = getElement(element) ? getElement(element)->name : "?";
	const char* selfname = getElement(toself) ? getElement(toself)->name : "?";
	const char* othname = getElement(toother) ? getElement(toother)->name : "?";

	char* tmp = new char[4096];
	if (trigger == nullptr)
		snprintf(tmp, 4096, "INTERACTION \"%s\" \"%s\" \"%s\" \"%s\" %i", e, ename, selfname, othname, rate);
	else snprintf(tmp, 4096, "INTERACTIONTRIGGER \"%s\" \"%s\" \"%s\" %i", e, ename, reinterpret_cast<Trigger*>(trigger)->name, rate);
	return tmp;
}

void initelements() {
	elementsmax = 500;
	elements = new Element[elementsmax];
	clearpic = getPic("Clear", "0");

	for (Uint16 i = 0; i < elementsmax; i++) {
		elements[i].interactions = new std::list<Interaction*>();
		elements[i].dies = new std::list<Die*>();
		elements[i].icon = clearpic;
	}
	setprecalc(3);
	clearelements();
}

static void resetElementFields(Element& el) {
	el.weight = 0;
	el.spray = 1;
	el.slide = 0;
	el.viscousity = 1;
	el.nobias = false;
	el.r = el.g = el.b = 0;
	el.cr1 = el.cg1 = el.cb1 = 0;
	el.cr2 = el.cg2 = el.cb2 = 0;
	el.cr3 = el.cg3 = el.cb3 = 0;
	el.dietotalrate = 0;
	el.interactioncount = 0;
}

void clearelements() {
	resetElementFields(elements[0]);
	elements[0].name = "Clear";
	elements[0].weight = 1;

	resetElementFields(elements[1]);
	elements[1].name = "Nothing";
	elements[1].weight = 9999999;

	for (Uint16 i = 2; i < elementsmax; i++) {
		elements[i].name = nullptr;
		resetElementFields(elements[i]);

		for (auto* p : *elements[i].interactions) delete p;
		elements[i].interactions->clear();

		for (auto* p : *elements[i].dies) delete p;
		elements[i].dies->clear();
	}

	for (auto* g : groups) {
		g->elements.clear();
		g->elementorder.clear();
	}

	setprecalc(3);
}

int getelementsmax() {
	return elementsmax;
}

int getelementscount() {
	int i = 0;
	while (elements[++i].name);
	return i;
}

Uint16 findElement(char* elementname, bool create) {
	if (elementname == nullptr) return 0;

	char* element = messageReplace(elementname);

	if (!strcmp(element, "CLEAR") || !strcmp(element, "Clear")) return 0;
	if (!strcmp(element, "DEFAULT") || !strcmp(element, "All")) return 1;

	const size_t elen = strlen(element);
	for (size_t i = 0; i < elen; i++) {
		if (element[i] == ':') {
			element[i] = 0;
			int n = 0;
			getVar(element + i + 1, &n);
			element[i] = ':';
			if (n > 0) {
				Group* g = getGroup(findGroup(element, create, -1));
				if (g && static_cast<int>(g->elements.size()) >= n) {
					auto it = g->elements.begin();
					std::advance(it, n - 1);
					return static_cast<Uint16>(*it);
				}
			}
			return 0;
		}
	}

	Uint16 i = 0;
	for (; elements[i].name; i++)
		if (!strcmp(element, elements[i].name))
			return i;

	if (!create) return 0;

	if (i > elementsmax - 2) {
		if (i > 32000) return 1;

		const Uint16 elementsmaxold = static_cast<Uint16>(elementsmax);
		elementsmax += 1500;
		Element* oldelements = elements;
		elements = new Element[elementsmax];

		for (Uint16 i2 = 0; i2 < elementsmaxold; i2++) {
			elements[i2] = oldelements[i2];
		}

		for (Uint16 i2 = elementsmaxold; i2 < static_cast<Uint16>(elementsmax); i2++) {
			elements[i2].interactions = new std::list<Interaction*>();
			elements[i2].dies = new std::list<Die*>();
			elements[i2].icon = getPic("Clear", "0");
			elements[i2].name = nullptr;
			elements[i2].weight = 0;
			elements[i2].spray = 1;
			elements[i2].slide = 0;
			elements[i2].viscousity = 1;
			elements[i2].nobias = false;
			elements[i2].r = elements[i2].g = elements[i2].b = 0;
			elements[i2].cr1 = elements[i2].cg1 = elements[i2].cb1 = 0;
			elements[i2].cr2 = elements[i2].cg2 = elements[i2].cb2 = 0;
			elements[i2].cr3 = elements[i2].cg3 = elements[i2].cb3 = 0;
			elements[i2].dietotalrate = 0;
			elements[i2].interactioncount = 0;
		}

		setprecalc(5);
		delete[] oldelements;
	}

	elements[i].name = new char[strlen(element) + 1];
	elements[i].icon = clearpic;
	strcpy(elements[i].name, element);
	return i;
}

int addInteraction(Uint16 id, Uint16 elementid, Uint16 toself, Uint16 toother, Uint16 rate, Uint16 except, void* trigger, int pos) {
	if (id == 1 || elements[id].interactioncount > 32000) return 0;

	Interaction* inter = new Interaction();
	inter->element = elementid;
	inter->toself = toself;
	inter->toother = toother;
	inter->rate = rate;
	inter->except = except;
	inter->trigger = trigger;

	if (pos == -1) {
		elements[id].interactions->push_back(inter);
	} else {
		auto it = elements[id].interactions->begin();
		std::advance(it, std::min(pos, static_cast<int>(elements[id].interactions->size())));
		elements[id].interactions->insert(it, inter);
	}

	elements[id].interactioncount++;
	setprecalc(3);
	return 0;
}

int clearInteraction(Uint16 id) {
	if (id == 1) return 0;
	for (auto* p : *elements[id].interactions) delete p;
	elements[id].interactions->clear();
	elements[id].interactioncount = 0;
	setprecalc(3);
	return 0;
}

int removeInteraction(Uint16 id, Uint16 pos) {
	if (id == 1) return 0;
	Uint16 tmp = 0;
	for (auto it = elements[id].interactions->begin();
		it != elements[id].interactions->end(); ++it) {
		if (tmp == pos) {
			elements[id].interactions->erase(it);
			elements[id].interactioncount--;
			delete* it;
			return 0;
		}
		tmp++;
	}
	setprecalc(3);
	return 0;
}

int clearDie(Uint16 id) {
	if (id == 1) return 0;
	for (auto* p : *elements[id].dies) delete p;
	elements[id].dies->clear();
	setprecalc(3);
	return 0;
}

void interactionTrigger(void* trigger, Uint16 element1, Uint16 element2, int x1, int y1, int x2, int y2) {
	if (element1 == 1 || element2 == 1) return;
	static Var* vari1 = (Var*)setVar("INTERACTION1", 0);
	static Var* vari2 = (Var*)setVar("INTERACTION2", 0);
	static Var* varx1 = (Var*)setVar("X1", 0);
	static Var* vary1 = (Var*)setVar("Y1", 0);
	static Var* varx2 = (Var*)setVar("X2", 0);
	static Var* vary2 = (Var*)setVar("Y2", 0);
	vari1->value = element1;
	vari2->value = element2;
	varx1->value = x1;
	vary1->value = y1;
	varx2->value = x2;
	vary2->value = y2;
	reinterpret_cast<Trigger*>(trigger)->exec();
}

int addDie(Uint16 id, Uint16 dieto, Uint16 rate) {
	if (id == 1 || dieto == 1 || rate == 0) return 0;
	if (elements[id].dies->size() > MAX_DIES - 3) return 0;
	Die* d = new Die();
	d->dieto = dieto;
	d->rate = rate;
	elements[id].dies->push_back(d);
	elements[id].dietotalrate = static_cast<Uint16>(std::min(static_cast<int>(elements[id].dietotalrate) + rate, 32768));
	setprecalc(3);
	return 0;
}

int setElementWeight(Uint16 id, int weight) {
	if (id == 1) return 0;
	elements[id].weight = weight;
	setprecalc(3);
	return 0;
}

int setElementSpray(Uint16 id, int spray) {
	if (id == 1) return 0;
	static int showerror = 1;
	if (!spray) {
		if (showerror) {
			addConsoleTextLine("Spray cannot be set to 0. Using 1. This will only be shown once.");
			showerror = 0;
		}
		spray = 1;
	}
	elements[id].spray = spray;
	setprecalc(3);
	return 0;
}

int setElementSlide(Uint16 id, int slide) {
	if (id == 1) return 0;
	elements[id].slide = slide;
	setprecalc(3);
	return 0;
}

int setElementViscousity(Uint16 id, int viscousity) {
	if (id == 1) return 0;
	if (!viscousity) {
		addConsoleTextLine("Viscousity cannot be set to 0. Using 1.");
		viscousity = 1;
	}
	elements[id].viscousity = viscousity;
	setprecalc(3);
	return 0;
}

int setElementColor(Uint16 id, unsigned char r, unsigned char g, unsigned char b) {
	if (id == 1) return 0;
	elements[id].r = r;
	elements[id].g = g;
	elements[id].b = b;
	setprecalc(3);
	return 0;
}

int setElementR(Uint16 id, unsigned char r) {
	if (id == 1) return 0;
	elements[id].r = r;
	setprecalc(3);
	return 0;
}

int setElementG(Uint16 id, unsigned char g) {
	if (id == 1) return 0;
	elements[id].g = g;
	setprecalc(3);
	return 0;
}

int setElementB(Uint16 id, unsigned char b) {
	if (id == 1) return 0;
	elements[id].b = b;
	setprecalc(3);
	return 0;
}

int setElementBias(Uint16 id, bool b) {
	if (id == 1) return 0;
	elements[id].nobias = b;
	setprecalc(3);
	return 0;
}

int setElementCustomColor(Uint16 id, int i, int c, unsigned char b) {
	if (id == 1) return 0;

	Element& e = elements[id];
	bool matched = true;

	switch (i) {
	case 1:
		switch (c) {
		case 1: e.cr1 = b; break;
		case 2: e.cg1 = b; break;
		case 3: e.cb1 = b; break;
		default: matched = false; break;
		}
		break;
	case 2:
		switch (c) {
		case 1: e.cr2 = b; break;
		case 2: e.cg2 = b; break;
		case 3: e.cb2 = b; break;
		default: matched = false; break;
		}
		break;
	case 3:
		switch (c) {
		case 1: e.cr3 = b; break;
		case 2: e.cg3 = b; break;
		case 3: e.cb3 = b; break;
		default: matched = false; break;
		}
		break;
	default:
		matched = false;
		break;
	}

	if (matched) setprecalc(3);
	return 0;
}

int setElementIcon(Uint16 id, Pic* icon) {
	if (id == 1) return 0;
	elements[id].icon = icon;
	return 0;
}

Element* getElement(Uint16 id) {
	if (!id) return elements;
	if ((id < elementsmax) && (elements[id].name)) return &(elements[id]);
	return 0;
}

int findGroup(char* groupname, bool create, int order) {
	const char* name = messageReplace(groupname);
	if (order == -1) order = 65535;

	int i = 0;
	for (auto* g : groups) {
		if (!strcmp(name, g->name)) return i;
		i++;
	}

	if (!create) return 0;

	Group* g = new Group();
	g->name = new char[strlen(name) + 1];
	strcpy(g->name, name);
	g->icon = nullptr;

	auto it = groups.begin();
	auto it2 = grouporder.begin();
	i = 0;
	for (; it != groups.end(); ++it, ++it2, ++i) {
		if (*it2 > order) {
			groups.insert(it, g);
			grouporder.insert(it2, order);
			return i;
		}
	}

	groups.push_back(g);
	grouporder.push_back(order);
	return i;
}

Group* getGroup(int value) {
	if (value < 0 || static_cast<size_t>(value) >= groups.size()) return nullptr;
	auto it = groups.begin();
	std::advance(it, value);
	return *it;
}

int getGroupElement(char* name, int num) {
	int i = 0;
	if (!getVar(name, &i)) i = findGroup(name, false, -1);
	Group* g = getGroup(i);
	if (!g || static_cast<int>(g->elements.size()) < num)
		return 0;
	auto it = g->elements.begin();
	std::advance(it, num - 1);
	return *it;
}

int getGroupElementOrder(char* name, int num) {
	int i = 0;
	if (!getVar(name, &i)) i = findGroup(name, false, -1);
	Group* g = getGroup(i);
	if (!g || static_cast<int>(g->elements.size()) < num)
		return 0;
	auto it = g->elementorder.begin();
	std::advance(it, num - 1);
	return *it;
}

bool isElementInGroup(Group* group, int element) {
	for (int v : group->elements)
		if (v == element) return true;
	return false;
}

void addElementToGroup(Group* group, int element, int pos) {
	if (element == 1) return;
	for (int v : group->elements)
		if (v == element) return;
	auto it = group->elements.begin();
	auto it2 = group->elementorder.begin();
	for (; it != group->elements.end(); ++it, ++it2) {
		if (*it2 > pos) {
			group->elements.insert(it, element);
			group->elementorder.insert(it2, pos);
			return;
		}
	}
	group->elements.push_back(element);
	group->elementorder.push_back(pos);
}

void clearGroup(char* name) {
	int i = 0;
	if (!getVar(name, &i)) i = findGroup(name, false, -1);
	Group* g = getGroup(i);
	if (!g) return;
	g->elements.clear();
	g->elementorder.clear();
}

void clearGroups() {
	for (auto* g : groups) {
		g->elements.clear();
		g->elementorder.clear();
	}
	groups.clear();
	grouporder.clear();
	findGroup("None", true, 0);
}

int countGroups() {
	return groups.size();
}