#include "vars.h"
#include "config.h"
#include "sdlbasics.h"
#include <math.h>
#define PI 3.141592654
#include <cstdint>
#include <cstring>

#define get_e int e = 0; if (!getVar(tmp, &e)) e = findElement(tmp, true);

std::list<Var*> vars;
int* parameters[MAXPARAMETERS + 1];
int parameterpos = -1;
Var* debugparameter;
Var* debugvar;

std::list<Var*>* getVars() {
	return &vars;
}

uintptr_t setVar(char* name, int value, bool set) {
	char tmp[255];
	if (sscanf(name, "ELEMENT%s", (char*)&tmp)) {
		if (set) {
			if (sscanf(tmp, "WEIGHT:%s", (char*)&tmp)) {
				get_e
					setElementWeight(e, value);
				return 0;
			} else if (sscanf(tmp, "SPRAY:%s", (char*)&tmp)) {
				get_e
					setElementSpray(e, value);
				return 0;
			} else if (sscanf(tmp, "SLIDE:%s", (char*)&tmp)) {
				get_e
					setElementSlide(e, value);
				return 0;
			} else if (sscanf(tmp, "VISCOUSITY:%s", (char*)&tmp)) {
				get_e
					setElementViscousity(e, value);
				return 0;
			} else if (sscanf(tmp, "R:%s", (char*)&tmp)) {
				get_e
					setElementR(e, value);
				return 0;
			} else if (sscanf(tmp, "G:%s", (char*)&tmp)) {
				get_e
					setElementG(e, value);
				return 0;
			} else if (sscanf(tmp, "B:%s", (char*)&tmp)) {
				get_e
					setElementB(e, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMR1:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 1, 1, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMG1:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 1, 2, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMB1:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 1, 3, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMR2:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 2, 1, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMG2:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 2, 2, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMB2:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 2, 3, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMR3:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 3, 1, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMG3:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 3, 2, value);
				return 0;
			} else if (sscanf(tmp, "CUSTOMB3:%s", (char*)&tmp)) {
				get_e
					setElementCustomColor(e, 3, 3, value);
				return 0;
			} else if (sscanf(tmp, "DEATHRATE:%s", (char*)&tmp)) {
				unsigned int i;
				if (sscanf(tmp, "%i:%s", &i, (char*)&tmp) == 2) {
					get_e
						if ((getElement(e)->dies)->size() == 0)
							return 0;
					std::list<Die*>::iterator it2 = getElement(e)->dies->begin();
					if (i <= (getElement(e)->dies)->size())
						for (unsigned int ii = 0; ii < i; ii++) it2++;
					(*it2)->rate = value;
					setprecalc(3);
					return 0;
				}
			}
		} else
			return 0;
	}
	std::list<Var*>::iterator it = vars.begin();
	if (strchr(name, '[') && (strchr(name, '[') < strchr(name, ']'))) {
		char* t2 = strchr(name, '[');
		char* t3 = strchr(name, ']');
		*t2 = 0;
		*t3 = 0;
		int var = 0;
		getVar(t2 + 1, &var);
		sprintf(tmp, "%s%i", name, var);
		*t2 = '[';
		*t3 = ']';
		return setVar(tmp, value, set);
	}
	while (it != vars.end()) {
		if (!strcmp((*it)->name, name)) {
			if (set) (*it)->value = value;
			return (uintptr_t)*it;
		}
		it++;
	}
	Var* v = new Var;
	strcpy(v->name, name);
	v->value = value;
	vars.push_back(v);
	return (uintptr_t)v;
}

int getVar(char* name, int* value) {
	if (!name) { *value = 0; return false; }
	char tmp[255];
	if (name[0] == '$') {
		if ((parameterpos != -1) && (parameters[parameterpos] != NULL) && (name[2] == '\0')) {
			int i = name[1] - '0';
			if ((i >= 0) && (i < 10)) {
				*value = parameters[parameterpos][i]; return true;
			} else {
				*value = 0; return false;
			}
		} else {
			*value = 0; return false;
		}
	}
	if (sscanf(name, "ELEMENT%s", (char*)&tmp)) {
		if (!strcmp(tmp, ":COUNT")) {
			*value = getelementscount();
			return true;
		} else if (sscanf(tmp, ":%s", (char*)&tmp)) {
			get_e
				* value = findElement(tmp, true);
			return true;
		} else if (sscanf(tmp, "WEIGHT:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->weight;
			return true;
		} else if (sscanf(tmp, "SPRAY:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->spray;
			return true;
		} else if (sscanf(tmp, "SLIDE:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->slide;
			return true;
		} else if (sscanf(tmp, "VISCOUSITY:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->viscousity;
			return true;
		} else if (sscanf(tmp, "R:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->r;
			return true;
		} else if (sscanf(tmp, "G:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->g;
			return true;
		} else if (sscanf(tmp, "B:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->b;
			return true;
		} else if (sscanf(tmp, "ICONTYPE:%s", (char*)&tmp)) {
			get_e
				* value = (uintptr_t)(getElement(e)->icon->type);
			return 7;
		} else if (sscanf(tmp, "ICONTEXT:%s", (char*)&tmp)) {
			get_e
				* value = (uintptr_t)(getElement(e)->icon->text);
			return 7;
		} else if (sscanf(tmp, "CUSTOMR1:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cr1;
			return 7;
		} else if (sscanf(tmp, "CUSTOMG1:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cg1;
			return true;
		} else if (sscanf(tmp, "CUSTOMB1:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cb1;
			return true;
		} else if (sscanf(tmp, "CUSTOMR2:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cr2;
			return true;
		} else if (sscanf(tmp, "CUSTOMG2:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cg2;
			return true;
		} else if (sscanf(tmp, "CUSTOMB2:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cb2;
			return true;
		} else if (sscanf(tmp, "CUSTOMR3:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cr3;
			return true;
		} else if (sscanf(tmp, "CUSTOMG3:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cg3;
			return true;
		} else if (sscanf(tmp, "CUSTOMB3:%s", (char*)&tmp)) {
			get_e
				* value = getElement(e)->cb3;
			return true;
		} else if (sscanf(tmp, "DEATHRATE:%s", (char*)&tmp)) {
			unsigned int i;
			if (sscanf(tmp, "%i:%s", &i, (char*)&tmp) == 2) {
				get_e
					if ((getElement(e)->dies)->size() == 0) {
						*value = 0;
						return false;
					}
				std::list<Die*>::iterator it2 = getElement(e)->dies->begin();
				if (i <= (getElement(e)->dies)->size())
					for (unsigned int ii = 0; ii < i; ii++) it2++;
				*value = (*it2)->rate;
				return true;
			}
		}
	} else if (sscanf(name, "GROUP:%s", (char*)&tmp)) {
		int val = 0;
		for (unsigned int i = 0; i < strlen(tmp) - 1; i++)
			if (tmp[i] == ':') {
				tmp[i] = 0;
				getVar(tmp + i + 1, &val);
				i = strlen(tmp);
				*value = getGroupElement(tmp, val);
				return true;
			}
		*value = findGroup(tmp, false, -1);
		return true;
	} else if (sscanf(name, "GROUPORDER:%s", (char*)&tmp)) {
		int val = 0;
		for (unsigned int i = 0; i < strlen(tmp) - 1; i++)
			if (tmp[i] == ':') {
				tmp[i] = 0;
				getVar(tmp + i + 1, &val);
				i = strlen(tmp);
				*value = getGroupElementOrder(tmp, val);
				return true;
			}
		*value = 0;
		return false;
	} else if (sscanf(name, "COUNT:GROUP:%s", (char*)&tmp)) {
		int i = 0;
		if (!getVar(name + 12, &i)) getVar(name + 6, &i);
		if (getGroup(i))
			*value = getGroup(i)->elements.size();
		else
			*value = 0;
		return true;
	} else if (!strcmp(name, "COUNT:GROUPS")) {
		*value = countGroups();
		return true;
	} else if (sscanf(name, "%i", value))
		return true;
	std::list<Var*>::iterator it = vars.begin();
	while (it != vars.end()) {
		if (!strcmp((*it)->name, name)) {
			*value = (*it)->value;
			return true;
		}
		it++;
	}
	*value = 0;
	return false;
}

Varint::Varint() {
	a = 0;
	b = 0;
	value = NULL;
	fixedvalue = 0;
	ok = true;
	function = 0;
	varvalue = 0;
	text = new char[1];
	text[0] = 0;
	max = 32768;
	trigger = 0;
	params = 0;
}

Varint::Varint(char* v, int max, int f) {
	a = new Varint(v);
	b = 0;
	function = f;
	ok = true;
	varvalue = 0;
	value = v;
	text = new char[strlen(v) + 1];
	strcpy(text, v);
	this->max = max;
	trigger = 0;
	params = 0;
}

Varint::Varint(int v) {
	value = NULL;
	fixedvalue = v;
	ok = true;
	function = 0;
	varvalue = 0;
	text = new char[20];
	sprintf(text, "%i", v);
	max = 32768;
	a = 0;
	b = 0;
	trigger = 0;
	params = 0;
}

Varint::Varint(char* v, int max) {
	char* ov = v;
	a = 0;
	b = 0;
	varvalue = nullptr;
	this->max = max;
	value = 0;
	trigger = 0;
	params = 0;
	if ((v == 0) || (strlen(v) == 0)) {
		text = new char[1];
		text[0] = 0;
		fixedvalue = 0;
		ok = false;
		return;
		function = 0;
	} else {
		text = new char[strlen(v) + 1];
		strcpy(text, v);
		while (*v == ' ')
			v++;
		if (*v == 0) {
			value = 0;
			return;
		}
		function = 0;
		int brace = 0;
		if (v[0] == '(') brace = 1;
		bool closebrace = false;
		if ((v[strlen(v) - 1] == ')')) {
			v[strlen(v) - 1] = 0;
			closebrace = true;
		}

		Token tokens(v + brace);
		char* t = tokens.getToken();
		if (!strcmp(t, "SIN") || !strcmp(t, "COS") || !strcmp(t, "SQRT") || !strcmp(t, "TAN") || !strcmp(t, "SIN-1") || !strcmp(t, "COS-1") || !strcmp(t, "SQRT-1")) {
			char* t2 = tokens.getToken();
			if (!strcmp(t, "SIN")) { a = new Varint(t2, max, 100); function = 1; } else if (!strcmp(t, "COS")) { a = new Varint(t2, max, 101); function = 1; } else if (!strcmp(t, "SQRT")) { a = new Varint(t2, max, 102); function = 1; } else if (!strcmp(t, "TAN")) { a = new Varint(t2, max, 103); function = 1; } else if (!strcmp(t, "SIN-1")) { a = new Varint(t2, max, 104); function = 1; } else if (!strcmp(t, "COS-1")) { a = new Varint(t2, max, 105); function = 1; } else if (!strcmp(t, "TAN-1")) { a = new Varint(t2, max, 106); function = 1; }
		} else {
			if ((t[0] == '*') && (strlen(t) > 1)) {
				trigger = (void*)findTrigger(t + 1, 0);
				params = getParameters(&tokens);
				ok = true;
				delete(t);
				delete(v);
				return;
			}
		}
		delete (t);

		char* t2, * v2;
		if (a) {
			t2 = tokens.getToken(true);
			v2 = tokens.getRest();
		} else {
			tokens.reset();
			t = tokens.getuntillast();
			if (!t) {
				tokens.reset();
				t = tokens.getToken(true);
			}
			t2 = tokens.getToken(true);
			v2 = tokens.getRest();
		}

		if (t2 && v2) {
			if (!a) a = new Varint(t);
			b = new Varint(v2);
			if (!strcmp(t2, "+")) function = 2;
			else if (!strcmp(t2, "-")) function = 3;
			else if (!strcmp(t2, "*")) function = 4;
			else if (!strcmp(t2, "/")) function = 5;
			else if (!strcmp(t2, "%")) function = 6;
			else if (!strcmp(t2, "&")) function = 7;
			else if (!strcmp(t2, "|")) function = 8;
			else if (!strcmp(t2, "&&")) function = 9;
			else if (!strcmp(t2, "||")) function = 10;
			else if (!strcmp(t2, "!=")) function = 11;
			else if (!strcmp(t2, "==")) function = 12;
			else if (!strcmp(t2, "<")) function = 13;
			else if (!strcmp(t2, "<=")) function = 14;
			else if (!strcmp(t2, ">")) function = 15;
			else if (!strcmp(t2, ">=")) function = 16;
			else if (!strcmp(t2, "RAND")) function = 17;
			else if (!strcmp(t2, "PIXEL")) function = 18;
			else if (!strcmp(t2, "INGROUP")) function = 19;
			else if (!strcmp(t2, "MIN")) function = 20;
			else if (!strcmp(t2, "MAX")) function = 21;
			else if (!strcmp(t2, "^")) function = 22;
			else {
				ok = false;
				delete (t2);
				delete(ov);
				return;
			}
			ok = true;
			delete (t2);
			delete(ov);
			return;
		}

		if (function)
			value = 0;
		else {
			char tmp[255];
			if (t[0] == '$') {
				int i = t[1] - '0';
				if ((i >= 0) && (i < 10))
					function = 200 + i;
				value = t;
			} else if (t[strlen(t) - 1] == '%') {
				float d;
				sscanf(t, "%f", &d);
				function = 0;
				value = 0;
				fixedvalue = (int)(d * max / 100);
			} else if (sscanf(t, "%i", &fixedvalue)) {
				delete(t);
				value = 0;
				function = 0;
			} else if (sscanf(t, "ELEMENT%s", (char*)&tmp) || sscanf(t, "GROUP:%s", (char*)&tmp)
				|| sscanf(t, "COUNT:GROUP:%s", (char*)&tmp) || sscanf(t, "COUNT:GROUPS%s", (char*)&tmp)) {
				delete(t);
				if (closebrace) v[strlen(v)] = ')';
				value = new char[strlen(v) + 1];
				strcpy(value, v);
			} else if (strchr(t, '[') && (strchr(t, '[') < strchr(t, ']'))) {
				*(strchr(t, ']')) = 0;
				char* tmp = new char[strlen(strchr(t, '['))];
				strcpy(tmp, strchr(t, '[') + 1);
				b = new Varint(tmp);
				*(strchr(t, '[')) = 0;
				a = new Varint(t);
				function = 300;
			} else {
				std::list<Var*>::iterator it = vars.begin();
				while (it != vars.end()) {
					if (!strcmp((*it)->name, t))
						varvalue = (*it);
					it++;
				}
				if (varvalue == 0) {
					varvalue = new Var;
					strcpy(varvalue->name, t);
					varvalue->value = 0;
					vars.push_back(varvalue);
				}
				value = t;
			}
		}
	}
	ok = true;
	delete(ov);
	return;
}

Varint::~Varint() {
	delete (a);
	delete (b);
	delete (text);
	delete (value);
}

int Varint::val() {
	static Varint* debug = 0;
	static int tmp1 = 0, tmp2 = 0, tmp3 = 0;
	if (debugvar->value && (debug != this) && text) {
		char* tmp = new char[512 + strlen(text)];
		sprintf(tmp, "calculating var: %s", text);
		std::cout << tmp << std::endl;
		debug = this;
		int v = val();
		debug = 0;
		sprintf(tmp, "calculated var: %s = %i", text, v);
		std::cout << tmp << std::endl;
		delete(tmp);
		return v;
	}

	if (!ok) return 0;
	if (function) {
		int i;
		if ((function > 199) && (function < 210)) {
			if ((parameterpos >= 0) && parameters[parameterpos]) return parameters[parameterpos][function - 200];
			return 0;
		}
		switch (function) {
		case 1:
			return a->val();
			break;
		case 2:
			return a->val() + b->val();
			break;
		case 3:
			return a->val() - b->val();
			break;
		case 4:
			return a->val() * b->val();
			break;
		case 5:
			if (b->val()) return a->val() / b->val();
			return 0;
			break;
		case 6:
			return a->val() % b->val();
			break;
		case 7:
			return a->val() & b->val();
			break;
		case 8:
			return a->val() | b->val();
			break;
		case 9:
			return a->val() && b->val();
			break;
		case 10:
			return a->val() || b->val();
			break;
		case 11:
			return a->val() != b->val();
			break;
		case 12:
			return a->val() == b->val();
			break;
		case 13:
			return a->val() < b->val();
			break;
		case 14:
			return a->val() <= b->val();
			break;
		case 15:
			return a->val() > b->val();
			break;
		case 16:
			return a->val() >= b->val();
			break;
		case 17:
			return RANDOMNUMBER * (b->val() - a->val()) / 32768 + a->val();
			break;
		case 18:
			return getPixel(a->val(), b->val());
			break;
		case 19:
			return isElementInGroup(getGroup(b->val()), a->val());
			break;
		case 20:
			tmp1 = a->val();
			tmp2 = b->val();
			return (tmp1 < tmp2) ? tmp1 : tmp2;
			break;
		case 21:
			tmp1 = a->val();
			tmp2 = b->val();
			return (tmp1 < tmp2) ? tmp2 : tmp1;
			break;
		case 22:
			tmp1 = a->val();
			tmp2 = b->val();
			tmp3 = 1;
			for (i = 0; i < tmp2; i++) tmp3 *= tmp1;
			return tmp3;
			break;
		case 100:
			return (int)(sin((double)PI * a->val() / 180000) * 1000.0);
			break;
		case 101:
			return (int)(cos((double)PI * a->val() / 180000) * 1000.0);
			break;
		case 102:
			return (int)(sqrt((float)a->val()));
			break;
		case 103:
			return (int)(tan((double)PI * a->val() / 180000) * 1000.0);
			break;
		case 104:
			return (int)(asin((double)a->val() / 1000) * 180000 / PI);
			break;
		case 105:
			return (int)(acos((double)a->val() / 1000) * 180000 / PI);
			break;
		case 106:
			return (int)(atan((double)a->val() / 1000) * 180000 / PI);
			break;
		case 300:
			char tmp[255];
			sprintf(tmp, "%s%i", a->text, b->val());
			int i;
			getVar(tmp, &i);
			return i;
			break;
		}
	}
	if (trigger) {
		if (params) addparams(calcparams(params));
		int i = ((Trigger*)trigger)->exec();
		if (params) removeparams();
		setreturn = false;
		return i;
	}
	if (value == NULL) return fixedvalue;
	if (varvalue) return varvalue->value;
	int r;
	if (sscanf(value, "%i", &r))
		return r;
	else
		getVar(value, &r);
	return r;
}

