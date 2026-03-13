#include "trigger.h"
#include "keys.h"

std::list<Key*> keys;

void addkey(char* name, int keycode) {
	std::list<Key*>::iterator it = keys.begin();
	while (it != keys.end()) {
		if ((*it)->code == keycode)
			return;
		it++;
	}
	Key* key = new Key();
	key->code = keycode;
	key->name = name;
	keys.push_front(key);
}

void execkey(char* type, int keycode) {
	char tmp[1024];
	std::list<Key*>::iterator it = keys.begin();
	while (it != keys.end()) {
		if ((*it)->code == keycode) {
			sprintf(tmp, "%s%s", type, (*it)->name);
			findTrigger(tmp, 0)->exec();
		}
		it++;
	}
}