#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "compiler.h"
#include "SDL/SDL.h"
#include <cstdlib>
#include <list>
#include "console.h"
#include "output.h"
#include "vars.h"
#include "pics.h"

#define MAX_INTERACTIONS 100
#define MAX_DIES 100

struct Interaction {
	Uint16 element;
	Uint16 toself;
	Uint16 toother;
	Uint16 rate;
	Uint16 except;
	void* trigger;
	char* toString(char* e);
};

struct Die {
	Uint16 dieto;
	Uint16 rate;
};

struct Element {
	std::list<Interaction*>* interactions;
	int interactioncount;
	std::list<Die*>* dies;
	char* name;
	int weight;
	int spray;
	int slide;
	int viscousity;
	Uint16 dietotalrate;
	bool nobias;
	unsigned char r, g, b;
	unsigned char cr1, cg1, cb1;
	unsigned char cr2, cg2, cb2;
	unsigned char cr3, cg3, cb3;
	Pic* icon;
};

struct Group {
	char* name;
	Pic* icon;
	std::list<int> elements;
	std::list<int> elementorder;
};

void initelements();
void clearelements();
int getelementsmax();
int getelementscount();
int setElementWeight(Uint16 id, int weight);
int setElementSpray(Uint16 id, int spray);
int setElementSlide(Uint16 id, int slide);
int setElementViscousity(Uint16 id, int viscousity);
int setElementColor(Uint16 id, unsigned char r, unsigned char g, unsigned char b);
int setElementR(Uint16 id, unsigned char r);
int setElementG(Uint16 id, unsigned char g);
int setElementB(Uint16 id, unsigned char b);
int setElementCustomColor(Uint16 id, int i, int c, unsigned char b);
int setElementIcon(Uint16 id, Pic* icon);
int setElementBias(Uint16 id, bool b);
Uint16 findElement(char* element, bool create = false);
int addInteraction(Uint16 id, Uint16 elementid, Uint16 toself, Uint16 toother, Uint16 rate, Uint16 except, void* trigger, int pos = -1);
void interactionTrigger(void* trigger, Uint16 element1, Uint16 element2, int x1, int y1, int x2, int y2);
int clearInteraction(Uint16 id);
int removeInteraction(Uint16 id, Uint16 pos);
int clearDie(Uint16 id);
int addDie(Uint16 id, Uint16 dieto, Uint16 rate);
Element* getElement(Uint16 id);

int findGroup(char* name, bool create, int order);
Group* getGroup(int value);
int getGroupElement(char* name, int num);
int getGroupElementOrder(char* name, int num);
bool isElementInGroup(Group* group, int element);
void addElementToGroup(Group* group, int element, int pos);
void clearGroup(char* name);
void clearGroups();
int countGroups();

#endif
