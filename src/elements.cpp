#include "elements.h"
#include "trigger.h"
#include <cstring>

Element* elements;
std::list<Group*> groups;
std::list<int> grouporder;
Pic* clearpic, * nopic;
int elementsmax;

char* Interaction::toString(char* e) {
	char* tmp = new char[4096];
	if (trigger == NULL)
		sprintf(tmp, "INTERACTION \"%s\" \"%s\" \"%s\" \"%s\" %i", e, getElement(element)->name, getElement(toself)->name, getElement(toother)->name, rate);
	else
		sprintf(tmp, "INTERACTIONTRIGGER \"%s\" \"%s\" \"%s\" %i", e, getElement(element)->name, ((Trigger*)trigger)->name, rate);
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

void clearelements() {
	std::list<Interaction*>::iterator it;
	std::list<Die*>::iterator it2;
	std::list<Group*>::iterator it3;
	elements[0].name = "Clear";
	elements[0].weight = 1;
	elements[0].spray = 1;
	elements[0].viscousity = 1;
	elements[0].nobias = false;
	elements[0].dietotalrate = 0;
	elements[0].r = 0;
	elements[0].g = 0;
	elements[0].b = 0;
	elements[0].interactioncount = 0;
	elements[1].name = "Nothing";
	elements[1].weight = 9999999;
	elements[1].spray = 1;
	elements[1].viscousity = 1;
	elements[1].nobias = false;
	elements[1].dietotalrate = 0;
	elements[1].r = 0;
	elements[1].g = 0;
	elements[1].b = 0;
	elements[1].cr1 = 0;
	elements[1].cg1 = 0;
	elements[1].cb1 = 0;
	elements[1].cr2 = 0;
	elements[1].cg2 = 0;
	elements[1].cb2 = 0;
	elements[1].cr3 = 0;
	elements[1].cg3 = 0;
	elements[1].cb3 = 0;
	elements[1].interactioncount = 0;
	for (Uint16 i = 2; i < elementsmax; i++) {
		elements[i].name = NULL;
		elements[i].weight = 0;
		elements[i].spray = 1;
		elements[i].slide = 0;
		elements[i].viscousity = 1;
		elements[i].r = 0;
		elements[i].g = 0;
		elements[i].b = 0;
		elements[i].nobias = false;
		elements[i].cr1 = 0;
		elements[i].cg1 = 0;
		elements[i].cb1 = 0;
		elements[i].cr2 = 0;
		elements[i].cg2 = 0;
		elements[i].cb2 = 0;
		elements[i].cr3 = 0;
		elements[i].cg3 = 0;
		elements[i].cb3 = 0;
		elements[i].dietotalrate = 0;
		elements[i].interactioncount = 0;
		for (it = elements[i].interactions->begin(); it != elements[i].interactions->end(); it++)
			delete (*it);
		elements[i].interactions->clear();
		for (it2 = elements[i].dies->begin(); it2 != elements[i].dies->end(); it2++)
			delete (*it2);
		elements[i].dies->clear();
	}
	for (it3 = groups.begin(); it3 != groups.end(); it3++) {
		(*it3)->elements.clear();
		(*it3)->elementorder.clear();
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
	if (elementname == 0)
		return 0;
	char* element = messageReplace(elementname);
	if (!strcmp(element, "CLEAR"))
		return 0;
	if (!strcmp(element, "Clear"))
		return 0;
	if (!strcmp(element, "DEFAULT"))
		return 1;
	if (!strcmp(element, "All"))
		return 1;
	Uint16 i = 0;
	for (i = 0; i < strlen(element); i++)
		if (element[i] == ':') {
			element[i] = 0;
			int n;
			getVar((element + i + 1), &n);
			unsigned int i2 = n;
			Group* g = getGroup(findGroup(element, create, -1));
			if ((g->elements.size() >= i2) && (i2 > 0)) {
				std::list<int>::iterator it = g->elements.begin();
				for (int ii = 1; ii < n; ii++)
					it++;
				return *it;
			}
		}
	i = 0;
	for (; elements[i].name; i++)
		if (!strcmp(element, elements[i].name)) {
			return i;
		}
	if (create == true) {
		if (i > elementsmax - 2) {
			if (i > 32000)
				return 1;
			int elementsmaxold = elementsmax;
			elementsmax += 1500;
			Element* oldelements = elements;
			elements = new Element[elementsmax];
			Uint16 i2 = 0;
			for (; i2 < elementsmaxold; i2++) {
				elements[i2].interactions = oldelements[i2].interactions;
				elements[i2].dies = oldelements[i2].dies;
				elements[i2].icon = oldelements[i2].icon;
			}
			for (; i2 < elementsmax; i2++) {
				elements[i2].interactions = new std::list<Interaction*>();
				elements[i2].dies = new std::list<Die*>();
				elements[i2].icon = getPic("Clear", "0");
			}
			i2 = 0;
			for (; i2 < elementsmaxold; i2++) {
				elements[i2].name = oldelements[i2].name;
				elements[i2].weight = oldelements[i2].weight;
				elements[i2].spray = oldelements[i2].spray;
				elements[i2].slide = oldelements[i2].slide;
				elements[i2].viscousity = oldelements[i2].viscousity;
				elements[i2].r = oldelements[i2].r;
				elements[i2].g = oldelements[i2].g;
				elements[i2].b = oldelements[i2].b;
				elements[i2].nobias = oldelements[i2].nobias;
				elements[i2].cr1 = oldelements[i2].cr1;
				elements[i2].cg1 = oldelements[i2].cg1;
				elements[i2].cb1 = oldelements[i2].cb1;
				elements[i2].cr2 = oldelements[i2].cr2;
				elements[i2].cg2 = oldelements[i2].cg2;
				elements[i2].cb2 = oldelements[i2].cb2;
				elements[i2].cr3 = oldelements[i2].cr3;
				elements[i2].cg3 = oldelements[i2].cg3;
				elements[i2].cb3 = oldelements[i2].cb3;
				elements[i2].dietotalrate = oldelements[i2].dietotalrate;
				elements[i2].interactioncount = oldelements[i2].interactioncount;
			}
			for (; i2 < elementsmax; i2++) {
				elements[i2].name = NULL;
				elements[i2].weight = 0;
				elements[i2].spray = 1;
				elements[i2].slide = 0;
				elements[i2].viscousity = 1;
				elements[i2].nobias = false;
				elements[i2].r = 0;
				elements[i2].g = 0;
				elements[i2].b = 0;
				elements[i2].cr1 = 0;
				elements[i2].cg1 = 0;
				elements[i2].cb1 = 0;
				elements[i2].cr2 = 0;
				elements[i2].cg2 = 0;
				elements[i2].cb2 = 0;
				elements[i2].cr3 = 0;
				elements[i2].cg3 = 0;
				elements[i2].cb3 = 0;
				elements[i2].dietotalrate = 0;
				elements[i2].interactioncount = 0;
			}
			setprecalc(5);
			delete (oldelements);
		}
		elements[i].name = new char[strlen(element) + 1];
		elements[i].icon = clearpic;
		strcpy(elements[i].name, element);
		return i;
	}
	return 0;
}

int addInteraction(Uint16 id, Uint16 elementid, Uint16 toself, Uint16 toother, Uint16 rate, Uint16 except, void* trigger, int pos) {
	if (elements[id].interactioncount > 32000)
		return 0;
	if (id == 1)
		return 0;
	Interaction* i = new Interaction();
	i->element = elementid;
	i->toself = toself;
	i->toother = toother;
	i->rate = rate;
	i->except = except;
	i->trigger = trigger;
	if (pos == -1)
		elements[id].interactions->push_back(i);
	else {
		std::list<Interaction*>::iterator it;
		for (it = elements[id].interactions->begin(); (it != elements[id].interactions->end()) && (pos); it++)
			pos--;
		elements[id].interactions->insert(it, i);
	}
	(elements[id].interactioncount)++;
	setprecalc(3);
	return 0;
}

int clearInteraction(Uint16 id) {
	if (id == 1)
		return 0;
	std::list<Interaction*>::iterator it;
	for (it = elements[id].interactions->begin(); it != elements[id].interactions->end(); it++)
		delete (*it);
	elements[id].interactions->clear();
	elements[id].interactioncount = 0;
	setprecalc(3);
	return 0;
}

int removeInteraction(Uint16 id, Uint16 pos) {
	if (id == 1)
		return 0;
	std::list<Interaction*>::iterator it;
	Uint16 tmp = 0;
	for (it = elements[id].interactions->begin(); it != elements[id].interactions->end(); it++) {
		if (tmp == pos) {
			elements[id].interactions->erase(it);
			elements[id].interactioncount--;
			delete (*it);
			return 0;
		}
		tmp++;
	}
	setprecalc(3);
	return 0;
}

int clearDie(Uint16 id) {
	if (id == 1)
		return 0;
	std::list<Die*>::iterator it;
	for (it = elements[id].dies->begin(); it != elements[id].dies->end(); it++)
		delete (*it);
	elements[id].dies->clear();
	setprecalc(3);
	return 0;
}

void interactionTrigger(void* trigger, Uint16 element1, Uint16 element2, int x1, int y1, int x2, int y2) {
	if (element1 == 1)
		return;
	if (element2 == 1)
		return;
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
	((Trigger*)trigger)->exec();
}

int addDie(Uint16 id, Uint16 dieto, Uint16 rate) {
	if (id == 1)
		return 0;
	if (dieto == 1)
		return 0;
	if (rate == 0)
		return 0;
	if (elements[id].dies->size() > MAX_DIES - 3)
		return 0;
	setprecalc(3);
	Die* i = new Die();
	i->dieto = dieto;
	i->rate = rate;
	elements[id].dies->push_back(i);
	if (elements[id].dietotalrate + rate > 32768)
		elements[id].dietotalrate = 32768;
	else
		elements[id].dietotalrate += rate;
	return 0;
}

int setElementWeight(Uint16 id, int weight) {
	if (id == 1)
		return 0;
	elements[id].weight = weight;
	setprecalc(3);
	return 0;
}

int setElementSpray(Uint16 id, int spray) {
	if (id == 1)
		return 0;
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
	if (id == 1)
		return 0;
	elements[id].slide = slide;
	setprecalc(3);
	return 0;
}

int setElementViscousity(Uint16 id, int viscousity) {
	if (id == 1)
		return 0;
	if (!viscousity) {
		addConsoleTextLine("Viscousity cannot be set to 0. Using 1.");
		viscousity = 1;
	}
	elements[id].viscousity = viscousity;
	setprecalc(3);
	return 0;
}

int setElementColor(Uint16 id, unsigned char r, unsigned char g, unsigned char b) {
	if (id == 1)
		return 0;
	elements[id].r = r;
	elements[id].g = g;
	elements[id].b = b;
	setprecalc(3);
	return 0;
}

int setElementR(Uint16 id, unsigned char r) {
	if (id == 1)
		return 0;
	elements[id].r = r;
	setprecalc(3);
	return 0;
}

int setElementG(Uint16 id, unsigned char g) {
	if (id == 1)
		return 0;
	elements[id].g = g;
	setprecalc(3);
	return 0;
}

int setElementB(Uint16 id, unsigned char b) {
	if (id == 1)
		return 0;
	elements[id].b = b;
	setprecalc(3);
	return 0;
}

int setElementBias(Uint16 id, bool b) {
	if (id == 1)
		return 0;
	elements[id].nobias = b;
	setprecalc(3);
	return 0;
}

int setElementCustomColor(Uint16 id, int i, int c, unsigned char b) {
	if (id == 1)
		return 0;
	if ((i == 1) && (c == 1))
		elements[id].cr1 = b;
	if ((i == 1) && (c == 2))
		elements[id].cg1 = b;
	if ((i == 1) && (c == 3))
		elements[id].cb1 = b;
	if ((i == 2) && (c == 1))
		elements[id].cr2 = b;
	if ((i == 2) && (c == 2))
		elements[id].cg2 = b;
	if ((i == 2) && (c == 3))
		elements[id].cb2 = b;
	if ((i == 3) && (c == 1))
		elements[id].cr3 = b;
	if ((i == 3) && (c == 2))
		elements[id].cg3 = b;
	if ((i == 3) && (c == 3))
		elements[id].cb3 = b;
	setprecalc(3);
	return 0;
}

int setElementIcon(Uint16 id, Pic* icon) {
	if (id == 1)
		return 0;
	elements[id].icon = icon;
	return 0;
}

Element* getElement(Uint16 id) {
	if (!id)
		return elements;
	if ((id < elementsmax) && (elements[id].name))
		return &(elements[id]);
	return 0;
}

int findGroup(char* groupname, bool create, int order) {
	char* name = messageReplace(groupname);
	if (order == -1)
		order = 65535;
	std::list<Group*>::iterator it;
	int i = 0;
	for (it = groups.begin(); it != groups.end(); it++) {
		if (!strcmp(name, (*it)->name)) {
			return i;
		}
		i++;
	}
	if (!create)
		return 0;
	Group* g = new Group();
	g->name = new char[strlen(name) + 1];
	strcpy(g->name, name);
	g->icon = 0;
	std::list<int>::iterator it2 = grouporder.begin();
	i = 0;
	for (it = groups.begin(); it != groups.end();) {
		if (*it2 > order) {
			groups.insert(it, g);
			grouporder.insert(it2, order);
			return i;
		}
		it++;
		it2++;
		i++;
	}
	groups.push_back(g);
	grouporder.push_back(order);
	return i;
}

Group* getGroup(int value) {
	unsigned int v = value;
	if (v >= groups.size())
		return 0;
	std::list<Group*>::iterator it = groups.begin();
	for (int i = 0; i < value; i++)
		it++;
	return (*it);
}

int getGroupElement(char* name, int num) {
	int i = 0;
	unsigned int n = num;
	if (!getVar(name, &i))
		i = findGroup(name, false, -1);
	Group* g = getGroup(i);
	if (!g)
		return 0;
	if (g->elements.size() < n)
		return 0;
	std::list<int>::iterator it = g->elements.begin();
	for (i = 1; i < num; i++)
		it++;
	return *it;
}

int getGroupElementOrder(char* name, int num) {
	int i = 0;
	unsigned int n = num;
	if (!getVar(name, &i))
		i = findGroup(name, false, -1);
	Group* g = getGroup(i);
	if (!g)
		return 0;
	if (g->elements.size() < n)
		return 0;
	std::list<int>::iterator it = g->elementorder.begin();
	for (i = 1; i < num; i++)
		it++;
	return *it;
}

bool isElementInGroup(Group* group, int element) {
	std::list<int>::iterator it;
	std::list<int>::iterator it2 = group->elements.end();
	for (it = group->elements.begin(); it != it2; it++)
		if ((*it) == element)
			return true;
	return false;
}

void addElementToGroup(Group* group, int element, int pos) {
	if (element == 1)
		return;
	std::list<int>::iterator it;
	std::list<int>::iterator it2;
	it2 = group->elementorder.begin();
	for (it = group->elements.begin(); it != group->elements.end(); it++)
		if (*it == element)
			return;
	for (it = group->elements.begin(); it != group->elements.end();) {
		if (*it2 > pos) {
			group->elements.insert(it, element);
			group->elementorder.insert(it2, pos);
			return;
		}
		it++;
		it2++;
	}
	group->elements.push_back(element);
	group->elementorder.push_back(pos);
}

void clearGroup(char* name) {
	int i = 0;
	if (!getVar(name, &i))
		i = findGroup(name, false, -1);
	Group* g = getGroup(i);
	if (!g)
		return;
	g->elements.clear();
	g->elementorder.clear();
}

void clearGroups() {
	std::list<Group*>::iterator it3;
	for (it3 = groups.begin(); it3 != groups.end(); it3++) {
		(*it3)->elements.clear();
		(*it3)->elementorder.clear();
	}
	groups.clear();
	grouporder.clear();
	findGroup("None", true, 0);
}

int countGroups() {
	return groups.size();
}